#include <FlexEngine/Factory/TreeFactory.h>

#include <FlexEngine/Container/Utility.h>
#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Factory/GeometryUtils.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/StandardRandom.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>
#include <FlexEngine/Resource/XMLHelpers.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

VegetationVertex VegetationVertex::Construct(const FatVertex& vertex)
{
    VegetationVertex result;
    result.position_ = vertex.position_;
    result.tangent_ = vertex.tangent_;
    result.binormal_ = vertex.binormal_;
    result.normal_ = vertex.normal_;
    result.geometryNormal_ = vertex.geometryNormal_;
    result.uv_.x_ = vertex.uv_[0].x_;
    result.uv_.y_ = vertex.uv_[0].y_;
    result.param_.x_ = vertex.mainAdherence_;
    result.param_.y_ = vertex.branchAdherence_;
    result.param_.z_ = vertex.phase_;
    result.param_.w_ = vertex.edgeOscillation_;
    return result;
}

PODVector<VertexElement> VegetationVertex::Format()
{
    static const PODVector<VertexElement> format =
    {
        VertexElement(TYPE_VECTOR3, SEM_POSITION),
        VertexElement(TYPE_VECTOR3, SEM_TANGENT),
        VertexElement(TYPE_VECTOR3, SEM_BINORMAL),
        VertexElement(TYPE_VECTOR3, SEM_NORMAL, 0),
        VertexElement(TYPE_VECTOR3, SEM_NORMAL, 1),
        VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 1),
    };
    return format;
}

// Branch Group
TreeFactory::BranchGroup& TreeFactory::BranchGroup::addBranchGroup()
{
    TreeFactory::BranchGroup group;

    // Inherit something
    group.p.material_ = p.material_;

    m_childrenBranchGroups.Push(group);
    return m_childrenBranchGroups.Back();
}
TreeFactory::FrondGroup& TreeFactory::BranchGroup::addFrondGroup()
{
    TreeFactory::FrondGroup group;
    m_childrenFrondGroups.Push(group);
    return m_childrenFrondGroups.Back();
}


// Helpers
TreeFactory::Point::Point(const Point& from, const Point& to, const float factor)
    : position(Lerp(from.position, to.position, factor))
    , radius(Lerp(from.radius, to.radius, factor))
    , location(Lerp(from.location, to.location, factor))

    , distance(Lerp(from.distance, to.distance, factor))
    , direction(Lerp(from.direction, to.direction, factor))
    , segmentLength(Lerp(from.segmentLength, to.segmentLength, factor))
    , zAxis(Lerp(from.zAxis, to.zAxis, factor))
    , xAxis(Lerp(from.xAxis, to.xAxis, factor))
    , yAxis(Lerp(from.yAxis, to.yAxis, factor))

    , relativeDistance(Lerp(from.relativeDistance, to.relativeDistance, factor))

    , branchAdherence(Lerp(from.branchAdherence, to.branchAdherence, factor))
    , phase(Lerp(from.phase, to.phase, factor))
{
}
void TreeFactory::Branch::addPoint(const float location,
                                 const Vector3& position, const float radius,
                                 const float phase, const float adherence)
{
    const unsigned last = points.Size();
    points.Push(Point());

    points[last].location = location;
    points[last].position = position;
    points[last].radius = radius;
    points[last].branchAdherence = adherence;
    points[last].phase = phase;

    // Reset and skip if first
    if (last == 0)
    {
        points[0].distance = 0.0f;
        points[0].direction = Vector3::ZERO;
        points[0].segmentLength = 0.0f;
        points[0].zAxis = Vector3::ZERO;
        points[0].xAxis = Vector3::ZERO;
        points[0].yAxis = Vector3::ZERO;
        points[0].relativeDistance = 0.0f;
        return;
    }

    // Compute distance
    points[last].distance = points[last - 1].distance + points[last - 1].segmentLength;

    // Compute direction
    const Vector3 dir = points[last].position - points[last - 1].position;
    points[last - 1].direction = dir.Normalized();
    points[last - 1].segmentLength = dir.Length();
    points[last].direction = points[last - 1].direction;
    points[last].segmentLength = 0.0f;

    // Compute relative distance
    ///                L        ln(r1/r2)
    /// u1 = u0 + ----------- * ---------
    ///           2 * pi * r0   r1/r0 - 1
    const float u0 = points[last - 1].relativeDistance;
    const float L = points[last - 1].segmentLength;
    const float r0 = points[last - 1].radius;
    const float r1 = points[last].radius;
    const float rr = r1 / r0;
    /// ln(x) / (x - 1) ~= 1 - (x - 1) / 2 + ...
    const float k = 1 - (rr - 1) / 2;
    points[last].relativeDistance = u0 + k * L / (2 * M_PI * r0);

    // Function to compute axises
    auto computeAxisXY = [](Point& p)
    {
        p.xAxis = ConstructOrthogonalVector(p.zAxis);
        p.yAxis = CrossProduct(p.zAxis, p.xAxis).Normalized();

        // Flip
        if (p.yAxis.y_ < 0.0f)
        {
            p.yAxis = -p.yAxis;
            p.xAxis = CrossProduct(p.yAxis, p.zAxis);
        }
    };

    // Compute axises for last point
    points[last].zAxis = points[last].direction;
    computeAxisXY(points[last]);

    // Compute axises for first point
    if (last == 1)
    {
        points[0].zAxis = points[0].direction;
        computeAxisXY(points[0]);
        return;
    }

    // Compute axises for middle points
    points[last - 1].zAxis = (points[last - 2].direction + points[last - 1].direction).Normalized();
    computeAxisXY(points[last - 1]);
}
TreeFactory::Point TreeFactory::Branch::getPoint(const float location) const
{
    // Compute points
    unsigned first = 0;
    unsigned second = 0;
    while (second + 1 < points.Size() && points[second].location < location)
        ++second;
    first = second == 0 ? 0 : second - 1;

    // Compute factor
    float factor = first == second
        ? 1.0f
        : UnLerp(points[first].location, points[second].location, location);

    // Generate point
    return Point(points[first], points[second], factor);

}


