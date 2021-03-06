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
                result.Push(locations[i]);
            break;
        }
    case TreeElementDistributionType::Opposite:
        {
            const PODVector<float> locations = IntegrateDensityFunction(distribution.density_, (count + 1) / 2);
            for (unsigned i = 0; i < count; ++i)
            {
                result.Push(locations[i / 2]);
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
    case TreeElementDistributionType::Opposite:
    {
        for (unsigned i = 0; i < count; ++i)
        {
            const float randomPad = random.FloatFrom11() * distribution.twirlNoise_;
            const float angle = distribution.twirlStep_ * i + distribution.twirlBase_ + randomPad;
            result.Push(angle);
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
    return rotation_ * Vector3(Cos(angle), 0.0f, Sin(angle));
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
BranchDescription GenerateBranch(const Vector3& initialPosition, const Quaternion& initialRotation, const Vector2& initialAdherence,
    float length, float baseRadius, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape, unsigned minNumKnots)
{
    // Compute parameters
    const unsigned numKnots = minNumKnots;
    const float step = length / (numKnots - 1);

    // Generate knots
    BranchDescription result;
    result.generateBranch_ = branchShape.generateBranch_;
    result.generateFrond_ = frondShape.generateFrond_;
    result.length_ = length;
    result.quality_ = branchShape.quality_;
    result.frondBending_ = frondShape.bendingAngle_;
    result.frondRotation_ = frondShape.rotationAngle_.Get(StableRandom(initialPosition));

    Vector3 position = initialPosition;
    Quaternion rotation = initialRotation;
    for (unsigned i = 0; i < numKnots; ++i)
    {
        const float t = i / (numKnots - 1.0f);
        const float degree = Pow(t, 1.0f / (1.0f - branchShape.resistance_));
        const Vector2 magnitudes(branchShape.windMainMagnitude_, branchShape.windTurbulenceMagnitude_);

        result.positions_.AddPoint(position);
        result.rotations_.AddPoint(rotation.RotationMatrix());
        result.radiuses_.AddPoint(branchShape.radius_.ComputeValue(t) * baseRadius);
        result.adherences_.AddPoint(initialAdherence + magnitudes * degree);
        result.frondSizes_.AddPoint(frondShape.size_.ComputeValue(t));

        // Apply gravity
        const Vector3 direction = rotation * Vector3::UP;
        if (direction.AbsDotProduct(Vector3::DOWN) < Cos(0.1))
        {
            const float maxAngle = direction.Angle(Vector3::DOWN);
            const float minAngle = direction.Angle(Vector3::UP);
            const float angle = Clamp(step * branchShape.gravityIntensity_, -minAngle, maxAngle);
            const Vector3 side = direction.CrossProduct(Vector3::DOWN);
            rotation = Quaternion(angle, side) * rotation;
        }

        // Next position
        position += (rotation * Vector3::UP) * step;
    }

    return result;
}

TessellatedBranchPoints TessellateBranch(const BranchDescription& branch, const BranchQualityParameters& quality)
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

    const unsigned numSegments = branch.positions_.GetNumPoints() - 1;
    const unsigned minNumSegments = Max(1u, static_cast<unsigned>(quality.minNumSegments_ * branch.quality_));
    const float minAngle = Clamp(quality.minAngle_ / branch.quality_, 1.0f, 90.0f);
    // Point is forcedly committed after specified number of skipped points
    const unsigned maxNumSkipped = (quality.maxNumSegments_ + minNumSegments - 1) / minNumSegments - 1;

    // Generate points
    Vector3 prevDirection;
    unsigned prevIndex = 0;
    for (unsigned i = 0; i <= quality.maxNumSegments_; ++i)
    {
        const float t = static_cast<float>(i) / quality.maxNumSegments_;
        const Vector3 curDirection = branch.positions_.SampleDerivative(t);
        if (i == 0 || i == quality.maxNumSegments_ || i - prevIndex == maxNumSkipped
            || curDirection.Angle(prevDirection) >= minAngle)
        {
            prevIndex = i;
            prevDirection = branch.positions_.SampleDerivative(t);

            TessellatedBranchPoint point;
            point.location_ = t;
            point.position_ = branch.positions_.SamplePoint(t);
            point.rotation_ = Quaternion(branch.rotations_.SamplePoint(t));
            point.radius_ = branch.radiuses_.SamplePoint(t);
            point.adherence_ = branch.adherences_.SamplePoint(t);
            point.frondSize_ = branch.frondSizes_.SamplePoint(t);
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

        // Compute relative distance
        if (i > 0)
        {
            ///                L        ln(r1/r0)
            /// u1 = u0 + ----------- * ---------
            ///           2 * pi * r0   r1/r0 - 1
            const float L = (result[i].position_ - prevPosition).Length();
            const float r0 = prevRadius;
            const float r1 = radius;
            const float rr = r1 / r0;
            /// ln(x) / (x - 1) ~= 1 - (x - 1) / 2 + ...
            const float k = 1 - (rr - 1) / 2;
            relativeDistance += k * L / (2 * M_PI * r0);
        }

        result[i].relativeDistance_ = relativeDistance;

        // Update previous values
        prevPosition = result[i].position_;
        prevRadius = radius;
    }
    return result;
}

PODVector<DefaultVertex> GenerateBranchVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
    const Vector2& textureScale, unsigned numRadialSegments)
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
            const Vector3 normal = points[i].GetPosition(-angle);

            DefaultVertex vertex;
            vertex.position_ = points[i].position_ + points[i].radius_ * normal;
            vertex.geometryNormal_ = normal;
            vertex.normal_ = normal;
            vertex.tangent_ = points[i].rotation_ * Vector3::UP;
            vertex.binormal_ = CrossProduct(vertex.normal_, vertex.tangent_);
            vertex.uv_[0] = Vector4(factor / textureScale.x_, points[i].relativeDistance_ / textureScale.y_, 0, 0);
            vertex.colors_[1].r_ = points[i].adherence_.x_;
            vertex.colors_[1].g_ = points[i].adherence_.y_;
            vertex.colors_[1].b_ = branch.phase_;
            vertex.colors_[1].a_ = 0.0f;

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

PODVector<DefaultVertex> GenerateFrondVertices(const BranchDescription& branch, const TessellatedBranchPoints& points)
{
    PODVector<DefaultVertex> result;
    if (points.Empty())
    {
        URHO3D_LOGERROR("Points array must not be empty");
        return result;
    }

    const float rotationAngle = branch.frondRotation_;
    for (unsigned i = 0; i < points.Size(); ++i)
    {
        // Left vertex
        const float leftAngle = 180.0f - branch.frondBending_ + rotationAngle;
        DefaultVertex vers[3];
        vers[0].position_ = points[i].position_ + points[i].GetPosition(leftAngle) * points[i].frondSize_;
        vers[0].uv_[0] = Vector4(0.0f, points[i].location_, 0, 0);
        vers[0].colors_[1].r_ = points[i].adherence_.x_;
        vers[0].colors_[1].g_ = points[i].adherence_.y_;
        vers[0].colors_[1].b_ = branch.phase_;
        vers[0].colors_[1].a_ = 0.0f;

        // Right vertex
        const float rightAngle = branch.frondBending_ + rotationAngle;
        vers[2].position_ = points[i].position_ + points[i].GetPosition(rightAngle) * points[i].frondSize_;
        vers[2].uv_[0] = Vector4(1.0f, points[i].location_, 0, 0);
        vers[2].colors_[1].r_ = points[i].adherence_.x_;
        vers[2].colors_[1].g_ = points[i].adherence_.y_;
        vers[2].colors_[1].b_ = branch.phase_;
        vers[2].colors_[1].a_ = 0.0f;

        // Center vertex
        vers[1] = LerpVertices(vers[0], vers[2], 0.5f);
        vers[1].position_ = points[i].position_;
        vers[1].colors_[1].r_ = points[i].adherence_.x_;
        vers[1].colors_[1].g_ = points[i].adherence_.y_;
        vers[1].colors_[1].b_ = branch.phase_;
        vers[1].colors_[1].a_ = 0.0f;

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
    const Vector2& textureScale, unsigned numRadialSegments)
{
    numRadialSegments = Max(3u, static_cast<unsigned>(numRadialSegments * branch.quality_));
    const PODVector<DefaultVertex> tempVertices =
        GenerateBranchVertices(branch, points, textureScale, numRadialSegments);
    const PODVector<unsigned> tempIndices =
        GenerateBranchIndices(PODVector<unsigned>(points.Size(), numRadialSegments), tempVertices.Size());
    factory.AddPrimitives(tempVertices, tempIndices, true);
}

void GenerateFrondGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points)
{
    PODVector<DefaultVertex> vertices = GenerateFrondVertices(branch, points);
    PODVector<unsigned> indices = GenerateFrondIndices(points.Size());
    CalculateNormals(vertices, indices);
    CalculateTangents(vertices, indices);
    for (unsigned i = 0; i < vertices.Size(); ++i)
        vertices[i].normal_ = vertices[i].geometryNormal_;
    factory.AddPrimitives(vertices, indices, true);
}

Vector<TreeElementLocation> DistributeElementsOverParent(const BranchDescription& parent, const TreeElementDistribution& distrib)
{
    const unsigned seed = distrib.seed_ != 0
        ? distrib.seed_
        : MakeHash(distrib.position_);
    StandardRandom random(seed);

    Vector<TreeElementLocation> result;
    switch (distrib.spawnMode_)
    {
    case TreeElementSpawnMode::Explicit:
        {
            TreeElementLocation elem;
            elem.seed_ = seed;
            elem.location_ = 0.5f;
            elem.position_ = distrib.position_;
            elem.rotation_ = distrib.rotation_;
            elem.size_ = distrib.growthScale_.ComputeValue(0.0f);
            elem.adherence_ = Vector2(0.0, 0.0);
            elem.phase_ = 0.0f;
            elem.baseRadius_ = 1.0f;
            result.Push(elem);
            break;
        }
    case TreeElementSpawnMode::Absolute:
    case TreeElementSpawnMode::Relative:
        {
            const float baseFrequency = distrib.spawnMode_ == TreeElementSpawnMode::Relative ? parent.length_ : 1.0f;
            const float baseLength = distrib.relativeSize_ ? parent.length_ : 1.0f;
            const unsigned numElements = static_cast<unsigned>(baseFrequency * distrib.frequency_);

            const PODVector<float> locations = ComputeChildLocations(distrib, random, numElements);
            const PODVector<float> twirlAngles = ComputeChildAngles(distrib, random, numElements, parent.index_);
            for (unsigned i = 0; i < locations.Size(); ++i)
            {
                // Find location
                const float interpolation = locations[i];
                const float location = distrib.location_.Get(interpolation);
                const float twirlAngle = twirlAngles[i];

                // Sample values
                const Vector3 position = parent.positions_.SamplePoint(location);
                const Matrix3 rotation = parent.rotations_.SamplePoint(location);

                // Generate some random
                const float growthScaleNoise = (StableRandom(position + Vector3::ONE * 1) * 2 - 1) * distrib.growthScaleNoise_ + 1;
                const float growthAngleNoise = (StableRandom(position + Vector3::ONE * 2) * 2 - 1) * distrib.growthAngleNoise_;
                const float growthTwirlNoise = (StableRandom(position + Vector3::ONE * 3) * 2 - 1) * distrib.growthTwirlNoise_;

                const float growthScale = distrib.growthScale_.ComputeValue(interpolation) * growthScaleNoise;
                const float growthAngle = 90 - (distrib.growthAngle_.ComputeValue(interpolation) + growthAngleNoise);
                const float growthTwirl = distrib.growthTwirl_.ComputeValue(interpolation) + growthTwirlNoise;

                // Generate branch
                TreeElementLocation elem;
                elem.seed_ = random.Random();
                elem.interpolation_ = interpolation;
                elem.location_ = location;
                elem.position_ = position;
                elem.rotation_ =
                    Quaternion(rotation)
                    * Quaternion(twirlAngles[i], Vector3::UP)
                    * Quaternion(growthAngle, Vector3::FORWARD)
                    * Quaternion(growthTwirl, Vector3::UP);
                elem.size_ = baseLength * growthScale;
                elem.adherence_ = parent.adherences_.SamplePoint(location);
                elem.phase_ = parent.phase_;
                elem.baseRadius_ = parent.radiuses_.SamplePoint(location);
                result.Push(elem);
            }
        }
        break;
    default:
        break;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
PODVector<float> IntegrateDensityFunction(const CubicCurveWrapper& density, unsigned count)
{
    PODVector<float> result;

    if (count == 1)
    {
        result.Push(0.0f);
    }
    else
    {
        // Integrate over density
        float minValue = M_INFINITY;
        float maxValue = 0.0f;
        float valueSum = 0.0f;
        for (unsigned i = 0; i < count; ++i)
        {
            const float densityValue = density.ComputeValue((i + 0.5f) / count);
            const float value = valueSum + densityValue / 2;

            result.Push(value);
            minValue = Min(minValue, value);
            maxValue = Max(maxValue, value);
            valueSum += densityValue;
        }

        // Normalize values
        if (maxValue - minValue > M_LARGE_EPSILON)
        {
            for (float& value : result)
                value = InverseLerp(minValue, maxValue, value);
        }
    }

    return result;
}

Vector<BranchDescription> InstantiateBranchGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape,
    unsigned minNumKnots)
{
    const Vector<TreeElementLocation> elements = DistributeElementsOverParent(parent, distribution);
    const FloatRange lengthRange(branchShape.length_.x_, branchShape.length_.y_);

    Vector<BranchDescription> result;
    for (unsigned i = 0; i < elements.Size(); ++i)
    {
        const TreeElementLocation& element = elements[i];
        BranchDescription branch = GenerateBranch(
            element.position_, element.rotation_, element.adherence_, element.size_, element.baseRadius_, branchShape, frondShape, minNumKnots);
        branch.phase_ = element.phase_ + StableRandom(element.position_ + Vector3::ONE * 5) * branchShape.windPhaseOffset_;
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
        branch.length_ = parent.length_ * (1.0f - location);
        branch.quality_ = parent.quality_;
        branch.phase_ = parent.phase_;
        branch.generateBranch_ = false;
        branch.generateFrond_ = false;

        // Compute number of knots
        const unsigned numKnots = Max(minNumKnots, static_cast<unsigned>((1.0f - location) * branch.positions_.GetNumPoints()));

        // Generate knots
        for (unsigned i = 0; i < numKnots; ++i)
        {
            const float t = Lerp(location, 1.0f, i / (numKnots - 1.0f));
            branch.positions_.AddPoint(parent.positions_.SamplePoint(t));
            branch.rotations_.AddPoint(parent.rotations_.SamplePoint(t));
            branch.radiuses_.AddPoint(parent.radiuses_.SamplePoint(t));
            branch.adherences_.AddPoint(parent.adherences_.SamplePoint(t));
            branch.frondSizes_.AddPoint(parent.frondSizes_.SamplePoint(t));
        }

        result.Push(branch);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
Vector<LeafDescription> InstantiateLeafGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const LeafShapeSettings& shape)
{
    const Vector<TreeElementLocation> elements = DistributeElementsOverParent(parent, distribution);

    Vector<LeafDescription> result;
    for (const TreeElementLocation& element : elements)
    {
        LeafDescription leaf;
        leaf.location_ = element;
        leaf.shape_ = shape;
        result.Push(leaf);
    }
    return result;
}

void GenerateLeafGeometry(ModelFactory& factory,
    const LeafShapeSettings& shape, const TreeElementLocation& location, const Vector3& foliageCenter)
{
    const float noise1 = StableRandom(location.position_ + Vector3::ONE * 1);

    // Instantiate random values
    const Vector3 position = location.position_;
    const Quaternion localRotation = location.rotation_;
    const Color leafColor = Lerp(shape.firstColor_, shape.secondColor_, noise1);

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

    // Get basis
    const Matrix3 rotationMatrix = localRotation.RotationMatrix();
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
    vers[0].colors_[0] = leafColor;
    vers[0].colors_[1].r_ = location.adherence_.x_ + shape.windMainMagnitude_.x_;
    vers[0].colors_[1].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.x_;
    vers[0].colors_[1].b_ = location.phase_;
    vers[0].colors_[1].a_ = shape.windOscillationMagnitude_.x_;

    vers[1].position_ = Vector3(0.5f, 0.0f, 0.0f);
    vers[1].uv_[0] = Vector4(1, 0, 0, 0);
    vers[1].colors_[0] = leafColor;
    vers[1].colors_[1].r_ = location.adherence_.x_ + shape.windMainMagnitude_.x_;
    vers[1].colors_[1].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.x_;
    vers[1].colors_[1].b_ = location.phase_;
    vers[1].colors_[1].a_ = shape.windOscillationMagnitude_.x_;

    vers[2].position_ = Vector3(-0.5f, 1.0f, 0.0f);
    vers[2].uv_[0] = Vector4(0, 1, 0, 0);
    vers[2].colors_[0] = leafColor;
    vers[2].colors_[1].r_ = location.adherence_.x_ + shape.windMainMagnitude_.y_;
    vers[2].colors_[1].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.y_;
    vers[2].colors_[1].b_ = location.phase_;
    vers[2].colors_[1].a_ = shape.windOscillationMagnitude_.y_;

    vers[3].position_ = Vector3(0.5f, 1.0f, 0.0f);
    vers[3].uv_[0] = Vector4(1, 1, 0, 0);
    vers[3].colors_[0] = leafColor;
    vers[3].colors_[1].r_ = location.adherence_.x_ + shape.windMainMagnitude_.y_;
    vers[3].colors_[1].g_ = location.adherence_.y_ + shape.windTurbulenceMagnitude_.y_;
    vers[3].colors_[1].b_ = location.phase_;
    vers[3].colors_[1].a_ = shape.windOscillationMagnitude_.y_;

    vers[4] = LerpVertices(vers[0], vers[3], 0.5f);
    vers[4].position_.z_ += shape.bending_;

    // Compute position in global space
    const Vector3 geometryScale = shape.scale_ * location.size_;
    const Vector3 basePosition = position + rotationMatrix * shape.junctionOffset_;
    for (DefaultVertex& vertex : vers)
        vertex.position_ = basePosition + rotationMatrix * (vertex.position_ * geometryScale);

    const unsigned inds[4 * 3] =
    {
        0, 4, 1,
        1, 4, 3,
        3, 4, 2,
        2, 4, 0
    };

    // Compute real normals
    CalculateNormals(vers, 5, inds, 4);
    if (shape.normalSmoothing_ == 0)
    {
        for (DefaultVertex& vertex : vers)
            vertex.normal_ = vertex.geometryNormal_;
        CalculateTangents(vers, 5, inds, 4);
    }
    else
    {
        for (unsigned i = 0; i < 5; ++i)
        {
            vers[i].normal_ = (vers[i].position_ - foliageCenter).Normalized();
            vers[i].tangent_ = ConstructOrthogonalVector(vers[i].normal_);
            vers[i].binormal_ = vers[i].normal_.CrossProduct(vers[i].tangent_);
        }
    }

    factory.AddPrimitives(vers, inds, true);
}

//////////////////////////////////////////////////////////////////////////
void TreeElementInstance::PostGenerate(TreeElementInstance* parent /*= nullptr*/)
{
    // Update recursive
    parent_ = parent;
    for (TreeElementInstance* child : children_)
        child->PostGenerate(this);

    // Compute foliage center
    foliageCenter_ = ComputeFoliageCenter();
    for (const TreeElementInstance* child : children_)
        foliageCenter_ += child->GetFoliageCenter();
}

void TreeElementInstance::Triangulate(ModelFactory& factory, BranchQualityParameters& quality, bool recursive /*= true*/)
{
    DoTriangulate(factory, quality);
    if (recursive)
    {
        for (const SharedPtr<TreeElementInstance>& child : children_)
            child->Triangulate(factory, quality, true);
    }
}

Vector3 TreeElementInstance::GetFoliageCenter(unsigned depth) const
{
    return parent_ && depth > 0
        ? parent_->GetFoliageCenter(depth - 1)
        : Vector3(foliageCenter_.Data()) / foliageCenter_.w_;
}

void TreeElementInstance::DoTriangulate(ModelFactory& /*factory*/, BranchQualityParameters& /*quality*/)
{
}

//////////////////////////////////////////////////////////////////////////
void TreeBranchInstance::DoTriangulate(ModelFactory& factory, BranchQualityParameters& quality)
{
    if (!desc_.generateBranch_ && !desc_.generateFrond_)
        return;

    const TessellatedBranchPoints tessellatedBranch = TessellateBranch(desc_, quality);

    if (desc_.generateBranch_)
    {
        factory.AddGeometry(branchMaterial_);
        GenerateBranchGeometry(factory, desc_, tessellatedBranch, Vector2::ONE, quality.numRadialSegments_);
    }

    if (desc_.generateFrond_)
    {
        factory.AddGeometry(frondMaterial_);
        GenerateFrondGeometry(factory, desc_, tessellatedBranch);
    }
}

//////////////////////////////////////////////////////////////////////////
Vector4 TreeLeafInstance::ComputeFoliageCenter() const
{
    //const Vector3 leafCenter = desc_.location_.position_
    //    + desc_.location_.rotation_ * (Vector3(0.0f, 0.5f, 0.0f) * desc_.shape_.scale_ * desc_.location_.size_);
    return Vector4(desc_.location_.position_, 1.0f);
}

void TreeLeafInstance::DoTriangulate(ModelFactory& factory, BranchQualityParameters& quality)
{
    factory.AddGeometry(leafMaterial_);
    GenerateLeafGeometry(factory, desc_.shape_, desc_.location_, GetFoliageCenter(desc_.shape_.normalSmoothing_));
}

}
