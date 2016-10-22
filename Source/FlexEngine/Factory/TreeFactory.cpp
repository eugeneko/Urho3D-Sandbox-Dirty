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
            const float randomPad = random.FloatFrom11() * distribution.twirlNoise_;
            const float angle = distribution.twirlStep_ * i + distribution.twirlBase_ + randomPad + baseAngle;
            const bool parity = Fract((angle + 90) / 360) < 0.5;
            result.Push(angle + distribution.twirlSkew_ * (parity ? -1 : 1));
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

/// Project vector onto plane.
Vector3 ProjectVectorOnPlane(const Vector3& vec, const Vector3& normal)
{
    return (vec - normal * normal.DotProduct(vec)).Normalized();
}

}

//////////////////////////////////////////////////////////////////////////
Vector3 TessellatedBranchPoint::GetPosition(float angle) const
{
    return (xAxis_ * Cos(angle) + yAxis_ * Sin(angle)).Normalized();
}

//////////////////////////////////////////////////////////////////////////
VegetationVertex VegetationVertex::Construct(const DefaultVertex& vertex)
{
    VegetationVertex result;
    result.position_ = vertex.position_;
    result.tangent_ = vertex.tangent_;
    result.binormal_ = vertex.binormal_;
    result.normal_ = vertex.normal_;
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
BranchDescription GenerateBranch(const Vector3& initialPosition, const Vector3& initialDirection, const Vector2& initialAdherence,
    float length, float baseRadius, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape, unsigned minNumKnots)
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
        const float t = i / (numKnots - 1.0f);
        result.positions_.Push(position);
        result.radiuses_.Push(branchShape.radius_.ComputeValue(t) * baseRadius);
        result.adherences_.Push(initialAdherence);
        result.frondSizes_.Push(frondShape.size_.ComputeValue(t));

        // Apply gravity
        if (direction.AbsDotProduct(Vector3::DOWN) < 1.0f - M_LARGE_EPSILON)
        {
            const float maxAngle = direction.Angle(Vector3::DOWN);
            const float angle = Min(maxAngle, step * branchShape.gravityIntensity_);
            const Vector3 side = direction.CrossProduct(Vector3::DOWN);
            direction = Quaternion(angle, side) * direction;
        }

        // Next position
        direction = direction.Normalized();
        position += direction * step;
    }

    // Update adherence
    for (unsigned i = 0; i < numKnots; ++i)
    {
        const float factor = static_cast<float>(i) / (numKnots + 1);
        float degree = Pow(factor, 1.0f / (1.0f - branchShape.resistance_));

        result.adherences_[i].x_ += branchShape.windMainMagnitude_ * degree;
        result.adherences_[i].y_ += branchShape.windTurbulenceMagnitude_ * degree;
    }

    result.length_ = length;
    result.GenerateCurves();
    return result;
}

TessellatedBranchPoints TessellateBranch(const BranchDescription& branch,
    const float multiplier, const BranchTessellationQualityParameters& quality)
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

    const unsigned numSegments = branch.positionsCurve_.xcoef_.Size();
    const unsigned minNumSegments = Max(1u, static_cast<unsigned>(quality.minNumSegments_ * multiplier));
    const float minAngle = Clamp(quality.minAngle_ / multiplier, 1.0f, 90.0f);
    // Point is forcedly committed after specified number of skipped points
    const unsigned maxNumSkipped = (quality.maxNumSegments_ + minNumSegments - 1) / minNumSegments - 1;

    // Generate points
    Vector3 prevDirection;
    unsigned prevIndex = 0;
    for (unsigned i = 0; i <= quality.maxNumSegments_; ++i)
    {
        const float t = static_cast<float>(i) / quality.maxNumSegments_;
        const Vector3 curDirection = SampleBezierCurveDerivative(branch.positionsCurve_, t);
        if (i == 0 || i == quality.maxNumSegments_ || i - prevIndex == maxNumSkipped
            || curDirection.Angle(prevDirection) >= minAngle)
        {
            prevIndex = i;
            prevDirection = SampleBezierCurveDerivative(branch.positionsCurve_, t);

            TessellatedBranchPoint point;
            point.location_ = t;
            point.position_ = SampleBezierCurve(branch.positionsCurve_, t);
            point.radius_ = SampleBezierCurve(branch.radiusesCurve_, t);
            point.adherence_ = SampleBezierCurve(branch.adherencesCurve_, t);
            point.frondSize_ = SampleBezierCurve(branch.frondSizesCurve_, t);
            result.Push(point);
        }
    }

    // Compute additional info
    float prevRadius = 0.0f;
    Vector3 prevPosition = Vector3::ZERO;
    float relativeDistance = 0.0f;
    for (unsigned i = 0; i < result.Size(); ++i)
    {
        const float radius = result[i].radius_;

        // Compute point position and orientation
        const Vector3 position = result[i].position_;
        result[i].zAxis_ = (result[Min(i + 1, result.Size() - 1)].position_ - result[i == 0 ? 0 : i - 1].position_).Normalized();
        result[i].basis_ = MakeBasisFromDirection(result[i].zAxis_);
        result[i].xAxis_ = GetBasisX(result[i].basis_.RotationMatrix());
        result[i].yAxis_ = GetBasisY(result[i].basis_.RotationMatrix());

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

        result[i].relativeDistance_ = relativeDistance;

        // Update previous values
        prevPosition = position;
        prevRadius = radius;
    }
    return result;
}