// Generation
struct TreeFactory::InternalGenerator
{
    void generateBranchGroup(const LodInfo& lodInfo, const BranchGroup& branchGroup,
                             Vector<Branch>& branchesArray, const Branch* parent)
    {
        m_random.Reset(branchGroup.p.seed_ == RootGroup ? m_seedGenerator.Random() : branchGroup.p.seed_);
        if (branchGroup.p.frequency_ == RootGroup)
        {
            // Actually this group is special case for root group

            // Generate children
            for (const BranchGroup& childGroup : branchGroup.branches())
            {
                if (childGroup.p.frequency_ != 0)
                {
                    // #TODO "Empty branch group must have only single branches"
                    continue;
                }
                generateBranchGroup(lodInfo, childGroup, branchesArray, nullptr);
            }
        }
        else if (branchGroup.p.frequency_ == 0)
        {
            branchesArray.Push(Branch());
            Branch& branch = branchesArray.Back();
            branch.position = branchGroup.p.position_;
            branch.direction = branchGroup.p.direction_;
            generateBranch(branch, lodInfo, branchGroup);

            // Generate children
            for (const BranchGroup& childGroup : branchGroup.branches())
            {
                generateBranchGroup(lodInfo, childGroup, branch.children, &branch);
            }
        }
        else
        {
            if (!parent)
            {
                // #TODO "Branch group must have parent"
                return;
            }

            // Compute locations
            Vector<float> locations = computeLocations(*branchGroup.p.distribFunction,
                                                       branchGroup.p.location,
                                                       branchGroup.p.frequency_);
            const float parentLength = parent->totalLength;

            // Emit branches
            const unsigned baseIndex = m_random.Random() % 2;
            for (unsigned idx = 0; idx < branchGroup.p.frequency_; ++idx)
            {
                const float location = locations[idx];

                // Allocate
                branchesArray.Push(Branch());
                Branch& branch = branchesArray.Back();
                branch.parentLength = parentLength;

                // Get connection point
                const Point connectionPoint = parent->getPoint(location);
                branch.location = connectionPoint.location;
                branch.parentRadius = connectionPoint.radius;
                branch.parentBranchAdherence = connectionPoint.branchAdherence;
                branch.parentPhase = connectionPoint.phase;
                branch.position = connectionPoint.position;

                // Init branch direction
                initBranch(branch, connectionPoint, 
                           baseIndex + idx, branchGroup);

                // Apply angle
                const float angle = branchGroup.p.angle->Compute(location);
                branch.direction = Cos(angle) * connectionPoint.zAxis + Sin(angle) * branch.direction;

                // Generate branch
                generateBranch(branch, lodInfo, branchGroup);

                // Generate children
                for (const BranchGroup& childGroup : branchGroup.branches())
                {
                    generateBranchGroup(lodInfo, childGroup, branch.children, &branch);
                }
            }
        }
    }
    static unsigned fixNumberOfSegments(const float num)
    {
        return Max(3u, static_cast<unsigned>(num));
    }
    void generateBranch(Branch& branch,
                        const LodInfo& lodInfo,
                        const BranchGroup& branchGroupInfo)
    {
        branch.groupInfo = branchGroupInfo;
        branch.groupInfo.clear();

        // Compute length
        branch.totalLength = branch.parentLength * branchGroupInfo.p.length->Compute(branch.location);

        // Compute oscillation phase
        const float phaseOffset = (m_random.FloatFrom01() - 0.5f) * branchGroupInfo.p.phaseRandom;

        // Compute number of segments and step length
        const unsigned numSegments = fixNumberOfSegments(lodInfo.numLengthSegments_ * branchGroupInfo.p.geometryMultiplier_);
        const float step = branch.totalLength / numSegments;

        // Compute weight offset
        const float weightOffset = branchGroupInfo.p.weight->Compute(branch.location) / numSegments;

        // Emit points
        Vector3 position = branch.position;
        Vector3 direction = branch.direction;
        for (unsigned idx = 0; idx <= numSegments; ++idx)
        {
            const float location = static_cast<float>(idx) / numSegments;
            const float radius = branchGroupInfo.p.radius->Compute(location) * branch.parentRadius;
            const float localAdherence = branch.groupInfo.p.branchAdherence->Compute(location);
            const float phase = branch.parentPhase + phaseOffset;
            const float adherence = localAdherence + branch.parentBranchAdherence;
            branch.addPoint(location, position, radius, phase, adherence);

            direction = (direction + Vector3(0.0f, -1.0f, 0.0f) * weightOffset).Normalized();
            position += direction * step;
        }
    }
    void initBranch(Branch& branch, const Point& connectionPoint,
                    const unsigned idx, const BranchGroup& branchGroupInfo)
    {
        switch (branchGroupInfo.p.distrib.type_)
        {
        case TreeFactory::DistributionType::Alternate:
            initAlternateBranch(branch, connectionPoint, idx, branchGroupInfo);
            break;
        default:
            break;
        }
    }
    void initAlternateBranch(Branch& branch, const Point& connectionPoint,
                             const unsigned idx, const BranchGroup& branchGroupInfo)
    {
        assert(branchGroupInfo.p.distrib.type_ == TreeFactory::DistributionType::Alternate);

        branch.location = connectionPoint.location;
        branch.parentRadius = connectionPoint.radius;
        branch.position = connectionPoint.position;

        const float angle = branchGroupInfo.p.distrib.alternateParam_.angleRandomness_ * m_random.FloatFrom11()
                                    + branchGroupInfo.p.distrib.alternateParam_.angleStep_ * idx;
        branch.direction = connectionPoint.xAxis * Cos(angle) + connectionPoint.yAxis * Sin(angle);
    }
    Vector<float> computeLocations(const MathFunction& distribFunction, 
                                   const Vector2& locationRange, const unsigned count)
    {
        // Compute sum
        Vector<float> factors = { 0 };
        float integralSum = 0.0f;
        for (unsigned idx = 0; idx < count; ++idx)
        {
            const float factor = static_cast<float>(idx) / count;
            integralSum += distribFunction.Compute(factor);
            factors.Push(integralSum);
        }

        // Normalize and recenter factors
        Vector<float> locations;
        for (unsigned idx = 0; idx < count; ++idx)
        {
            const float normalizedLocation = (factors[idx] + factors[idx + 1]) / (2 * integralSum);
            locations.Push(Lerp(locationRange.x_,
                                     locationRange.y_,
                                     normalizedLocation));
        }

        return locations;
    }

