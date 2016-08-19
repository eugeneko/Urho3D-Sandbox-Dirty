#include <FlexEngine/Factory/TreeFactory.h>

#include <FlexEngine/Container/Utility.h>
#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Factory/GeometryUtils.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Math/BezierCurve.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/StandardRandom.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>
#include <FlexEngine/Resource/XMLHelpers.h>

#include <Urho3D/Container/Pair.h>
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

namespace
{

/// Get orientation by direction.
Quaternion MakeBasisFromDirection(const Vector3& zAxis)
{
    Vector3 xAxis = ConstructOrthogonalVector(zAxis).Normalized();
    Vector3 yAxis = CrossProduct(zAxis, xAxis).Normalized();

    // Flip
    if (yAxis.y_ < 0.0f)
    {
        yAxis = -yAxis;
        xAxis = CrossProduct(yAxis, zAxis).Normalized();
    }

    return Quaternion(xAxis, yAxis, zAxis).Normalized();
}

/// Compute location and angle of child branch with specified distribution.
PODVector<float> ComputeChildLocations(const TreeElementDistribution& distribution, StandardRandom& random, unsigned count)
{
    PODVector<float> result;
    switch (distribution.distributionType_)
    {
    case TreeElementDistributionType::Alternate:
    {
        const PODVector<float> locations = IntegrateDensityFunction(distribution.density_, count);
        for (unsigned i = 0; i < count; ++i)
        {
            result.Push(distribution.location_.Get(locations[i]));
        }
        break;
    }
    default:
        break;
    }
    return result;
}

/// Compute location and angle of child branch with specified distribution.
PODVector<float> ComputeChildAngles(const TreeElementDistribution& distribution, StandardRandom& random, unsigned count, unsigned idx)
{
    PODVector<float> result;
    switch (distribution.distributionType_)
    {
    case TreeElementDistributionType::Alternate:
    {
        // This hack is used to make alternate distribution symmetric
        const float baseAngle = idx % 2 ? 180.0f : 0.0f;
        for (unsigned i = 0; i < count; ++i)
        {
            const float randomPad = random.FloatFrom01() * distribution.twirlNoise_;
            result.Push(distribution.twirlStep_ * i + distribution.twirlBase_ + randomPad + baseAngle);
        }
        break;
    }
    default:
        break;
    }
    return result;
}

/// Get material index.
int GetMaterialIndex(const Vector<String>& materials, const String& name)
{
    const Vector<String>::ConstIterator iter = materials.Find(name);
    return iter == materials.End()
        ? -1
        : iter - materials.Begin();
}

}

