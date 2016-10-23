#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/BezierCurve.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/StandardRandom.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace Urho3D
{

class Model;
class ResourceCache;
class XMLElement;

}

namespace FlexEngine
{

struct FactoryContext;
struct BezierCurve3D;
struct DefaultVertex;
class ModelFactory;

/// Vegetation Vertex.
struct VegetationVertex
{
    Vector3 position_;
    Vector3 tangent_;
    Vector3 binormal_;
    Vector3 normal_;
    Vector3 geometryNormal_;
    Vector2 uv_;
    Vector4 param_;

    /// Construct from fat vertex.
    static VegetationVertex Construct(const DefaultVertex& vertex);
    /// Returns vertex format.
    static PODVector<VertexElement> Format();
};

/// Tree element distribution type.
enum class TreeElementDistributionType
{
    /// Child branch has random location on the parent branch.
    //Random,
    /// Child branch is rotated on specified angle relatively to previous child branch.
    Alternate,
    /// Same as alternate, but each pair of branches grow from the same point.
    Opposite,
};

/// Branch shape settings.
struct BranchShapeSettings
{
    /// Scale of texture.
    Vector2 textureScale_;
    /// Multiplier for quality settings.
    float quality_ = 0.0f;
    /// The length of the branch is randomly taken from range [begin, end].
    FloatRange length_ = 1.0f;
    /// Specifies whether the end of parent branch is also interpreted as child branch.
    bool fakeEnding_ = false;
    /// Radius of the branch.
    CubicCurveWrapper radius_;

    /// Controls branch geometry bending caused by external force. Should be in range [0, 1).
    /// 0 is the linear deformation of the branch without geometry bending.
    float resistance_ = 0.0f;
    /// Intensity of deformation caused by gravity.
    float gravityIntensity_ = 0.0f;
    /// Value of deformations caused by main wind.
    float windMainMagnitude_ = 0.0f;
    /// Value of deformations caused by turbulence.
    float windTurbulenceMagnitude_ = 0.0f;
    /// Wind oscillation phase offset.
    float windPhaseOffset_ = 0.0f;
};

/// Frond shape settings.
struct FrondShapeSettings
{
    /// Size of the frond.
    CubicCurveWrapper size_;
    /// Bending angle.
    float bendingAngle_ = 0.0f;
    /// Rotation angle range.
    FloatRange rotationAngle_;
};

/// Branch description.
struct BranchDescription
{
    /// Fake branch provides no geometry but can be used for nesting.
    bool fake_ = false;
    /// Index of this branch on parent.
    unsigned index_ = 0;
    /// Positions of branch knots.
    BezierCurve<Vector3> positions_;
    /// Rotations of branch knots.
    BezierCurve<Matrix3> rotations_;
    /// Radiuses of branch knots.
    BezierCurve<float> radiuses_;
    /// Adherences of branch knots.
    BezierCurve<Vector2> adherences_;
    /// Sizes of frond.
    BezierCurve<float> frondSizes_;
    /// Branch length.
    float length_ = 1.0f;
    /// Branch oscillation phase.
    float phase_ = 0.0f;
};

/// Generate branch using specified parameters. Return Bezier curve knots. Number of knots is computed automatically.
BranchDescription GenerateBranch(const Vector3& initialPosition, const Quaternion& initialRotation, const Vector2& initialAdherence,
    float length, float baseRadius, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape, unsigned minNumKnots);

/// Branch tessellation quality parameters.
struct BranchTessellationQualityParameters
{
    /// New point is emitted if Bezier curve direction change is at least minAngle_.
    float minAngle_ = 0.0f;
    /// Minimal number of segments. Minimal value is 1.
    unsigned minNumSegments_ = 1;
    /// Maximal number of segments. Minimal value is 5.
    unsigned maxNumSegments_ = 5;
};

/// Tessellated branch point.
struct TessellatedBranchPoint
{
    /// Location on curve.
    float location_;
    /// Radius of point.
    float radius_;
    /// Position.
    Vector3 position_;
    /// Adherence.
    Vector2 adherence_;
    /// Frond size.
    float frondSize_;

    /// Branch point rotation.
    Quaternion rotation_;
    /// Relative uv distance from branch point.
    float relativeDistance_;

    /// Get relative position at the unit distance and specified angle.
    Vector3 GetPosition(float angle) const;

};

/// Tessellated branch points.
using TessellatedBranchPoints = PODVector<TessellatedBranchPoint>;

/// Tessellate branch with specified quality. Return array of points with position on curve in w component.
TessellatedBranchPoints TessellateBranch(const BranchDescription& branch,
    const float multiplier, const BranchTessellationQualityParameters& quality);

/// Generate branch geometry vertices.
PODVector<DefaultVertex> GenerateBranchVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments);

/// Generate branch geometry vertices.
PODVector<unsigned> GenerateBranchIndices(const PODVector<unsigned>& numRadialSegments, unsigned maxVertices);

/// Generate branch fronds vertices.
PODVector<DefaultVertex> GenerateFrondVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
    const FrondShapeSettings& shape);