    StandardRandom m_seedGenerator;
    StandardRandom m_random;
};


// Triangulation
struct TreeFactory::InternalBranchTriangulator
{
    struct PPPoint : Point
    {
        PPPoint() = default;
        /// @brief Copy ctor
        PPPoint(const Point& base)
        {
            static_cast<Point&>(*this) = base;
        }

        unsigned numVertices = 0; ///< Number of vertices in circle
        unsigned startVertex = 0; ///< Start vertex of circle
    };
    void preprocessPoints(const Branch& branch, const LodInfo& lodInfo,
                          Vector<PPPoint>& points)
    {
        const unsigned numPoints = branch.points.Size();
        assert(numPoints >= 2);

        PushElements(points, branch.points);

        // Compute number of radial segments
        unsigned startVertex = 0;
        for (unsigned idx = 0; idx < numPoints; ++idx)
        {
            const float rawNumSegments = lodInfo.numRadialSegments_ * branch.groupInfo.p.geometryMultiplier_;
            points[idx].numVertices = Max(3u, static_cast<unsigned>(rawNumSegments));
            points[idx].startVertex = startVertex;
            startVertex += points[idx].numVertices + 1;
        }
    }
    void triangulateBranch(unsigned materialIndex, const Branch& branch, const LodInfo& lodInfo,
                           Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        Vector<PPPoint> points;
        if (branch.groupInfo.p.material_.materialIndex_ == materialIndex)
        {
            preprocessPoints(branch, lodInfo, points);
            generateBranchGeometry(branch, points, vertices, indices);
        }

        // Triangulate children
        for (const Branch& child : branch.children)
        {
            triangulateBranch(materialIndex, child, lodInfo, vertices, indices);
        }
    }
    void generateBranchGeometry(const Branch& branch, const Vector<PPPoint>& points,
                                Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        const Vector2 textureScale = branch.groupInfo.p.material_.textureScale_;
        const unsigned baseVertex = vertices.Size();
        for (const PPPoint& point : points)
        {
            // Add points
            for (unsigned idx = 0; idx <= point.numVertices; ++idx)
            {
                const float factor = static_cast<float>(idx) / point.numVertices;
                const float angle = factor * 360;
                const Vector3 normal = (point.xAxis * Cos(angle) + point.yAxis * Sin(angle)).Normalized();

                FatVertex vertex;
                vertex.position_ = point.position + point.radius * normal;
                vertex.geometryNormal_ = normal;
                vertex.normal_ = normal;
                vertex.tangent_ = point.zAxis;
                vertex.binormal_ = CrossProduct(vertex.normal_, vertex.tangent_);
                vertex.uv_[0] = Vector4(factor / textureScale.x_, point.relativeDistance / textureScale.y_, 0, 0);
                vertex.branchAdherence_ = point.branchAdherence;
                vertex.phase_ = point.phase;
                vertex.edgeOscillation_ = 0.0f;

                vertices.Push(vertex);
            }
        }

        // Connect point circles
        for (unsigned pointIndex = 0; pointIndex < points.Size() - 1; ++pointIndex)
        {
            const PPPoint& pointA = points[pointIndex];
            const PPPoint& pointB = points[pointIndex + 1];
            const unsigned numA = pointA.numVertices;
            const unsigned numB = pointB.numVertices;
            const unsigned baseVertexA = baseVertex + pointA.startVertex;
            const unsigned baseVertexB = baseVertex + pointB.startVertex;

            unsigned idxA = 0;
            unsigned idxB = 0;
            const unsigned numTriangles = pointA.numVertices + pointB.numVertices;
            for (unsigned triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                if (idxA * numB <= idxB * numA)
                {
                    // A-based triangle
                    indices.Push(baseVertexA + idxA % (numA + 1));
                    indices.Push(baseVertexA + (idxA + 1) % (numA + 1));
                    indices.Push(baseVertexB + idxB % (numB + 1));
                    ++idxA;
                }
                else
                {
                    // B-based triangle
                    indices.Push(baseVertexB + (idxB + 1) % (numB + 1));
                    indices.Push(baseVertexB + idxB % (numB + 1));
                    indices.Push(baseVertexA + idxA % (numA + 1));
                    ++idxB;
                }
            }
        }
    }
};
struct TreeFactory::InternalFrondTriangulator
{
    void triangulateBranch(unsigned materialIndex, const Branch& branch, const LodInfo& lodInfo,
                           Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        // Triangulate frond groups
        for (const FrondGroup& group : branch.groupInfo.fronds())
        {
            triangulateFrondGroup(materialIndex, branch, group, lodInfo, vertices, indices);
        }

        // Iterate over children
        for (const Branch& child : branch.children)
        {
            triangulateBranch(materialIndex, child, lodInfo, vertices, indices);
        }
    }
    void triangulateFrondGroup(unsigned materialIndex, const Branch& branch, const FrondGroup& frondGroup,
                               const LodInfo& lodInfo,
                               Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        if (frondGroup.material_.materialIndex_ != materialIndex)
        {
            return;
        }
        const float size = frondGroup.size_->Compute(m_random.FloatFrom01());
        switch (frondGroup.type_)
        {
        case FrondType::Simple:
            TriangulateFrond(branch, frondGroup, size, lodInfo, vertices, indices);
            break;
        case TreeFactory::FrondType::RagEnding:
            triangulateRagEndingFrondGroup(branch, frondGroup, size, lodInfo, vertices, indices);
            break;
        case TreeFactory::FrondType::BrushEnding:
            triangulateBrushEndingFrondGroup(branch, frondGroup, size, lodInfo, vertices, indices);
            break;
        default:
            break;
        }
    }
    /// @name Ending
    /// @{
    void TriangulateFrond(const Branch& branch, const FrondGroup& frondGroup,
        const float size, const LodInfo& lodInfo,
        Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        // Instantiate random values
        const FrondParameters& param = frondGroup.param_;
        const float adjustUpToGlobal = param.adjustUpToGlobal_;
        const float yaw = param.yaw_;
        const float pitch = param.pitch_;
        const float roll = param.roll_;
        const Point location = branch.getPoint(1.0f);

        // Compute global axises
        const Quaternion localRotation(location.xAxis, location.yAxis, location.zAxis);
        const Vector3 xAxisGlobal = CrossProduct(Vector3::UP, location.zAxis).Normalized();
        const Vector3 zAxisGlobal = CrossProduct(xAxisGlobal, Vector3::UP).Normalized();
        const Vector3 yAxisGlobal = CrossProduct(zAxisGlobal, xAxisGlobal).Normalized();

        // Skip such cases
        if (Equals(xAxisGlobal.LengthSquared(), 0.0f)
            || Equals(yAxisGlobal.LengthSquared(), 0.0f)
            || Equals(zAxisGlobal.LengthSquared(), 0.0f))
        {
            return;
        }

        // Apply rotations
        const Quaternion globalRotation(xAxisGlobal, yAxisGlobal, zAxisGlobal);
        const Quaternion baseRotation = localRotation.Slerp(globalRotation, adjustUpToGlobal);
        const Quaternion yprRotation = Quaternion(yaw, Vector3::UP)
            * Quaternion(pitch, Vector3::RIGHT) * Quaternion(roll, Vector3::FORWARD);

        const Matrix3 rotationMatrix = (baseRotation * yprRotation).RotationMatrix();
        const Vector3 xAxis = Vector3(rotationMatrix.m00_, rotationMatrix.m10_, rotationMatrix.m20_);
        const Vector3 yAxis = Vector3(rotationMatrix.m01_, rotationMatrix.m11_, rotationMatrix.m21_);
        const Vector3 zAxis = Vector3(rotationMatrix.m02_, rotationMatrix.m12_, rotationMatrix.m22_);

        // #TODO Use custom geometry
        FatVertex vers[4];

        vers[0].position_ = Vector3(-0.5f, 0.0f, 0.0f);
        vers[0].uv_[0] = Vector4(0, 0, 0, 0);

        vers[1].position_ = Vector3(0.5f, 0.0f, 0.0f);
        vers[1].uv_[0] = Vector4(1, 0, 0, 0);

        vers[2].position_ = Vector3(-0.5f, 0.0f, 1.0f);
        vers[2].uv_[0] = Vector4(0, 1, 0, 0);

        vers[3].position_ = Vector3(0.5f, 0.0f, 1.0f);
        vers[3].uv_[0] = Vector4(1, 1, 0, 0);

        Vector<FatVertex> newVertices;
        Vector<FatIndex> newIndices;
        AppendQuadGridToVertices(newVertices, newIndices, vers[0], vers[1], vers[2], vers[3], 2, 2);

        // Compute max factor
        const Vector3 junctionAdherenceFactor = Vector3(1.0f, 0.0f, 1.0f);
        Vector3 maxFactor = Vector3::ONE * M_EPSILON;
        for (FatVertex& vertex : newVertices)
        {
            maxFactor = VectorMax(maxFactor, vertex.position_.Abs());
        }

        // Finalize
        const Vector3 geometryScale = Vector3(size, size, size);
        const Vector3 basePosition = location.position + rotationMatrix * param.junctionOffset_;
        for (FatVertex& vertex : newVertices)
        {
            // Compute factor of junction adherence
            const Vector3 factor = vertex.position_.Abs() / maxFactor;

            // Compute position in global space
            vertex.position_ = basePosition + rotationMatrix * vertex.position_ * geometryScale;

            // Compute distance to junction
            const float len = (vertex.position_ - location.position).Length();

            // Apply gravity
            const Vector3 perComponentGravity = param.gravityIntensity_ * Pow(factor, Vector3::ONE / param.gravityResistance_);
            vertex.position_.y_ -= perComponentGravity.DotProduct(Vector3::ONE);

            // Restore shape
            vertex.position_ = (vertex.position_ - location.position).Normalized() * len + location.position;

            // Apply wind parameters
            vertex.edgeOscillation_ = 0;
            vertex.branchAdherence_ = location.branchAdherence;
            vertex.phase_ = location.phase;
        }

        // Triangles
        AppendGeometryToVertices(vertices, indices, newVertices, newIndices);
    }
    void triangulateRagEndingFrondGroup(const Branch& branch, const FrondGroup& frondGroup,
                                        const float size, const LodInfo& lodInfo,
                                        Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        const RagEndingFrondParameters& param = frondGroup.ragEndingParam_;
        const float halfSize = size / 2;
        const float relativeOffset = param.locationOffset_ / branch.totalLength;
        const Point location = branch.getPoint(1.0f - relativeOffset);
        const Vector3 zyAxis = Lerp(location.zAxis, -location.yAxis, param.zBendingFactor_).Normalized();
        FatVertex vers[9];

        // Center
        vers[4].position_ = location.position
            + (location.zAxis + location.yAxis).Normalized() * param.centerOffset_;
        vers[4].uv_[0] = Vector4(0.5f, 0.5f, 0, 0);
        vers[4].edgeOscillation_ = param.junctionOscillation_;

        // Center -+z
        vers[3].position_ = vers[4].position_ - halfSize * location.zAxis;
        vers[3].uv_[0] = Vector4(0.5f, 0.0f, 0, 0);
        vers[3].edgeOscillation_ = param.edgeOscillation_;
        vers[5].position_ = vers[4].position_ + halfSize * zyAxis;
        vers[5].uv_[0] = Vector4(0.5f, 1.0f, 0, 0);
        vers[5].edgeOscillation_ = param.edgeOscillation_;

        // Center -+x
        vers[1].position_ = vers[4].position_ - halfSize * Lerp(location.xAxis, location.yAxis, param.xBendingFactor_).Normalized();
        vers[1].uv_[0] = Vector4(0.0f, 0.5f, 0, 0);
        vers[1].edgeOscillation_ = param.edgeOscillation_;
        vers[7].position_ = vers[4].position_ + halfSize * Lerp(location.xAxis, -location.yAxis, param.xBendingFactor_).Normalized();
        vers[7].uv_[0] = Vector4(1.0f, 0.5f, 0, 0);
        vers[7].edgeOscillation_ = param.edgeOscillation_;

        // Center -x -+z
        vers[0].position_ = vers[1].position_ - halfSize * location.zAxis;
        vers[0].uv_[0] = Vector4(0.0f, 0.0f, 0, 0);
        vers[0].edgeOscillation_ = param.edgeOscillation_;
        vers[2].position_ = vers[1].position_ + halfSize * zyAxis;
        vers[2].uv_[0] = Vector4(0.0f, 1.0f, 0, 0);
        vers[2].edgeOscillation_ = param.edgeOscillation_;

        // Center +x -+z
        vers[6].position_ = vers[7].position_ - halfSize * location.zAxis;
        vers[6].uv_[0] = Vector4(1.0f, 0.0f, 0, 0);
        vers[6].edgeOscillation_ = param.edgeOscillation_;
        vers[8].position_ = vers[7].position_ + halfSize * zyAxis;
        vers[8].uv_[0] = Vector4(1.0f, 1.0f, 0, 0);
        vers[8].edgeOscillation_ = param.edgeOscillation_;

        // Set wind
        for (FatVertex& vertex : vers)
        {
            vertex.branchAdherence_ = location.branchAdherence;
            vertex.phase_ = location.phase;
        }

        // Triangles
        const unsigned base = vertices.Size();
        AppendQuadToIndices(indices, base, 0, 1, 3, 4, true);
        AppendQuadToIndices(indices, base, 1, 2, 4, 5, true);
        AppendQuadToIndices(indices, base, 3, 4, 6, 7, true);
        AppendQuadToIndices(indices, base, 4, 5, 7, 8, true);
        vertices.Insert(vertices.End(), vers, vers + 9);
    }
    void triangulateBrushEndingFrondGroup(const Branch& branch, const FrondGroup& frondGroup,
                                          const float size, const LodInfo& lodInfo,
                                          Vector<FatVertex>& vertices, Vector<FatIndex>& indices)
    {
        const BrushEndingFrondParameters& param = frondGroup.brushEndingParam_;
        const float halfSize = size / 2;
        const float relativeOffset = param.locationOffset_ / branch.totalLength;
        const float xyAngle = param.spreadAngle_ / 2;
        const Point location = branch.getPoint(1.0f - relativeOffset);

        // Build volume geometry
        static const unsigned NumQuads = 3;
        for (unsigned idx = 0; idx < NumQuads; ++idx)
        {
            const float zAngle = 360.0f * idx / NumQuads;
            const Vector3 position = location.position;
            const Vector3 side = location.xAxis * Cos(zAngle) + location.yAxis * Sin(zAngle);
            const Vector3 direction = Quaternion(xyAngle, side).RotationMatrix() * location.zAxis;
            const Vector3 normal = CrossProduct(side, direction);

            // triangulate
            FatVertex vers[5];
            vers[0].position_ = position - halfSize * side;
            vers[0].uv_[0] = Vector4(0.0f, 0.0f, 0, 0);
            vers[0].edgeOscillation_ = param.junctionOscillation_;
            vers[1].position_ = position + halfSize * side;
            vers[1].uv_[0] = Vector4(1.0f, 0.0f, 0, 0);
            vers[1].edgeOscillation_ = param.junctionOscillation_;
            vers[2].position_ = position - halfSize * side + size * direction;
            vers[2].uv_[0] = Vector4(0.0f, 1.0f, 0, 0);
            vers[2].edgeOscillation_ = param.edgeOscillation_;
            vers[3].position_ = position + halfSize * side + size * direction;
            vers[3].uv_[0] = Vector4(1.0f, 1.0f, 0, 0);
            vers[3].edgeOscillation_ = param.edgeOscillation_;
            vers[4] = Lerp(vers[0], vers[3], 0.5f);
            vers[4].position_ += normal * size * param.centerOffset_;

            // Set wind
            for (FatVertex& vertex : vers)
            {
                vertex.branchAdherence_ = location.branchAdherence;
                vertex.phase_ = location.phase;
            }

            // put data to buffers
            const unsigned base = vertices.Size();
            if (param.centerOffset_ == 0)
            {
                AppendQuadToIndices(indices, base, 0, 1, 2, 3, true);
                vertices.Insert(vertices.End(), vers, vers + 4);
            }
            else
            {
                vertices.Insert(vertices.End(), vers, vers + 5);
                indices.Push(base + 0); indices.Push(base + 1); indices.Push(base + 4);
                indices.Push(base + 1); indices.Push(base + 3); indices.Push(base + 4);
                indices.Push(base + 3); indices.Push(base + 2); indices.Push(base + 4);
                indices.Push(base + 2); indices.Push(base + 0); indices.Push(base + 4);
            }
        }
    }
    /// @}