//////////////////////////////////////////////////////////////////////////
VegetationVertex VegetationVertex::Construct(const DefaultVertex& vertex)
{
    VegetationVertex result;
    result.position_ = vertex.position_;
    result.tangent_ = vertex.tangent_;
    result.binormal_ = vertex.binormal_;
    result.normal_ = vertex.normal_;
    result.geometryNormal_ = vertex.geometryNormal_;
    result.uv_.x_ = vertex.uv_[0].x_;
    result.uv_.y_ = vertex.uv_[0].y_;
    result.param_.x_ = vertex.uv_[1].x_;
    result.param_.y_ = vertex.uv_[1].y_;
    result.param_.z_ = vertex.uv_[1].z_;
    result.param_.w_ = vertex.uv_[1].w_;
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

//////////////////////////////////////////////////////////////////////////
BranchDescription GenerateBranch(const Vector3& initialPosition, const Vector3& initialDirection, float length, float baseRadius,
    const BranchShapeSettings& shape, unsigned minNumKnots)
{
    // Compute parameters
    const unsigned numKnots = minNumKnots;
    const float step = length / (numKnots - 1);

    // Generate knots
    BranchDescription result;

    Vector3 position = initialPosition;
    Vector3 direction = initialDirection;
    for (unsigned i = 0; i < numKnots; ++i)
    {
        result.positions_.Push(position);
        result.radiuses_.Push(shape.radius_.ComputeValue(i / (numKnots - 1.0f)) * baseRadius);

        direction = direction.Normalized();
        position += direction * step;
    }

    // Apply gravity and restore shape
    for (unsigned i = 0; i < numKnots; ++i)
    {
        const float factor = static_cast<float>(i) / (numKnots + 1);
        const float len = (result.positions_[i] - initialPosition).Length();
        result.positions_[i].y_ -= shape.gravityIntensity_ * Pow(factor, 1.0f / shape.gravityResistance_);
        result.positions_[i] = (result.positions_[i] - initialPosition).Normalized() * len + initialPosition;
    }

    result.length_ = length;
    result.radiusesCurve_ = CreateBezierCurve(result.radiuses_);
    result.positionsCurve_ = CreateBezierCurve(result.positions_);
    return result;
}

TessellatedBranchPoints TessellateBranch(const BezierCurve3D& positions, const BezierCurve1D& radiuses,
    const BranchTessellationQualityParameters& quality)
{
    TessellatedBranchPoints result;
    if (quality.maxNumSegments_ < 5)
    {
        URHO3D_LOGERROR("Maximum number of segments must be greater or equal than 5");
        return result;
    }

    if (quality.minNumSegments_ < 1)
    {
        URHO3D_LOGERROR("Minimum number of segments must be greater or equal than 1");
        return result;
    }

    if (quality.minNumSegments_ > quality.maxNumSegments_)
    {
        URHO3D_LOGERROR("Minimum number of segments must be less or equal to maximum number of segments");
        return result;
    }

    const unsigned numSegments = positions.xcoef_.Size();
    // Point is forcedly committed after specified number of skipped points
    const unsigned maxNumSkipped = (quality.maxNumSegments_ + quality.minNumSegments_ - 1) / quality.minNumSegments_ - 1;

    Vector3 prevDirection;
    unsigned prevIndex = 0;
    for (unsigned i = 0; i <= quality.maxNumSegments_; ++i)
    {
        const float t = static_cast<float>(i) / quality.maxNumSegments_;
        const Vector3 curDirection = SampleBezierCurveDerivative(positions, t);
        if (i == 0 || i == quality.maxNumSegments_ || i - prevIndex == maxNumSkipped
            || AngleBetween(curDirection, prevDirection) >= quality.minAngle_)
        {
            prevIndex = i;
            prevDirection = SampleBezierCurveDerivative(positions, t);
            const float radius = SampleBezierCurve(radiuses, t);
            const Vector3 position = SampleBezierCurve(positions, t);
            result.Push({ t, radius, position });
        }
    }
    return result;
}

PODVector<DefaultVertex> GenerateBranchVertices(const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments)
{
    PODVector<DefaultVertex> result;
    if (points.Empty())
    {
        URHO3D_LOGERROR("Points array must not be empty");
        return result;
    }
    if (numRadialSegments < 3)
    {
        URHO3D_LOGERROR("Number of segments must be greater or equal than 3");
        return result;
    }

    // Emit vertices
    float prevRadius = 0.0f;
    Vector3 prevPosition = Vector3::ZERO;
    float relativeDistance = 0.0f;

    for (unsigned i = 0; i < points.Size(); ++i)
    {
        const float location = points[i].location_ / (points.Size() - 1);
        const float radius = points[i].radius_;

        // Compute point position and orientation
        const Vector3 position = points[i].position_;
        const Vector3 zAxis = (points[Min(i + 1, points.Size() - 1)].position_ - points[i == 0 ? 0 : i - 1].position_).Normalized();
        const Quaternion basis = MakeBasisFromDirection(zAxis);
        const Vector3 xAxis = GetBasisX(basis.RotationMatrix());
        const Vector3 yAxis = GetBasisY(basis.RotationMatrix());

        // Compute relative distance
        if (i > 0)
        {
            ///                L        ln(r1/r0)
            /// u1 = u0 + ----------- * ---------
            ///           2 * pi * r0   r1/r0 - 1
            const float L = (position - prevPosition).Length();
            const float r0 = prevRadius;
            const float r1 = radius;
            const float rr = r1 / r0;
            /// ln(x) / (x - 1) ~= 1 - (x - 1) / 2 + ...
            const float k = 1 - (rr - 1) / 2;
            relativeDistance += k * L / (2 * M_PI * r0);
        }

        for (unsigned j = 0; j <= numRadialSegments; ++j)
        {
            const float factor = static_cast<float>(j) / numRadialSegments;
            const float angle = factor * 360;
            const Vector3 normal = (xAxis * Cos(angle) + yAxis * Sin(angle)).Normalized();

            DefaultVertex vertex;
            vertex.position_ = position + radius * normal;
            vertex.geometryNormal_ = normal;
            vertex.normal_ = normal;
            vertex.tangent_ = zAxis;
            vertex.binormal_ = CrossProduct(vertex.normal_, vertex.tangent_);
            vertex.uv_[0] = Vector4(factor / shape.textureScale_.x_, relativeDistance / shape.textureScale_.y_, 0, 0);
//             vertex.branchAdherence_ = param.windAdherence_;
//             vertex.phase_ = param.windPhase_;
//             vertex.edgeOscillation_ = 0.0f;

            result.Push(vertex);
        }

        // Update previous values
        prevPosition = position;
        prevRadius = radius;
    }

    return result;
}

PODVector<unsigned> GenerateBranchIndices(const PODVector<unsigned>& numRadialSegments, unsigned maxVertices)
{
    PODVector<unsigned> result;
    if (numRadialSegments.Empty())
    {
        URHO3D_LOGERROR("Points array must not be empty");
        return result;
    }

    // Compute indices
    unsigned baseVertex = 0;
    for (unsigned i = 0; i < numRadialSegments.Size() - 1; ++i)
    {
        const unsigned numA = numRadialSegments[i];
        const unsigned numB = numRadialSegments[i + 1];
        const unsigned baseVertexA = baseVertex;
        const unsigned baseVertexB = baseVertex + numA + 1;
        baseVertex = baseVertexB;

        assert(baseVertexA + numA <= maxVertices);
        assert(baseVertexB + numB <= maxVertices);

        unsigned idxA = 0;
        unsigned idxB = 0;
        const unsigned numTriangles = numA + numB;
        for (unsigned j = 0; j < numTriangles; ++j)
        {
            if (idxA * numB <= idxB * numA)
            {
                // A-based triangle
                result.Push(baseVertexA + idxA % (numA + 1));
                result.Push(baseVertexA + (idxA + 1) % (numA + 1));
                result.Push(baseVertexB + idxB % (numB + 1));
                ++idxA;
            }
            else
            {
                // B-based triangle
                result.Push(baseVertexB + (idxB + 1) % (numB + 1));
                result.Push(baseVertexB + idxB % (numB + 1));
                result.Push(baseVertexA + idxA % (numA + 1));
                ++idxB;
            }
        }
    }

    return result;
}

void GenerateBranchGeometry(ModelFactory& factory, const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments)
{
    numRadialSegments = Max(3u, static_cast<unsigned>(numRadialSegments * shape.quality_));
    const PODVector<DefaultVertex> tempVertices =
        GenerateBranchVertices(points, shape, numRadialSegments);
    const PODVector<unsigned> tempIndices =
        GenerateBranchIndices(ConstructPODVector(points.Size(), numRadialSegments), tempVertices.Size());
    factory.Push(tempVertices, tempIndices, true);
}

PODVector<TreeElementLocation> DistributeElementsOverParent(const BranchDescription& parent, const TreeElementDistribution& distrib)
{
    const unsigned seed = distrib.seed_ != 0
        ? distrib.seed_
        : MakeHash(distrib.position_);
    StandardRandom random(seed);

    PODVector<TreeElementLocation> result;
    if (distrib.frequency_ == 0)
    {
        TreeElementLocation elem;
        elem.seed_ = seed;
        elem.location_ = 0.5f;
        elem.position_ = distrib.position_;
        elem.rotation_ = MakeBasisFromDirection(distrib.direction_);
        elem.baseRadius_ = 1.0f;
        elem.noise_ = random.Vector4From01();
        result.Push(elem);
    }
    else
    {
        const PODVector<float> locations = ComputeChildLocations(distrib, random, distrib.frequency_);
        const PODVector<float> twirlAngles = ComputeChildAngles(distrib, random, distrib.frequency_, parent.index_);
        for (unsigned i = 0; i < distrib.frequency_; ++i)
        {
            // Find location
            const float location = locations[i];
            const float twirlAngle = twirlAngles[i];
            const float leanAngle = distrib.growthAngle_.ComputeValue(location);

            // Compute position
            const Vector3 position = SampleBezierCurve(parent.positionsCurve_, location);

            // Compute basis for parent branch
            const Vector3 zAxis = SampleBezierCurveDerivative(parent.positionsCurve_, location).Normalized();
            const Quaternion basis = MakeBasisFromDirection(zAxis);
            const Vector3 xAxis = GetBasisX(basis.RotationMatrix()).Normalized();
            const Vector3 yAxis = GetBasisY(basis.RotationMatrix()).Normalized();

            // Compute base direction
            const Vector3 baseDirection = (xAxis * Cos(twirlAngle) + yAxis * Sin(twirlAngle)).Normalized();
            const Vector3 direction = (Cos(90 - leanAngle) * zAxis + Sin(90 - leanAngle) * baseDirection).Normalized();

            // Generate branch
            TreeElementLocation elem;
            elem.seed_ = random.Random();
            elem.location_ = location;
            elem.position_ = position;
            elem.rotation_ = MakeBasisFromDirection(direction);
            elem.baseRadius_ = SampleBezierCurve(parent.radiusesCurve_, location);
            elem.noise_ = random.Vector4From01();
            result.Push(elem);
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
PODVector<float> IntegrateDensityFunction(const CubicCurveWrapper& density, unsigned count)
{
    PODVector<float> result;

    // Integrate over density
    float valueSum = 0.0f;
    for (unsigned i = 0; i < count; ++i)
    {
        const float value = density.ComputeValue((i + 0.5f) / count);
        result.Push(valueSum + value / 2);
        valueSum += value;
    }

    if (valueSum < M_LARGE_EPSILON && !result.Empty())
    {
        URHO3D_LOGWARNING("Density function mustn't be zero");
        return result;
    }

    // Normalize values
    for (float& value : result)
    {
        value /= valueSum;
    }
    return result;
}

Vector<BranchDescription> InstantiateBranchGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const BranchShapeSettings& shape, unsigned minNumKnots)
{
    const PODVector<TreeElementLocation> elements = DistributeElementsOverParent(parent, distribution);
    const FloatRange lengthRange(shape.length_.x_, Max(shape.length_.x_, shape.length_.y_));
    const float parentLength = distribution.frequency_ == 0 || !shape.relativeLength_ ? 1.0f : parent.length_;

    Vector<BranchDescription> result;
    for (unsigned i = 0; i < elements.Size(); ++i)
    {
        const TreeElementLocation& element = elements[i];
        const Vector3 position = element.position_;
        const Vector3 direction = GetBasisZ(element.rotation_.RotationMatrix());
        const float length = parentLength * // #TODO Move this code to DistributeElementsOverParent
            lengthRange.Get(element.noise_.w_) * distribution.growthScale_.ComputeValue(element.location_);
        BranchDescription branch = GenerateBranch(
            position, direction, length, element.baseRadius_, shape, minNumKnots);
        branch.index_ = i;
        result.Push(branch);
    }


    // Create fake ending branch
    if (shape.fakeEnding_ && distribution.frequency_ != 0)
    {
        BranchDescription branch;

        // Compute length of fake branch
        const float location = elements.Empty() ? 0.0f : elements.Back().location_;
        branch.index_ = elements.Size();
        branch.length_ = parentLength * (1.0f - location);

        // Compute number of knots
        const unsigned numKnots = Max(minNumKnots, static_cast<unsigned>((1.0f - location) * branch.positions_.Size()));

        // Generate knots
        for (unsigned i = 0; i < numKnots; ++i)
        {
            const float t = Lerp(location, 1.0f, i / (numKnots - 1.0f));
            branch.positions_.Push(SampleBezierCurve(parent.positionsCurve_, t));
            branch.radiuses_.Push(SampleBezierCurve(parent.radiusesCurve_, t));
        }

        branch.positionsCurve_ = CreateBezierCurve(branch.positions_);
        branch.radiusesCurve_ = CreateBezierCurve(branch.radiuses_);

        branch.fake_ = true;
        result.Push(branch);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
PODVector<LeafDescription> InstantiateLeafGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const LeafShapeSettings& shape)
{
    const PODVector<TreeElementLocation> elements = DistributeElementsOverParent(parent, distribution);

    PODVector<LeafDescription> result;
    for (const TreeElementLocation& element : elements)
    {
        LeafDescription leaf;
        leaf.location_ = element;
        result.Push(leaf);
    }
    return result;
}

void GenerateLeafGeometry(ModelFactory& factory,
    const LeafShapeSettings& shape, const TreeElementLocation& location, const Vector3& foliageCenter)
{
    // Instantiate random values
    const float adjustToGlobal = shape.adjustToGlobal_.Get(location.noise_.x_);
    const float alignVerical = shape.alignVertical_.Get(location.noise_.y_);
    const Vector3 position = location.position_;
    const Quaternion localRotation = location.rotation_;

    // Compute global axises
    const Vector3 xAxisGlobal = CrossProduct(Vector3::UP, GetBasisZ(localRotation.RotationMatrix())).Normalized();
    const Vector3 zAxisGlobal = CrossProduct(xAxisGlobal, Vector3::UP).Normalized();
    const Vector3 yAxisGlobal = CrossProduct(zAxisGlobal, xAxisGlobal).Normalized();

    // Skip such cases
    if (Equals(xAxisGlobal.LengthSquared(), 0.0f)
        || Equals(yAxisGlobal.LengthSquared(), 0.0f)
        || Equals(zAxisGlobal.LengthSquared(), 0.0f))
    {
        return;
    }

    // Adjust to global space
    const Quaternion globalRotation = Quaternion(xAxisGlobal, yAxisGlobal, zAxisGlobal).Normalized();
    const Quaternion baseRotation = localRotation.Slerp(globalRotation, adjustToGlobal).Normalized();

    const Quaternion verticalRotation = Quaternion(GetBasisZ(baseRotation.RotationMatrix()), Vector3::DOWN).Normalized();
    const Quaternion alignVerticalRotation = Quaternion::IDENTITY.Slerp(verticalRotation, alignVerical).Normalized();

    const Matrix3 rotationMatrix = (alignVerticalRotation * baseRotation).RotationMatrix();
    const Vector3 xAxis = GetBasisX(rotationMatrix);
    const Vector3 yAxis = GetBasisY(rotationMatrix);
    const Vector3 zAxis = GetBasisZ(rotationMatrix);

    // Compute normals
//     const float FACTOR = 0.5f;// #TODO move to settings and get name
//     const Vector3 baseNormal = (location.position_ * Vector3(1, 0, 1)).Normalized();
//     const Vector3 baseSide = ConstructOrthogonalVector(baseNormal).Normalized();
//     const Vector3 leftNormal = Lerp(baseNormal, baseSide)
//     const Vector3 upNormal = Vector3::UP;
//     const Vector3 downNormal = (baseNormal + Vector3::DOWN).Normalized();

    // #TODO Use custom geometry
    DefaultVertex vers[4];

    vers[0].position_ = Vector3(-0.5f, 0.0f, 0.0f);
//     vers[0].normal_ = upNormal;
    vers[0].uv_[0] = Vector4(0, 0, 0, 0);

    vers[1].position_ = Vector3(0.5f, 0.0f, 0.0f);
//     vers[0].normal_ = upNormal;
    vers[1].uv_[0] = Vector4(1, 0, 0, 0);

    vers[2].position_ = Vector3(-0.5f, 0.0f, 1.0f);
//     vers[2].normal_ = downNormal;
    vers[2].uv_[0] = Vector4(0, 1, 0, 0);

    vers[3].position_ = Vector3(0.5f, 0.0f, 1.0f);
//     vers[3].normal_ = downNormal;
    vers[3].uv_[0] = Vector4(1, 1, 0, 0);

    PODVector<DefaultVertex> newVertices;
    PODVector<unsigned> newIndices;
    AppendQuadGridToVertices(newVertices, newIndices, vers[0], vers[1], vers[2], vers[3], 2, 2);

    // Compute max gravity factor
    const Vector3 junctionAdherenceFactor = Vector3(1.0f, 0.0f, 1.0f);
    Vector3 maxFactor = Vector3::ONE * M_EPSILON;
    for (const DefaultVertex& vertex : newVertices)
    {
        maxFactor = VectorMax(maxFactor, vertex.position_.Abs());
    }

    // Apply gravity factor to position
    const Vector3 geometryScale = shape.scale_ * shape.size_.Get(location.noise_.z_);
    const Vector3 basePosition = position + rotationMatrix * shape.junctionOffset_;
    for (DefaultVertex& vertex : newVertices)
    {
        // Compute factor of junction adherence
        const Vector3 factor = vertex.position_.Abs() / maxFactor;

        // Compute position in global space
        vertex.position_ = basePosition + rotationMatrix * vertex.position_ * geometryScale;

        // Compute distance to junction
        const float len = (vertex.position_ - position).Length();

        // Apply gravity
        const Vector3 perComponentGravity = shape.gravityIntensity_ * Pow(factor, Vector3::ONE / shape.gravityResistance_);
        vertex.position_.y_ -= perComponentGravity.DotProduct(Vector3::ONE);

        // Restore shape
        vertex.position_ = (vertex.position_ - position).Normalized() * len + position;

        // Apply wind parameters
//         vertex.edgeOscillation_ = 0;
//         vertex.branchAdherence_ = location.branchAdherence;
//         vertex.phase_ = location.phase;
    }

    // Compute normals
    for (DefaultVertex& vertex : newVertices)
    {
        vertex.normal_ = (vertex.position_ - Lerp(foliageCenter, basePosition, shape.bumpNormals_)).Normalized();
    }

    factory.Push(newVertices, newIndices, true);
}
// 
// //////////////////////////////////////////////////////////////////////////
// SharedPtr<Model> TriangulateBranchesAndLeaves(Context* context, const Vector<TreeLodDescription>& lods,
//     const Vector<BranchDescription>& branches, const PODVector<LeafDescription>& leaves, unsigned numMaterials)
// {
//     // Prepare buffers for accumulated geometry data
//     SharedPtr<VertexBuffer> vertexBuffer = MakeShared<VertexBuffer>(context);
//     vertexBuffer->SetShadowed(true);
//     SharedPtr<IndexBuffer> indexBuffer = MakeShared<IndexBuffer>(context);
//     indexBuffer->SetShadowed(true);
//     SharedPtr<Model> model = MakeShared<Model>(context);
//     model->SetVertexBuffers({ vertexBuffer }, { 0 }, { 0 });
//     model->SetIndexBuffers({ indexBuffer });
// 
//     // Number of geometries is equal to number of materials
//     model->SetNumGeometries(numMaterials);
// 
//     const unsigned numLods = lods.Size();
//     for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
//     {
//         model->SetNumGeometryLodLevels(materialIndex, numLods);
//     }
// 
//     PODVector<VegetationVertex> vertexData;
//     PODVector<unsigned short> indexData;
//     PODVector<unsigned> groupOffsets;
//     PODVector<unsigned> groupSizes;
// 
//     // Generate data for each material and LOD
//     BoundingBox boundingBox;
//     unsigned indexOffset = 0;
//     for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
//     {
//         for (unsigned lod = 0; lod < lods.Size(); ++lod)
//         {
//             const TreeLodDescription& lodInfo = lods[lod];
// 
//             PODVector<FatVertex> vertices;
//             PODVector<FatIndex> indices;
//             for (const BranchDescription& branch : branches)
//             {
//                 if (branch.material_.materialId_ != static_cast<int>(materialIndex))
//                 {
//                     continue;
//                 }
//                 const TessellatedBranchPoints tessellatedPoints =
//                     TessellateBranch(branch.curve_, branch.radiuses_, lodInfo.branchTessellationQuality_);
//                 const PODVector<FatVertex> tempVertices =
//                     GenerateBranchVertices(tessellatedPoints, branch.shape_, branch.material_, 5);
//                 const PODVector<FatIndex> tempIndices =
//                     GenerateBranchIndices(ConstructPODVector(tessellatedPoints.Size(), 5u), tempVertices.Size());
//                 AppendGeometryToVertices(vertices, indices, tempVertices, tempIndices);
//             }
// 
//             for (const LeafDescription& leaf : leaves)
//             {
// //                 if (branch.material_.materialId_ != static_cast<int>(materialIndex))
// //                 {
// //                     continue;
// //                 }
//                 PODVector<FatVertex> tempVertices;
//                 PODVector<FatIndex> tempIndices;
//                 GenerateLeafGeometry(leaf.shape_, leaf.location_, tempVertices, tempIndices);
//                 AppendGeometryToVertices(vertices, indices, tempVertices, tempIndices);
//             }
// 
//             // Compute AABB and radius
//             const BoundingBox aabb = CalculateBoundingBox(vertices.Buffer(), vertices.Size());
//             const Vector3 aabbCenter = static_cast<Vector3>(aabb.Center());
// //             modelRadius_ = 0.0f;
// //             for (const FatVertex& vertex : vertices)
// //             {
// //                 const float dist = ((vertex.position_ - aabbCenter) * Vector3(1, 0, 1)).Length();
// //                 modelRadius_ = Max(modelRadius_, dist);
// //             }
//             boundingBox.Merge(aabb);
// 
//             SharedPtr<Geometry> geometry = MakeShared<Geometry>(context);
//             geometry->SetVertexBuffer(0, vertexBuffer);
//             geometry->SetIndexBuffer(indexBuffer);
//             model->SetGeometry(materialIndex, lod, geometry);
//             groupOffsets.Push(indexOffset);
//             groupSizes.Push(indices.Size());
//             indexOffset += indices.Size();
// 
//             unsigned vertexOffset = vertexData.Size();
//             for (unsigned i = 0; i < indices.Size(); ++i)
//             {
//                 indexData.Push(static_cast<unsigned short>(indices[i] + vertexOffset));
//             }
// 
//             for (unsigned i = 0; i < vertices.Size(); ++i)
//             {
//                 vertexData.Push(VegetationVertex::Construct(vertices[i]));
//             }
//         }
//     }
// 
//     // Flush data to buffers
//     vertexBuffer->SetSize(vertexData.Size(), VegetationVertex::Format());
//     vertexBuffer->SetData(vertexData.Buffer());
//     indexBuffer->SetSize(indexData.Size(), false);
//     indexBuffer->SetData(indexData.Buffer());
// 
//     // Setup ranges
//     unsigned group = 0;
//     for (unsigned materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
//     {
//         for (unsigned lod = 0; lod < lods.Size(); ++lod)
//         {
//             model->GetGeometry(materialIndex, lod)->SetDrawRange(TRIANGLE_LIST, groupOffsets[group], groupSizes[group]);
//             ++group;
//         }
//     }
// 
//     // Setup other things
//     model->SetBoundingBox(boundingBox);
// 
//     return model;
// }




// 
// 
// BranchGroupDescription ReadBranchGroupDescriptionFromXML(const XMLElement& element, const Vector<String>& materials)
// {
//     BranchGroupDescription result;
// 
//     // Load distribution
// //     LoadAttributeOrChild(element, "seed", result.distribution_.seed_);
// //     LoadAttributeOrChild(element, "frequency", result.distribution_.frequency_);
// //     LoadAttributeOrChild(element, "quality", result.distribution_.quality_);
// // 
// //     LoadAttributeOrChild(element, "position", result.distribution_.position_);
// //     LoadAttributeOrChild(element, "direction", result.distribution_.direction_);
// // 
// //     LoadAttributeOrChild(element, "disributionType", result.distribution_.distributionType_);
// //     LoadAttributeOrChild(element, "location", result.distribution_.location_);
// //     LoadAttributeOrChild(element, "density", result.distribution_.density_);
// //     LoadAttributeOrChild(element, "alternateStep", result.distribution_.twirlStep_);
// //     LoadAttributeOrChild(element, "randomOffset", result.distribution_.twirlNoise_);
// //     LoadAttributeOrChild(element, "twirl", result.distribution_.twirlBase_);
// //     LoadAttributeOrChild(element, "growthScale", result.distribution_.growthScale_);
// //     LoadAttributeOrChild(element, "growthAngle", result.distribution_.growthAngle_);
// 
//     // Load material
// //     result.material_.materialId_ = GetMaterialIndex(materials, GetAttributeOrChild(element, "material", String::EMPTY));
//     LoadAttributeOrChild(element, "scale", result.material_.textureScale_);
// 
//     return result;
// }
// 
// BranchGroup ReadBranchGroupFromXML(const XMLElement& element, const Vector<String>& materials)
// {
//     BranchGroup result;
//     result.fake_ = element.GetName().Compare("root", false) == 0;
//     result.desc_ = result.fake_ ? BranchGroupDescription() : ReadBranchGroupDescriptionFromXML(element, materials);
//     for (XMLElement child = element.GetChild("branch"); child; child = child.GetNext("branch"))
//     {
//         result.children_.Push(ReadBranchGroupFromXML(child, materials));
//     }
//     return result;
// }
// 
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
//     void triangulateBranch(unsigned materialIndex, const Branch& branch, const LodInfo& lodInfo,
//                            PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         Vector<PPPoint> points;
//         if (branch.groupInfo.p.material_.materialIndex_ == materialIndex)
//         {
//             preprocessPoints(branch, lodInfo, points);
//             generateBranchGeometry(branch, points, vertices, indices);
//         }
// 
//         // Triangulate children
//         for (const Branch& child : branch.children)
//         {
//             triangulateBranch(materialIndex, child, lodInfo, vertices, indices);
//         }
//     }
//     void generateBranchGeometry(const Branch& branch, const Vector<PPPoint>& points,
//                                 PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
// //         BranchGeometryParameters branchGeometry;
// //         branchGeometry.baseRadius_ = branch.points[0].radius;
// //         branchGeometry.textureScale_ = Vector2(1, 4);
// // 
// //         BranchTessellationQualityParameters tessQuality;
// //         tessQuality.minAngle_ = 10.0f;
// //         tessQuality.minNumSegments_ = 3;
// //         tessQuality.maxNumSegments_ = 100;
// // 
// //         const TessellatedBranchPoints tessellatedPoints = TessellateBranch(branch.curve_, tessQuality);
// //         const PODVector<FatVertex> tempVertices = GenerateBranchVertices(tessellatedPoints, branchGeometry, 5);
// //         const PODVector<FatIndex> tempIndices = GenerateBranchIndices(ConstructPODVector(tessellatedPoints.Size(), 5u), tempVertices.Size());
// //         AppendGeometryToVertices(vertices, indices, tempVertices, tempIndices);
// // 
// //         return;
// 
//         const Vector2 textureScale = branch.groupInfo.p.material_.textureScale_;
//         const unsigned baseVertex = vertices.Size();
//         for (const PPPoint& point : points)
//         {
//             // Add points
//             for (unsigned idx = 0; idx <= point.numVertices; ++idx)
//             {
//                 const float factor = static_cast<float>(idx) / point.numVertices;
//                 const float angle = factor * 360;
//                 const Vector3 normal = (point.xAxis * Cos(angle) + point.yAxis * Sin(angle)).Normalized();
// 
//                 FatVertex vertex;
//                 vertex.position_ = point.position + point.radius * normal;
//                 vertex.geometryNormal_ = normal;
//                 vertex.normal_ = normal;
//                 vertex.tangent_ = point.zAxis;
//                 vertex.binormal_ = CrossProduct(vertex.normal_, vertex.tangent_);
//                 vertex.uv_[0] = Vector4(factor / textureScale.x_, point.relativeDistance / textureScale.y_, 0, 0);
//                 vertex.branchAdherence_ = point.branchAdherence;
//                 vertex.phase_ = point.phase;
//                 vertex.edgeOscillation_ = 0.0f;
// 
//                 vertices.Push(vertex);
//             }
//         }
// 
//         // Connect point circles
//         for (unsigned pointIndex = 0; pointIndex < points.Size() - 1; ++pointIndex)
//         {
//             const PPPoint& pointA = points[pointIndex];
//             const PPPoint& pointB = points[pointIndex + 1];
//             const unsigned numA = pointA.numVertices;
//             const unsigned numB = pointB.numVertices;
//             const unsigned baseVertexA = baseVertex + pointA.startVertex;
//             const unsigned baseVertexB = baseVertex + pointB.startVertex;
// 
//             unsigned idxA = 0;
//             unsigned idxB = 0;
//             const unsigned numTriangles = pointA.numVertices + pointB.numVertices;
//             for (unsigned triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
//             {
//                 if (idxA * numB <= idxB * numA)
//                 {
//                     // A-based triangle
//                     indices.Push(baseVertexA + idxA % (numA + 1));
//                     indices.Push(baseVertexA + (idxA + 1) % (numA + 1));
//                     indices.Push(baseVertexB + idxB % (numB + 1));
//                     ++idxA;
//                 }
//                 else
//                 {
//                     // B-based triangle
//                     indices.Push(baseVertexB + (idxB + 1) % (numB + 1));
//                     indices.Push(baseVertexB + idxB % (numB + 1));
//                     indices.Push(baseVertexA + idxA % (numA + 1));
//                     ++idxB;
//                 }
//             }
//         }
//     }
};
struct TreeFactory::InternalFrondTriangulator
{
//     void triangulateBranch(unsigned materialIndex, const Branch& branch, const LodInfo& lodInfo,
//                            PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         // Triangulate frond groups
//         for (const FrondGroup& group : branch.groupInfo.fronds())
//         {
//             triangulateFrondGroup(materialIndex, branch, group, lodInfo, vertices, indices);
//         }
// 
//         // Iterate over children
//         for (const Branch& child : branch.children)
//         {
//             triangulateBranch(materialIndex, child, lodInfo, vertices, indices);
//         }
//     }
//     void triangulateFrondGroup(unsigned materialIndex, const Branch& branch, const FrondGroup& frondGroup,
//                                const LodInfo& lodInfo,
//                                PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         if (frondGroup.material_.materialIndex_ != materialIndex)
//         {
//             return;
//         }
//         const float size = frondGroup.size_->Compute(m_random.FloatFrom01());
//         switch (frondGroup.type_)
//         {
//         case FrondType::Simple:
//             TriangulateFrond(branch, frondGroup, size, lodInfo, vertices, indices);
//             break;
//         case TreeFactory::FrondType::RagEnding:
//             triangulateRagEndingFrondGroup(branch, frondGroup, size, lodInfo, vertices, indices);
//             break;
//         case TreeFactory::FrondType::BrushEnding:
//             triangulateBrushEndingFrondGroup(branch, frondGroup, size, lodInfo, vertices, indices);
//             break;
//         default:
//             break;
//         }
//     }
//     /// @name Ending
//     /// @{
//     void TriangulateFrond(const Branch& branch, const FrondGroup& frondGroup,
//         const float size, const LodInfo& lodInfo,
//         PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         // Instantiate random values
//         const FrondParameters& param = frondGroup.param_;
//         const float adjustToGlobal = param.adjustUpToGlobal_;
//         const float yaw = param.yaw_;
//         const float pitch = param.pitch_;
//         const float roll = param.roll_;
//         const Point location = branch.getPoint(1.0f);
// 
//         // Compute global axises
//         const Quaternion localRotation(location.xAxis, location.yAxis, location.zAxis);
//         const Vector3 xAxisGlobal = CrossProduct(Vector3::UP, location.zAxis).Normalized();
//         const Vector3 zAxisGlobal = CrossProduct(xAxisGlobal, Vector3::UP).Normalized();
//         const Vector3 yAxisGlobal = CrossProduct(zAxisGlobal, xAxisGlobal).Normalized();
// 
//         // Skip such cases
//         if (Equals(xAxisGlobal.LengthSquared(), 0.0f)
//             || Equals(yAxisGlobal.LengthSquared(), 0.0f)
//             || Equals(zAxisGlobal.LengthSquared(), 0.0f))
//         {
//             return;
//         }
// 
//         // Apply rotations
//         const Quaternion globalRotation(xAxisGlobal, yAxisGlobal, zAxisGlobal);
//         const Quaternion baseRotation = localRotation.Slerp(globalRotation, adjustToGlobal);
//         const Quaternion yprRotation = Quaternion(yaw, Vector3::UP)
//             * Quaternion(pitch, Vector3::RIGHT) * Quaternion(roll, Vector3::FORWARD);
// 
//         const Matrix3 rotationMatrix = (baseRotation * yprRotation).RotationMatrix();
//         const Vector3 xAxis = Vector3(rotationMatrix.m00_, rotationMatrix.m10_, rotationMatrix.m20_);
//         const Vector3 yAxis = Vector3(rotationMatrix.m01_, rotationMatrix.m11_, rotationMatrix.m21_);
//         const Vector3 zAxis = Vector3(rotationMatrix.m02_, rotationMatrix.m12_, rotationMatrix.m22_);
// 
//         // #TODO Use custom geometry
//         FatVertex vers[4];
// 
//         vers[0].position_ = Vector3(-0.5f, 0.0f, 0.0f);
//         vers[0].uv_[0] = Vector4(0, 0, 0, 0);
// 
//         vers[1].position_ = Vector3(0.5f, 0.0f, 0.0f);
//         vers[1].uv_[0] = Vector4(1, 0, 0, 0);
// 
//         vers[2].position_ = Vector3(-0.5f, 0.0f, 1.0f);
//         vers[2].uv_[0] = Vector4(0, 1, 0, 0);
// 
//         vers[3].position_ = Vector3(0.5f, 0.0f, 1.0f);
//         vers[3].uv_[0] = Vector4(1, 1, 0, 0);
// 
//         PODVector<FatVertex> newVertices;
//         PODVector<FatIndex> newIndices;
//         AppendQuadGridToVertices(newVertices, newIndices, vers[0], vers[1], vers[2], vers[3], 2, 2);
// 
//         // Compute max factor
//         const Vector3 junctionAdherenceFactor = Vector3(1.0f, 0.0f, 1.0f);
//         Vector3 maxFactor = Vector3::ONE * M_EPSILON;
//         for (FatVertex& vertex : newVertices)
//         {
//             maxFactor = VectorMax(maxFactor, vertex.position_.Abs());
//         }
// 
//         // Finalize
//         const Vector3 geometryScale = Vector3(size, size, size);
//         const Vector3 basePosition = location.position + rotationMatrix * param.junctionOffset_;
//         for (FatVertex& vertex : newVertices)
//         {
//             // Compute factor of junction adherence
//             const Vector3 factor = vertex.position_.Abs() / maxFactor;
// 
//             // Compute position in global space
//             vertex.position_ = basePosition + rotationMatrix * vertex.position_ * geometryScale;
// 
//             // Compute distance to junction
//             const float len = (vertex.position_ - location.position).Length();
// 
//             // Apply gravity
//             const Vector3 perComponentGravity = param.gravityIntensity_ * Pow(factor, Vector3::ONE / param.gravityResistance_);
//             vertex.position_.y_ -= perComponentGravity.DotProduct(Vector3::ONE);
// 
//             // Restore shape
//             vertex.position_ = (vertex.position_ - location.position).Normalized() * len + location.position;
// 
//             // Apply wind parameters
//             vertex.edgeOscillation_ = 0;
//             vertex.branchAdherence_ = location.branchAdherence;
//             vertex.phase_ = location.phase;
//         }
// 
//         // Triangles
//         AppendGeometryToVertices(vertices, indices, newVertices, newIndices);
//     }
//     void triangulateRagEndingFrondGroup(const Branch& branch, const FrondGroup& frondGroup,
//                                         const float size, const LodInfo& lodInfo,
//                                         PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         const RagEndingFrondParameters& param = frondGroup.ragEndingParam_;
//         const float halfSize = size / 2;
//         const float relativeOffset = param.locationOffset_ / branch.totalLength;
//         const Point location = branch.getPoint(1.0f - relativeOffset);
//         const Vector3 zyAxis = Lerp(location.zAxis, -location.yAxis, param.zBendingFactor_).Normalized();
//         FatVertex vers[9];
// 
//         // Center
//         vers[4].position_ = location.position
//             + (location.zAxis + location.yAxis).Normalized() * param.centerOffset_;
//         vers[4].uv_[0] = Vector4(0.5f, 0.5f, 0, 0);
//         vers[4].edgeOscillation_ = param.junctionOscillation_;
// 
//         // Center -+z
//         vers[3].position_ = vers[4].position_ - halfSize * location.zAxis;
//         vers[3].uv_[0] = Vector4(0.5f, 0.0f, 0, 0);
//         vers[3].edgeOscillation_ = param.edgeOscillation_;
//         vers[5].position_ = vers[4].position_ + halfSize * zyAxis;
//         vers[5].uv_[0] = Vector4(0.5f, 1.0f, 0, 0);
//         vers[5].edgeOscillation_ = param.edgeOscillation_;
// 
//         // Center -+x
//         vers[1].position_ = vers[4].position_ - halfSize * Lerp(location.xAxis, location.yAxis, param.xBendingFactor_).Normalized();
//         vers[1].uv_[0] = Vector4(0.0f, 0.5f, 0, 0);
//         vers[1].edgeOscillation_ = param.edgeOscillation_;
//         vers[7].position_ = vers[4].position_ + halfSize * Lerp(location.xAxis, -location.yAxis, param.xBendingFactor_).Normalized();
//         vers[7].uv_[0] = Vector4(1.0f, 0.5f, 0, 0);
//         vers[7].edgeOscillation_ = param.edgeOscillation_;
// 
//         // Center -x -+z
//         vers[0].position_ = vers[1].position_ - halfSize * location.zAxis;
//         vers[0].uv_[0] = Vector4(0.0f, 0.0f, 0, 0);
//         vers[0].edgeOscillation_ = param.edgeOscillation_;
//         vers[2].position_ = vers[1].position_ + halfSize * zyAxis;
//         vers[2].uv_[0] = Vector4(0.0f, 1.0f, 0, 0);
//         vers[2].edgeOscillation_ = param.edgeOscillation_;
// 
//         // Center +x -+z
//         vers[6].position_ = vers[7].position_ - halfSize * location.zAxis;
//         vers[6].uv_[0] = Vector4(1.0f, 0.0f, 0, 0);
//         vers[6].edgeOscillation_ = param.edgeOscillation_;
//         vers[8].position_ = vers[7].position_ + halfSize * zyAxis;
//         vers[8].uv_[0] = Vector4(1.0f, 1.0f, 0, 0);
//         vers[8].edgeOscillation_ = param.edgeOscillation_;
// 
//         // Set wind
//         for (FatVertex& vertex : vers)
//         {
//             vertex.branchAdherence_ = location.branchAdherence;
//             vertex.phase_ = location.phase;
//         }
// 
//         // Triangles
//         const unsigned base = vertices.Size();
//         AppendQuadToIndices(indices, base, 0, 1, 3, 4, true);
//         AppendQuadToIndices(indices, base, 1, 2, 4, 5, true);
//         AppendQuadToIndices(indices, base, 3, 4, 6, 7, true);
//         AppendQuadToIndices(indices, base, 4, 5, 7, 8, true);
//         vertices.Insert(vertices.End(), vers, vers + 9);
//     }
//     void triangulateBrushEndingFrondGroup(const Branch& branch, const FrondGroup& frondGroup,
//                                           const float size, const LodInfo& lodInfo,
//                                           PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices)
//     {
//         const BrushEndingFrondParameters& param = frondGroup.brushEndingParam_;
//         const float halfSize = size / 2;
//         const float relativeOffset = param.locationOffset_ / branch.totalLength;
//         const float xyAngle = param.spreadAngle_ / 2;
//         const Point location = branch.getPoint(1.0f - relativeOffset);
// 
//         // Build volume geometry
//         static const unsigned NumQuads = 3;
//         for (unsigned idx = 0; idx < NumQuads; ++idx)
//         {
//             const float zAngle = 360.0f * idx / NumQuads;
//             const Vector3 position = location.position;
//             const Vector3 side = location.xAxis * Cos(zAngle) + location.yAxis * Sin(zAngle);
//             const Vector3 direction = Quaternion(xyAngle, side).RotationMatrix() * location.zAxis;
//             const Vector3 normal = CrossProduct(side, direction);
// 
//             // triangulate
//             FatVertex vers[5];
//             vers[0].position_ = position - halfSize * side;
//             vers[0].uv_[0] = Vector4(0.0f, 0.0f, 0, 0);
//             vers[0].edgeOscillation_ = param.junctionOscillation_;
//             vers[1].position_ = position + halfSize * side;
//             vers[1].uv_[0] = Vector4(1.0f, 0.0f, 0, 0);
//             vers[1].edgeOscillation_ = param.junctionOscillation_;
//             vers[2].position_ = position - halfSize * side + size * direction;
//             vers[2].uv_[0] = Vector4(0.0f, 1.0f, 0, 0);
//             vers[2].edgeOscillation_ = param.edgeOscillation_;
//             vers[3].position_ = position + halfSize * side + size * direction;
//             vers[3].uv_[0] = Vector4(1.0f, 1.0f, 0, 0);
//             vers[3].edgeOscillation_ = param.edgeOscillation_;
//             vers[4] = Lerp(vers[0], vers[3], 0.5f);
//             vers[4].position_ += normal * size * param.centerOffset_;
// 
//             // Set wind
//             for (FatVertex& vertex : vers)
//             {
//                 vertex.branchAdherence_ = location.branchAdherence;
//                 vertex.phase_ = location.phase;
//             }
// 
//             // put data to buffers
//             const unsigned base = vertices.Size();
//             if (param.centerOffset_ == 0)
//             {
//                 AppendQuadToIndices(indices, base, 0, 1, 2, 3, true);
//                 vertices.Insert(vertices.End(), vers, vers + 4);
//             }
//             else
//             {
//                 vertices.Insert(vertices.End(), vers, vers + 5);
//                 indices.Push(base + 0); indices.Push(base + 1); indices.Push(base + 4);
//                 indices.Push(base + 1); indices.Push(base + 3); indices.Push(base + 4);
//                 indices.Push(base + 3); indices.Push(base + 2); indices.Push(base + 4);
//                 indices.Push(base + 2); indices.Push(base + 0); indices.Push(base + 4);
//             }
//         }
//     }
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
// unsigned TreeFactory::triangulateBranches(unsigned materialIndex, const LodInfo& lodInfo,
//                                     const Vector<Branch>& trunksArray, 
//                                     PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices) const
// {
//     const unsigned startIndex = indices.Size();
//     InternalBranchTriangulator triangulator;
//     for (const Branch& branch : trunksArray)
//     {
//         triangulator.triangulateBranch(materialIndex, branch, lodInfo, vertices, indices);
//     }
//     const unsigned numTriangles = (indices.Size() - startIndex) / 3;
//     return numTriangles;
// }
// unsigned TreeFactory::triangulateFronds(unsigned materialIndex, const LodInfo& lodInfo,
//                                   const Vector<Branch>& trunksArray, 
//                                   PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices) const
// {
//     // Triangulate
//     const unsigned startIndex = indices.Size();
//     InternalFrondTriangulator triangulator;
//     for (const Branch& branch : trunksArray)
//     {
//         triangulator.triangulateBranch(materialIndex, branch, lodInfo, vertices, indices);
//     }
// 
//     // Generate TBN
//     const unsigned numTriangles = (indices.Size() - startIndex) / 3;
//     CalculateNormals(vertices.Buffer(), vertices.Size(), 
//         indices.Buffer() + startIndex, numTriangles);
//     CalculateTangents(vertices.Buffer(), vertices.Size(),
//         indices.Buffer() + startIndex, numTriangles);
// 
//     // Return number of triangles
//     return numTriangles;
// }


// Wind stuff
// float TreeFactory::computeMainAdherence(const float relativeHeight, const float trunkStrength)
// {
//     return pow(Max(relativeHeight, 0.0f),
//                1 / (1 - trunkStrength));
// }
// void TreeFactory::computeMainAdherence(PODVector<FatVertex>& vertices)
// {
//     // Compute max height
//     float maxHeight = 0.0f;
//     for (FatVertex& vertex : vertices)
//     {
//         maxHeight = Max(maxHeight, vertex.position_.y_);
//     }
// 
//     // Compute wind
//     auto compute = [this](const float relativeHeight) -> float
//     {
//         return computeMainAdherence(relativeHeight, trunkStrength_);
//     };
//     for (FatVertex& vertex : vertices)
//     {
//         vertex.mainAdherence_ = compute(vertex.position_.y_ / maxHeight);
//     }
// }
// void TreeFactory::normalizeBranchesAdherence(PODVector<FatVertex>& vertices)
// {
//     // Compute max adherence
//     float maxAdherence = M_LARGE_EPSILON;
//     for (FatVertex& vertex : vertices)
//     {
//         maxAdherence = Max(maxAdherence, vertex.branchAdherence_);
//     }
// 
//     // Compute wind
//     for (FatVertex& vertex : vertices)
//     {
//         vertex.branchAdherence_ /= maxAdherence;
//     }
// }
// void TreeFactory::normalizePhases(PODVector<FatVertex>& vertices)
// {
//     for (FatVertex& vertex : vertices)
//     {
//         vertex.phase_ = Fract(vertex.phase_);
//     }
// }


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
//             const LodInfo& lodInfo = lodInfos[lod];
// 
//             // Generate branches
//             // #TODO It is possible to move branch generation out of the loop
//             Vector<TreeFactory::Branch> branches;
//             generateSkeleton(lodInfo, branches);
// 
//             // Triangulate all
//             PODVector<FatVertex> vertices;
//             PODVector<FatIndex> indices;
//             triangulateBranches(materialIndex, lodInfo, branches, vertices, indices);
//             triangulateFronds(materialIndex, lodInfo, branches, vertices, indices);
//             computeMainAdherence(vertices);
//             normalizeBranchesAdherence(vertices);
//             normalizePhases(vertices);
// 
//             // Compute AABB and radius
//             const BoundingBox aabb = CalculateBoundingBox(vertices.Buffer(), vertices.Size());
//             const Vector3 aabbCenter = static_cast<Vector3>(aabb.Center());
//             modelRadius_ = 0.0f;
//             for (const FatVertex& vertex : vertices)
//             {
//                 const float dist = ((vertex.position_ - aabbCenter) * Vector3(1, 0, 1)).Length();
//                 modelRadius_ = Max(modelRadius_, dist);
//             }
//             boundingBox.Merge(aabb);
// 
//             SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);
//             geometry->SetVertexBuffer(0, vertexBuffer);
//             geometry->SetIndexBuffer(indexBuffer);
//             model->SetGeometry(materialIndex, lod, geometry);
//             groupOffsets.Push(indexOffset);
//             groupSizes.Push(indices.Size());
//             indexOffset += indices.Size();
// 
//             unsigned vertexOffset = vertexData.Size();
//             for (unsigned i = 0; i < indices.Size(); ++i)
//             {
//                 indexData.Push(static_cast<unsigned short>(indices[i] + vertexOffset));
//             }
// 
//             for (unsigned i = 0; i < vertices.Size(); ++i)
//             {
//                 vertexData.Push(VegetationVertex::Construct(vertices[i]));
//             }
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
// 
// void TreeFactory::SetRootBranchGroup(const FlexEngine::BranchGroup& root)
// {
//     REAL_ROOT = root;
// }
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
    const Vector<String>& materials = GetMaterials(node.GetChild("materials"));
    factory.SetMaterials(materials);
    factory.SetTrunkStrength(GetValue(node.GetChild("trunkStrength"), 0.5f));
//     factory.SetRootBranchGroup(ReadBranchGroupFromXML(node.GetChild("root"), materials));

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

template <>
TreeElementDistributionType To<TreeElementDistributionType>(const String& str)
{
    const static HashMap<String, TreeElementDistributionType> m =
    {
        MakePair(String("Alternate"), TreeElementDistributionType::Alternate),
    };
    return *m[str];
}

}