PODVector<DefaultVertex> GenerateBranchVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
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
    for (unsigned i = 0; i < points.Size(); ++i)
    {
        for (unsigned j = 0; j <= numRadialSegments; ++j)
        {
            const float factor = static_cast<float>(j) / numRadialSegments;
            const float angle = factor * 360;
            const Vector3 normal = (points[i].xAxis_ * Cos(angle) + points[i].yAxis_ * Sin(angle)).Normalized();

            DefaultVertex vertex;
            vertex.position_ = points[i].position_ + points[i].radius_ * normal;
            vertex.geometryNormal_ = normal;
            vertex.normal_ = normal;
            vertex.tangent_ = points[i].zAxis_;
            vertex.binormal_ = CrossProduct(vertex.normal_, vertex.tangent_);
            vertex.uv_[0] = Vector4(factor / shape.textureScale_.x_, points[i].relativeDistance_ / shape.textureScale_.y_, 0, 0);
            vertex.colors_[0].r_ = points[i].adherence_.x_;
            vertex.colors_[0].g_ = points[i].adherence_.y_;
            vertex.colors_[0].b_ = branch.phase_;
            vertex.colors_[0].a_ = 0.0f;

            result.Push(vertex);
        }
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

PODVector<DefaultVertex> GenerateFrondVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
    const FrondShapeSettings& shape)
{
    PODVector<DefaultVertex> result;
    if (points.Empty())
    {
        URHO3D_LOGERROR("Points array must not be empty");
        return result;
    }

    const float rotationAngle = shape.rotationAngle_.Get(StableRandom(branch.positions_[0]));
    for (unsigned i = 0; i < points.Size(); ++i)
    {
        // Left vertex
        const float leftAngle = 180.0f - shape.bendingAngle_ + rotationAngle;
        DefaultVertex vers[3];
        vers[0].position_ = points[i].position_ + points[i].GetPosition(leftAngle) * points[i].frondSize_;
        vers[0].uv_[0] = Vector4(0.0f, points[i].location_, 0, 0);
        vers[0].colors_[0].r_ = points[i].adherence_.x_;
        vers[0].colors_[0].g_ = points[i].adherence_.y_;
        vers[0].colors_[0].b_ = branch.phase_;
        vers[0].colors_[0].a_ = 0.0f;

        // Right vertex
        const float rightAngle = shape.bendingAngle_ + rotationAngle;
        vers[2].position_ = points[i].position_ + points[i].GetPosition(rightAngle) * points[i].frondSize_;
        vers[2].uv_[0] = Vector4(1.0f, points[i].location_, 0, 0);
        vers[2].colors_[0].r_ = points[i].adherence_.x_;
        vers[2].colors_[0].g_ = points[i].adherence_.y_;
        vers[2].colors_[0].b_ = branch.phase_;
        vers[2].colors_[0].a_ = 0.0f;

        // Center vertex
        vers[1] = LerpVertices(vers[0], vers[2], 0.5f);
        vers[1].position_ = points[i].position_;
        vers[1].colors_[0].r_ = points[i].adherence_.x_;
        vers[1].colors_[0].g_ = points[i].adherence_.y_;
        vers[1].colors_[0].b_ = branch.phase_;
        vers[1].colors_[0].a_ = 0.0f;

        result.Push(vers[0]);
        result.Push(vers[1]);
        result.Push(vers[2]);
    }
    return result;
}