    StandardRandom m_random;
};


// Main
TreeFactory::TreeFactory(Context* context)
    : Object(context)
{
    // Root has special frequency
    root_.p.frequency_ = RootGroup;
}
TreeFactory::~TreeFactory()
{
}


// Get hierarchy root
TreeFactory::BranchGroup& TreeFactory::GetRoot()
{
    return root_;
}


// Generate
void TreeFactory::generateSkeleton(const LodInfo& lodInfo, Vector<Branch>& trunksArray) const
{
    InternalGenerator generator;
    generator.generateBranchGroup(lodInfo, root_, trunksArray, nullptr);
}


// Triangulate
unsigned TreeFactory::triangulateBranches(unsigned materialIndex, const LodInfo& lodInfo,
                                    const Vector<Branch>& trunksArray, 
                                    Vector<FatVertex>& vertices, Vector<unsigned>& indices) const
{
    const unsigned startIndex = indices.Size();
    InternalBranchTriangulator triangulator;
    for (const Branch& branch : trunksArray)
    {
        triangulator.triangulateBranch(materialIndex, branch, lodInfo, vertices, indices);
    }
    const unsigned numTriangles = (indices.Size() - startIndex) / 3;
    return numTriangles;
}
unsigned TreeFactory::triangulateFronds(unsigned materialIndex, const LodInfo& lodInfo,
                                  const Vector<Branch>& trunksArray, 
                                  Vector<FatVertex>& vertices, Vector<unsigned>& indices) const
{
    // Triangulate
    const unsigned startIndex = indices.Size();
    InternalFrondTriangulator triangulator;
    for (const Branch& branch : trunksArray)
    {
        triangulator.triangulateBranch(materialIndex, branch, lodInfo, vertices, indices);
    }

    // Generate TBN
    const unsigned numTriangles = (indices.Size() - startIndex) / 3;
    CalculateNormals(vertices.Buffer(), vertices.Size(), 
        indices.Buffer() + startIndex, numTriangles);
    CalculateTangents(vertices.Buffer(), vertices.Size(),
        indices.Buffer() + startIndex, numTriangles);

    // Return number of triangles
    return numTriangles;
}