/// Generate branch fronds vertices.
PODVector<unsigned> GenerateFrondIndices(unsigned numPoints);

/// Generate branch geometry.
void GenerateBranchGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments);

/// Generate frond geometry.
void GenerateFrondGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points,
    const FrondShapeSettings& shape);

/// Tree element spawn mode.
enum class TreeElementSpawnMode
{
    /// Single element with specified position and orientation is spawned.
    Explicit,
    /// Absolute number of elements is spawned. Frequency determines the total amount of elements.
    Absolute,
    /// Relative number of elements is spawned. Frequency determines number of elements per unit.
    Relative
};

/// Tree element distribution settings.
struct TreeElementDistribution
{
    /// Seed of random generator.
    unsigned seed_ = 0;
    /// Spawn mode.
    TreeElementSpawnMode spawnMode_ = TreeElementSpawnMode::Explicit;
    /// Number of branches.
    float frequency_ = 0;

    /// Position (if explicit).
    Vector3 position_;
    /// Rotation (if explicit).
    Quaternion rotation_;

    /// Distribution type.
    TreeElementDistributionType distributionType_;
    /// Location on parent branch.
    FloatRange location_;
    /// Child density function.
    CubicCurveWrapper density_;
    /// Step of branch twirl (alternate distribution).
    float twirlStep_ = 0.0f;
    /// Random angle added to branch twirl.
    float twirlNoise_ = 0.0f;
    /// Base rotation angle of all children branches.
    float twirlBase_ = 0.0f;

    /// Size of element depends on parent size.
    bool relativeSize_ = true;
    /// Scale of child elements size or length.
    CubicCurveWrapper growthScale_;
    /// Angle between children and parent branch.
    CubicCurveWrapper growthAngle_;
    /// Rotation of children along Y axis.
    CubicCurveWrapper growthTwirl_;
};

/// Location of tree element.
struct TreeElementLocation
{
    /// Seed of random parameters of element shape.
    unsigned seed_;
    /// Location on parent branch.
    float location_;

    /// Position of element.
    Vector3 position_;
    /// Rotation of element.
    Quaternion rotation_;
    /// Size of element.
    float size_;

    /// Base adherence.
    Vector2 adherence_;
    /// Phase.
    float phase_;
    /// Parent (base) radius.
    float baseRadius_;
    /// Just random noise in range [0, 1] that could be used for further generation.
    Vector4 noise_;
};

/// Distribute children over parent branch.
PODVector<TreeElementLocation> DistributeElementsOverParent(const BranchDescription& parent, const TreeElementDistribution& distrib);

/// Integrate density function and generate points in range [0, 1] distributed with specified density.
PODVector<float> IntegrateDensityFunction(const CubicCurveWrapper& density, unsigned count);

/// Instantiate branch group.
Vector<BranchDescription> InstantiateBranchGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape,
    unsigned minNumKnots);

/// Type of leaf normal.
enum class LeafNormalType
{
    /// Fair normal of leaf geometry.
    Fair,
    /// Fake normal.
    Fake
};

/// Leaf shape settings.
struct LeafShapeSettings
{
    /// Scale of leaf geometry along dimensions.
    Vector3 scale_ = Vector3::ONE;
    /// Adjusts whether the leaves are orientated in the global world space instead of local space of parent branch. Should be in range [0, 1].
    FloatRange adjustToGlobal_ = 0.0f;
    /// Adjusts whether the leaves are orientated vertically.
    FloatRange alignVertical_ = 0.0f;
    /// Offset of the leaf junction point.
    Vector3 junctionOffset_ = Vector3::ZERO;
    /// Rotation of the leaf along Z axis is randomly taken from the range.
    FloatRange rotateZ_ = 1.0f;
    /// Degree of leaf geometry bending.
    float bending_ = 0.0f;
    /// Normal type.
    LeafNormalType normalType_ = LeafNormalType::Fair;
    /// Adjusts intensity of bumping fake leaf normals.
    float bumpNormals_ = 0.0f;
    /// First color.
    Color firstColor_ = Color::WHITE;
    /// Second color.
    Color secondColor_ = Color::WHITE;
    /// Value of deformations caused by main wind, minimum and maximum values.
    Vector2 windMainMagnitude_ = Vector2::ZERO;
    /// Value of deformations caused by turbulence, minimum and maximum values.
    Vector2 windTurbulenceMagnitude_ = Vector2::ZERO;
    /// Value of foliage oscillations, minimum and maximum values.
    Vector2 windOscillationMagnitude_ = Vector2::ZERO;
};

/// Leaf description.
struct LeafDescription
{
    /// Location of the leaf.
    TreeElementLocation location_;
};

/// Instantiate leafs.
PODVector<LeafDescription> InstantiateLeafGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const LeafShapeSettings& shape);

/// Generate leaf geometry vertices and indices.
void GenerateLeafGeometry(ModelFactory& factory,
    const LeafShapeSettings& shape, const TreeElementLocation& location, const Vector3& foliageCenter);

}