PODVector<unsigned> GenerateFrondIndices(unsigned numPoints)
{
    PODVector<unsigned> result;
    for (unsigned i = 1; i < numPoints; ++i)
    {
        AppendQuadToIndices(result, (i - 1) * 3, 0, 1, 3, 4);
        AppendQuadToIndices(result, (i - 1) * 3, 1, 2, 4, 5);
    }
    return result;
}

void GenerateBranchGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments)
{
    numRadialSegments = Max(3u, static_cast<unsigned>(numRadialSegments * shape.quality_));
    const PODVector<DefaultVertex> tempVertices =
        GenerateBranchVertices(branch, points, shape, numRadialSegments);
    const PODVector<unsigned> tempIndices =
        GenerateBranchIndices(PODVector<unsigned>(points.Size(), numRadialSegments), tempVertices.Size());
    factory.AddPrimitives(tempVertices, tempIndices, true);
}

void GenerateFrondGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points,
    const FrondShapeSettings& shape)
{
    PODVector<DefaultVertex> vertices = GenerateFrondVertices(branch, points, shape);
    PODVector<unsigned> indices = GenerateFrondIndices(points.Size());
    CalculateNormals(vertices, indices);
    CalculateTangents(vertices, indices);
    for (unsigned i = 0; i < vertices.Size(); ++i)
        vertices[i].normal_ = vertices[i].geometryNormal_;
    factory.AddPrimitives(vertices, indices, true);
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
        elem.adherence_ = Vector2(0.0, 0.0);
        elem.phase_ = 0.0f;
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
            elem.adherence_ = SampleBezierCurve(parent.adherencesCurve_, location);
            elem.phase_ = parent.phase_;
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
    const TreeElementDistribution& distribution, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape,
    unsigned minNumKnots)
{
    const PODVector<TreeElementLocation> elements = DistributeElementsOverParent(parent, distribution);
    const FloatRange lengthRange(branchShape.length_.x_, branchShape.length_.y_);
    const float parentLength = distribution.frequency_ == 0 || !branchShape.relativeLength_ ? 1.0f : parent.length_;

    Vector<BranchDescription> result;
    for (unsigned i = 0; i < elements.Size(); ++i)
    {
        const TreeElementLocation& element = elements[i];
        const Vector3 direction = GetBasisZ(element.rotation_.RotationMatrix());
        const float length = parentLength * // #TODO Move this code to DistributeElementsOverParent
            lengthRange.Get(element.noise_.w_) * distribution.growthScale_.ComputeValue(element.location_);
        BranchDescription branch = GenerateBranch(
            element.position_, direction, element.adherence_, length, element.baseRadius_, branchShape, frondShape, minNumKnots);
        branch.phase_ = element.phase_ + element.noise_.w_ * branchShape.windPhaseOffset_;
        branch.index_ = i;
        result.Push(branch);
    }


    // Create fake ending branch
    if (branchShape.fakeEnding_ && distribution.frequency_ != 0)
    {
        BranchDescription branch;

        // Compute length of fake branch
        const float location = elements.Empty() ? 0.0f : elements.Back().location_;
        branch.index_ = elements.Size();
        branch.length_ = parentLength * (1.0f - location);
        branch.phase_ = parent.phase_;

        // Compute number of knots
        const unsigned numKnots = Max(minNumKnots, static_cast<unsigned>((1.0f - location) * branch.positions_.Size()));

        // Generate knots
        for (unsigned i = 0; i < numKnots; ++i)
        {
            const float t = Lerp(location, 1.0f, i / (numKnots - 1.0f));
            branch.positions_.Push(SampleBezierCurve(parent.positionsCurve_, t));
            branch.radiuses_.Push(SampleBezierCurve(parent.radiusesCurve_, t));
            branch.adherences_.Push(SampleBezierCurve(parent.adherencesCurve_, t));
            branch.frondSizes_.Push(SampleBezierCurve(parent.frondSizesCurve_, t));
        }
        branch.GenerateCurves();

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

    const Quaternion zRotation = Quaternion(shape.rotateZ_.Get(location.noise_.w_), Vector3::FORWARD);

    const Matrix3 rotationMatrix = (alignVerticalRotation * baseRotation * zRotation).RotationMatrix();
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
    DefaultVertex vers[5];

    vers[0].position_ = Vector3(-0.5f, 0.0f, 0.0f);
    vers[0].uv_[0] = Vector4(0, 0, 0, 0);
    vers[0].colors_[0].r_ = location.adherence_.x_ + shape.windMainMagnitude_.x_;
    vers[0].colors_[0].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.x_;
    vers[0].colors_[0].b_ = location.phase_;
    vers[0].colors_[0].a_ = shape.windOscillationMagnitude_.x_;

    vers[1].position_ = Vector3(0.5f, 0.0f, 0.0f);
    vers[1].uv_[0] = Vector4(1, 0, 0, 0);
    vers[1].colors_[0].r_ = location.adherence_.x_ + shape.windMainMagnitude_.x_;
    vers[1].colors_[0].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.x_;
    vers[1].colors_[0].b_ = location.phase_;
    vers[1].colors_[0].a_ = shape.windOscillationMagnitude_.x_;

    vers[2].position_ = Vector3(-0.5f, 0.0f, 1.0f);
    vers[2].uv_[0] = Vector4(0, 1, 0, 0);
    vers[2].colors_[0].r_ = location.adherence_.x_ + shape.windMainMagnitude_.y_;
    vers[2].colors_[0].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.y_;
    vers[2].colors_[0].b_ = location.phase_;
    vers[2].colors_[0].a_ = shape.windOscillationMagnitude_.y_;

    vers[3].position_ = Vector3(0.5f, 0.0f, 1.0f);
    vers[3].uv_[0] = Vector4(1, 1, 0, 0);
    vers[3].colors_[0].r_ = location.adherence_.x_ + shape.windMainMagnitude_.y_;
    vers[3].colors_[0].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.y_;
    vers[3].colors_[0].b_ = location.phase_;
    vers[3].colors_[0].a_ = shape.windOscillationMagnitude_.y_;

    vers[4] = LerpVertices(vers[0], vers[3], 0.5f);
    vers[4].position_.y_ += shape.bending_;

    // Compute position in global space
    const Vector3 geometryScale = shape.scale_ * shape.size_.Get(location.noise_.z_);
    const Vector3 basePosition = position + rotationMatrix * shape.junctionOffset_;
    for (DefaultVertex& vertex : vers)
        vertex.position_ = basePosition + rotationMatrix * (vertex.position_ * geometryScale);

    // Compute normals
    const Vector3 flatNormal = ((vers[4].position_ - foliageCenter) * Vector3(1, 0, 1)).Normalized();
    const Vector3 globalNormal = Lerp(flatNormal, Vector3::UP, 0.5f).Normalized();

    vers[4].normal_ = globalNormal;
    for (unsigned i = 0; i < 4; ++i)
    {
        const Vector3 bumpedNormal = ProjectVectorOnPlane(vers[i].position_ - vers[4].position_, globalNormal).Normalized();
        vers[i].normal_ = Lerp(globalNormal, bumpedNormal, shape.bumpNormals_).Normalized();
    }

    for (unsigned i = 0; i < 5; ++i)
    {
        vers[i].tangent_ = ConstructOrthogonalVector(vers[i].normal_);
        vers[i].binormal_ = vers[i].normal_.CrossProduct(vers[i].tangent_);
    }

    const unsigned inds[4 * 3] =
    {
        0, 4, 1,
        1, 4, 3,
        3, 4, 2,
        2, 4, 0
    };

    // Compute real normals
    CalculateNormals(vers, 5, inds, 4);

    factory.AddPrimitives(vers, inds, true);
}

}