// Wind stuff
float TreeFactory::computeMainAdherence(const float relativeHeight, const float trunkStrength)
{
    return pow(Max(relativeHeight, 0.0f),
               1 / (1 - trunkStrength));
}
void TreeFactory::computeMainAdherence(Vector<FatVertex>& vertices)
{
    // Compute max height
    float maxHeight = 0.0f;
    for (FatVertex& vertex : vertices)
    {
        maxHeight = Max(maxHeight, vertex.position_.y_);
    }

    // Compute wind
    auto compute = [this](const float relativeHeight) -> float
    {
        return computeMainAdherence(relativeHeight, trunkStrength_);
    };
    for (FatVertex& vertex : vertices)
    {
        vertex.mainAdherence_ = compute(vertex.position_.y_ / maxHeight);
    }
}
void TreeFactory::normalizeBranchesAdherence(Vector<FatVertex>& vertices)
{
    // Compute max adherence
    float maxAdherence = M_LARGE_EPSILON;
    for (FatVertex& vertex : vertices)
    {
        maxAdherence = Max(maxAdherence, vertex.branchAdherence_);
    }

    // Compute wind
    for (FatVertex& vertex : vertices)
    {
        vertex.branchAdherence_ /= maxAdherence;
    }
}
void TreeFactory::normalizePhases(Vector<FatVertex>& vertices)
{
    for (FatVertex& vertex : vertices)
    {
        vertex.phase_ = Fract(vertex.phase_);
    }
}


// Main generator
SharedPtr<Model> TreeFactory::GenerateModel(const PODVector<LodInfo>& lodInfos)
{
    assert(!lodInfos.Empty());

    // Prepare buffers for accumulated geometry data
    SharedPtr<VertexBuffer> vertexBuffer = MakeShared<VertexBuffer>(context_);
    vertexBuffer->SetShadowed(true);
    SharedPtr<IndexBuffer> indexBuffer = MakeShared<IndexBuffer>(context_);
    indexBuffer->SetShadowed(true);
    SharedPtr<Model> model = MakeShared<Model>(context_);
    model->SetVertexBuffers({ vertexBuffer }, { 0 }, { 0 });
    model->SetIndexBuffers({ indexBuffer });

    // Number of geometries is equal to number of materials
    const unsigned numMaterials = GetNumMaterials();
    model->SetNumGeometries(numMaterials);

    const unsigned numLods = lodInfos.Size();
    for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
    {
        model->SetNumGeometryLodLevels(materialIndex, numLods);
    }

    PODVector<VegetationVertex> vertexData;
    PODVector<unsigned short> indexData;
    PODVector<unsigned> groupOffsets;
    PODVector<unsigned> groupSizes;

    // Generate data for each material and LOD
    BoundingBox boundingBox;
    unsigned indexOffset = 0;
    for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
    {
        for (unsigned lod = 0; lod < lodInfos.Size(); ++lod)
        {
            const LodInfo& lodInfo = lodInfos[lod];

            // Generate branches
            // #TODO It is possible to move branch generation out of the loop
            Vector<TreeFactory::Branch> branches;
            generateSkeleton(lodInfo, branches);

            // Triangulate all
            Vector<FatVertex> vertices;
            Vector<FatIndex> indices;
            triangulateBranches(materialIndex, lodInfo, branches, vertices, indices);
            triangulateFronds(materialIndex, lodInfo, branches, vertices, indices);
            computeMainAdherence(vertices);
            normalizeBranchesAdherence(vertices);
            normalizePhases(vertices);

            // Compute AABB and radius
            const BoundingBox aabb = CalculateBoundingBox(vertices.Buffer(), vertices.Size());
            const Vector3 aabbCenter = static_cast<Vector3>(aabb.Center());
            modelRadius_ = 0.0f;
            for (const FatVertex& vertex : vertices)
            {
                const float dist = ((vertex.position_ - aabbCenter) * Vector3(1, 0, 1)).Length();
                modelRadius_ = Max(modelRadius_, dist);
            }
            boundingBox.Merge(aabb);

            SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);
            geometry->SetVertexBuffer(0, vertexBuffer);
            geometry->SetIndexBuffer(indexBuffer);
            model->SetGeometry(materialIndex, lod, geometry);
            groupOffsets.Push(indexOffset);
            groupSizes.Push(indices.Size());
            indexOffset += indices.Size();

            unsigned vertexOffset = vertexData.Size();
            for (unsigned i = 0; i < indices.Size(); ++i)
            {
                indexData.Push(static_cast<unsigned short>(indices[i] + vertexOffset));
            }

            for (unsigned i = 0; i < vertices.Size(); ++i)
            {
                vertexData.Push(VegetationVertex::Construct(vertices[i]));
            }
        }
    }

    // Flush data to buffers
    vertexBuffer->SetSize(vertexData.Size(), VegetationVertex::Format());
    vertexBuffer->SetData(vertexData.Buffer());
    indexBuffer->SetSize(indexData.Size(), false);
    indexBuffer->SetData(indexData.Buffer());

    // Setup ranges
    unsigned group = 0;
    for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
    {
        for (unsigned lod = 0; lod < lodInfos.Size(); ++lod)
        {
            model->GetGeometry(materialIndex, lod)->SetDrawRange(TRIANGLE_LIST, groupOffsets[group], groupSizes[group]);
            ++group;
        }
    }

    // Setup other things
    model->SetBoundingBox(boundingBox);

    return model;
}

void TreeFactory::SetTrunkStrength(const float trunkStrength)
{
    trunkStrength_ = trunkStrength;
}

//////////////////////////////////////////////////////////////////////////
void TreeFactory::SetMaterials(const Vector<String>& materials)
{
    materials_ = materials;
}

int TreeFactory::GetMaterialIndex(const String& name) const
{
    Vector<String>::ConstIterator iter = materials_.Find(name);
    return iter == materials_.End()
        ? -1
        : iter - materials_.Begin();
}

unsigned TreeFactory::GetNumMaterials() const
{
    return materials_.Size();
}

//////////////////////////////////////////////////////////////////////////
namespace
{

/// Get materials from XML element to vector.
Vector<String> GetMaterials(const XMLElement& elem)
{
    Vector<String> result;
    for (XMLElement material = elem.GetChild("material"); material; material = material.GetNext("material"))
    {
        const String value = material.GetValue();
        if (result.Contains(value))
        {
            URHO3D_LOGERRORF("Duplicate material entry '%s' is ignored", value.CString());
            continue;
        }
        result += value;
    }
    return result;
}

void SetBranchGroupParameters(TreeFactory::BranchGroupParam& dest, XMLElement source)
{
    dest.material_.materialName_ = source.GetChild("material").GetAttribute("name");
    dest.material_.textureScale_ = source.GetChild("material").GetVector2("scale");

    LoadValue(source.GetChild("seed"), dest.seed_);
    LoadValue(source.GetChild("frequency"), dest.frequency_);
    LoadValue(source.GetChild("geometry_multiplier"), dest.geometryMultiplier_);

    LoadValue(source.GetChild("position"), dest.position_);
    LoadValue(source.GetChild("direction"), dest.direction_);

    LoadValue(source.GetChild("distrib").GetChild("type"), dest.distrib.type_);
    switch (dest.distrib.type_)
    {
    case TreeFactory::DistributionType::Alternate:
        LoadValue(source.GetChild("distrib").GetChild("angleStep"), dest.distrib.alternateParam_.angleStep_);
        LoadValue(source.GetChild("distrib").GetChild("angleRandom"), dest.distrib.alternateParam_.angleRandomness_);
        break;
    default:
        break;
    }

    LoadValue(source.GetChild("distrib_function"), dest.distribFunction);
    LoadValue(source.GetChild("location").GetChild("from"), dest.location.x_);
    LoadValue(source.GetChild("location").GetChild("to"), dest.location.y_);

    LoadValue(source.GetChild("length"), dest.length);
    LoadValue(source.GetChild("radius"), dest.radius);
    LoadValue(source.GetChild("angle"), dest.angle);
    LoadValue(source.GetChild("weight"), dest.weight);

    LoadValue(source.GetChild("branch_adherence"), dest.branchAdherence);
    LoadValue(source.GetChild("phase_random"), dest.phaseRandom);
}

void SetFrondGroupParameters(TreeFactory::FrondGroup& dest, XMLElement source)
{
    XMLElement materialNode = source.GetChild("material");
    LoadAttribute(materialNode, "name", dest.material_.materialName_);
    LoadAttribute(materialNode, "scale_x", dest.material_.textureScale_.x_);
    LoadAttribute(materialNode, "scale_y", dest.material_.textureScale_.y_);

    LoadAttribute(source, "type", dest.type_);
    LoadAttribute(source, "size", dest.size_);

    switch (dest.type_)
    {
    case TreeFactory::FrondType::Simple:
        LoadAttribute(source, "adjust",     dest.param_.adjustUpToGlobal_);
        LoadAttribute(source, "yaw",        dest.param_.yaw_);
        LoadAttribute(source, "pitch",      dest.param_.pitch_);
        LoadAttribute(source, "roll",       dest.param_.roll_);
        LoadAttribute(source, "offset",     dest.param_.junctionOffset_);
        LoadAttribute(source, "gravity",    dest.param_.gravityIntensity_);
        LoadAttribute(source, "resistance", dest.param_.gravityResistance_);
        break;
    case TreeFactory::FrondType::RagEnding:
        LoadValue(source.GetChild("location_offset"), dest.ragEndingParam_.locationOffset_);
        LoadValue(source.GetChild("center_offset"), dest.ragEndingParam_.centerOffset_);
        LoadValue(source.GetChild("z_bending"), dest.ragEndingParam_.zBendingFactor_);
        LoadValue(source.GetChild("x_bending"), dest.ragEndingParam_.xBendingFactor_);
        LoadValue(source.GetChild("junction_oscillation"), dest.ragEndingParam_.junctionOscillation_);
        LoadValue(source.GetChild("edge_oscillation"), dest.ragEndingParam_.edgeOscillation_);
        break;
    case TreeFactory::FrondType::BrushEnding:
        LoadValue(source.GetChild("location_offset"), dest.brushEndingParam_.locationOffset_);
        LoadValue(source.GetChild("center_offset"), dest.brushEndingParam_.centerOffset_);
        LoadValue(source.GetChild("spread_angle"), dest.brushEndingParam_.spreadAngle_);
        LoadValue(source.GetChild("junction_oscillation"), dest.brushEndingParam_.junctionOscillation_);
        LoadValue(source.GetChild("edge_oscillation"), dest.brushEndingParam_.edgeOscillation_);
        break;
    default:
        break;
    }
}

void AddFrondGroup(TreeFactory& factory, XMLElement frondGroup, TreeFactory::BranchGroup& parent)
{
    // Create new group
    TreeFactory::FrondGroup& group = parent.addFrondGroup();

    SetFrondGroupParameters(group, frondGroup);
    group.material_.materialIndex_ = factory.GetMaterialIndex(group.material_.materialName_);
    if (group.material_.materialIndex_ < 0)
    {
        URHO3D_LOGERRORF("Unknown material '%s'", group.material_.materialName_.CString());
    }
}

void AddBranchGroup(TreeFactory& factory, XMLElement branchGroup, TreeFactory::BranchGroup& parent)
{
    // Create new group
    TreeFactory::BranchGroup& group = parent.addBranchGroup();
    XMLElement description = branchGroup.GetChild("desc");

    // Load parameters
    SetBranchGroupParameters(group.p, description);
    group.p.material_.materialIndex_ = factory.GetMaterialIndex(group.p.material_.materialName_);
    if (group.p.material_.materialIndex_ < 0)
    {
        URHO3D_LOGERRORF("Unknown material '%s'", group.p.material_.materialName_.CString());
    }

    // Process children branch groups
    for (XMLElement child = branchGroup.GetChild("branches"); child; child = child.GetNext("branches"))
    {
        AddBranchGroup(factory, child, group);
    }

    // Process children frond groups
    for (XMLElement child = branchGroup.GetChild("fronds"); child; child = child.GetNext("fronds"))
    {
        AddFrondGroup(factory, child, group);
    }
}

}

void InitializeTreeFactory(TreeFactory& factory, XMLElement& node)
{
    // Read parameters
    factory.SetMaterials(GetMaterials(node.GetChild("materials")));
    factory.SetTrunkStrength(GetValue(node.GetChild("trunkStrength"), 0.5f));

    // Read branches
    for (XMLElement child = node.GetChild("branches"); child; child = child.GetNext("branches"))
    {
        AddBranchGroup(factory, child, factory.GetRoot());
    }
}

void GenerateTreeFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    // Load output node
    const XMLElement outputNode = node.GetChild("output");
    if (!outputNode)
    {
        URHO3D_LOGERROR("Tree must have <output> node");
        return;
    }

    // Load output model name
    const String outputModelName = factoryContext.ExpandName(outputNode.GetChild("model").GetValue());
    if (outputModelName.Empty())
    {
        URHO3D_LOGERROR("Tree output file name mustn't be empty");
        return;
    }

    // Skip generation if possible
    const bool alreadyGenerated = !!resourceCache.GetFile(outputModelName);
    if (!factoryContext.forceGeneration_ && alreadyGenerated)
    {
        return;
    }

    // Load LODs
    PODVector<TreeFactory::LodInfo> lodInfos;
    for (XMLElement lodNode = outputNode.GetChild("lod"); lodNode; lodNode = lodNode.GetNext("lod"))
    {
        TreeFactory::LodInfo lodInfo;
        lodInfo.numLengthSegments_ = 5;
        lodInfo.numRadialSegments_ = 5;

        LoadValue(lodNode.GetChild("numLengthSegments"), lodInfo.numLengthSegments_);
        LoadValue(lodNode.GetChild("numRadialSegments"), lodInfo.numRadialSegments_);

        if (lodInfo.numLengthSegments_ < 1)
        {
            URHO3D_LOGERROR("Tree must have at least one length segment");
            return;
        }
        if (lodInfo.numRadialSegments_ < 3)
        {
            URHO3D_LOGERROR("Tree must have at least three radial segments");
            return;
        }

        lodInfos.Push(lodInfo);
    }

    if (lodInfos.Empty())
    {
        URHO3D_LOGERROR("Tree must have at least one level of detail");
        return;
    }

    // Generate
    TreeFactory factory(resourceCache.GetContext());
    InitializeTreeFactory(factory, node);
    SharedPtr<Model> model = factory.GenerateModel(lodInfos);

    // Save
    const String& outputFileName = factoryContext.outputDirectory_ + outputModelName;
    CreateDirectoriesToFile(resourceCache, outputFileName);
    File outputFile(resourceCache.GetContext(), outputFileName, FILE_WRITE);
    if (outputFile.IsOpen())
    {
        model->Save(outputFile);
        outputFile.Close();

        // Reload
        resourceCache.ReloadResourceWithDependencies(outputModelName);
    }
    else
    {
        URHO3D_LOGERRORF("Cannot save tree model to '%s'", outputFileName.CString());
    }
}

// String readers
template <>
TreeFactory::FrondType To<TreeFactory::FrondType>(const String& str)
{
    const static HashMap<String, TreeFactory::FrondType> m =
    {
        MakePair(String("Simple"),      TreeFactory::FrondType::Simple),
        MakePair(String("RagEnding"),   TreeFactory::FrondType::RagEnding),
        MakePair(String("BrushEnding"), TreeFactory::FrondType::BrushEnding),
    };
    return *m[str];
}

template <>
TreeFactory::DistributionType To<TreeFactory::DistributionType>(const String& str)
{
    const static HashMap<String, TreeFactory::DistributionType> m =
    {
        MakePair(String("Alternate"), TreeFactory::DistributionType::Alternate),
    };
    return *m[str];
}

}
