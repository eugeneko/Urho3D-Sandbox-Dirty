#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/BezierCurve.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/StandardRandom.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace Urho3D
{

class Model;
class ResourceCache;
class XMLElement;

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
    /// Whether to generate branch geometry.
    bool generateBranch_ = true;
    /// Multiplier for quality settings.
    float quality_ = 1.0f;
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
    /// Whether to generate frond geometry.
    bool generateFrond_ = false;
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
    /// Whether the branch has branch geometry.
    bool generateBranch_ = false;
    /// Whether the branch has frond geometry.
    bool generateFrond_ = false;
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
    /// Quality modifier.
    float quality_ = 1.0f;
    /// Rotation of fronds.
    float frondRotation_ = 0.0f;
    /// Bending of fronds.
    float frondBending_ = 0.0f;
};

/// Generate branch using specified parameters. Return Bezier curve knots. Number of knots is computed automatically.
BranchDescription GenerateBranch(const Vector3& initialPosition, const Quaternion& initialRotation, const Vector2& initialAdherence,
    float length, float baseRadius, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape, unsigned minNumKnots);

/// Branch quality parameters.
struct BranchQualityParameters
{
    /// New point is emitted if Bezier curve direction change is at least minAngle_.
    float minAngle_ = 0.0f;
    /// Minimal number of segments. Minimal value is 1.
    unsigned minNumSegments_ = 1;
    /// Maximal number of segments. Minimal value is 5.
    unsigned maxNumSegments_ = 5;
    /// Number of radial segments.
    unsigned numRadialSegments_ = 5;
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
TessellatedBranchPoints TessellateBranch(const BranchDescription& branch, const BranchQualityParameters& quality);

/// Generate branch geometry vertices.
PODVector<DefaultVertex> GenerateBranchVertices(const BranchDescription& branch, const TessellatedBranchPoints& points,
    const Vector2& textureScale, unsigned numRadialSegments);

/// Generate branch geometry vertices.
PODVector<unsigned> GenerateBranchIndices(const PODVector<unsigned>& numRadialSegments, unsigned maxVertices);

/// Generate branch fronds vertices.
PODVector<DefaultVertex> GenerateFrondVertices(const BranchDescription& branch, const TessellatedBranchPoints& points);

/// Generate branch fronds vertices.
PODVector<unsigned> GenerateFrondIndices(unsigned numPoints);

/// Generate branch geometry.
void GenerateBranchGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points,
    const Vector2& textureScale, unsigned numRadialSegments);

/// Generate frond geometry.
void GenerateFrondGeometry(ModelFactory& factory, const BranchDescription& branch, const TessellatedBranchPoints& points);

/// Tree element spawn mode.
enum class TreeElementSpawnMode
{
    /// Single element with specified position and orientation is spawned.
    Explicit,
    /// Absolute number of elements is spawned. Frequency determines the total amount of elements.
    Absolute,
    /// Experimental! Relative number of elements is spawned. Frequency determines number of elements per unit.
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
    /// Size or length of child elements.
    CubicCurveWrapper growthScale_;
    /// Size is multiplied by value from [1-noise, 1+noise].
    float growthScaleNoise_ = 0.0f;
    /// Angle between children and parent branch.
    CubicCurveWrapper growthAngle_;
    /// Random value from [-noise, +noise] is added to angle between children and parent branch.
    float growthAngleNoise_ = 0.0f;
    /// Rotation of children around Y axis.
    CubicCurveWrapper growthTwirl_;
    /// Random value from [-noise, +noise] is added to twirl.
    float growthTwirlNoise_ = 0.0f;
};

/// Location of tree element.
struct TreeElementLocation
{
    /// Seed of random parameters of element shape.
    unsigned seed_;
    /// Interpolation factor from to [0, 1].
    float interpolation_;
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
Vector<TreeElementLocation> DistributeElementsOverParent(const BranchDescription& parent, const TreeElementDistribution& distrib);

/// Integrate density function and generate points in range [0, 1] distributed with specified density.
PODVector<float> IntegrateDensityFunction(const CubicCurveWrapper& density, unsigned count);

/// Instantiate branch group.
Vector<BranchDescription> InstantiateBranchGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const BranchShapeSettings& branchShape, const FrondShapeSettings& frondShape,
    unsigned minNumKnots);

/// Leaf shape settings.
struct LeafShapeSettings
{
    /// Scale of leaf geometry along dimensions.
    Vector3 scale_ = Vector3::ONE;
    /// Offset of the leaf junction point.
    Vector3 junctionOffset_ = Vector3::ZERO;
    /// Degree of leaf geometry bending.
    float bending_ = 0.0f;
    /// Normal smoothing level.
    unsigned normalSmoothing_ = 0;
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
    /// Leaf shape settings.
    LeafShapeSettings shape_;
};

/// Instantiate leafs.
Vector<LeafDescription> InstantiateLeafGroup(const BranchDescription& parent,
    const TreeElementDistribution& distribution, const LeafShapeSettings& shape);

/// Generate leaf geometry vertices and indices.
void GenerateLeafGeometry(ModelFactory& factory,
    const LeafShapeSettings& shape, const TreeElementLocation& location, const Vector3& foliageCenter);

/// Instance of tree element.
class TreeElementInstance
    : public RefCounted
{
public:
    /// Post-generation update.
    void PostGenerate(TreeElementInstance* parent = nullptr);
    /// Triangulate element.
    void Triangulate(ModelFactory& factory, BranchQualityParameters& quality, bool recursive = true);

    /// Add children.
    void AddChild(SharedPtr<TreeElementInstance> child) { children_.Push(child); }
    /// Get children.
    const Vector<SharedPtr<TreeElementInstance>>& GetChildren() const { return children_; }
    /// Get foliage center.
    Vector4 GetFoliageCenter() const { return foliageCenter_; }
    /// Get foliage center at specified depth.
    Vector3 GetFoliageCenter(unsigned depth) const;

private:
    /// Compute center of the foliage.
    virtual Vector4 ComputeFoliageCenter() const { return Vector4::ZERO; }
    /// Triangulate element implementation.
    virtual void DoTriangulate(ModelFactory& factory, BranchQualityParameters& quality);

private:
    /// Parent element.
    TreeElementInstance* parent_ = nullptr;
    /// Children.
    Vector<SharedPtr<TreeElementInstance>> children_;
    /// Center of the foliage.
    Vector4 foliageCenter_ = Vector4::ZERO;
};

/// Instance of tree branch.
class TreeBranchInstance
    : public TreeElementInstance
{
public:
    /// Construct.
    TreeBranchInstance(const BranchDescription& desc,
        SharedPtr<Material> branchMaterial, SharedPtr<Material> frondMaterial)
        : desc_(desc), branchMaterial_(branchMaterial), frondMaterial_(frondMaterial) { }
    /// Get description.
    const BranchDescription& GetDescription() const { return desc_; }

private:
    /// Triangulate element implementation.
    virtual void DoTriangulate(ModelFactory& factory, BranchQualityParameters& quality) override;

private:
    /// Description of the branch.
    BranchDescription desc_;
    /// Material of the branch geometry.
    SharedPtr<Material> branchMaterial_;
    /// Material of the frond geometry.
    SharedPtr<Material> frondMaterial_;
};

/// Instance of tree leaf.
class TreeLeafInstance
    : public TreeElementInstance
{
public:
    /// Construct.
    TreeLeafInstance(const LeafDescription& desc, SharedPtr<Material> leafMaterial) : desc_(desc), leafMaterial_(leafMaterial) { }
    /// Get description.
    const LeafDescription& GetDescription() const { return desc_; }

private:
    /// Compute center of the foliage.
    virtual Vector4 ComputeFoliageCenter() const override;
    /// Triangulate element implementation.
    virtual void DoTriangulate(ModelFactory& factory, BranchQualityParameters& quality) override;

private:
    /// Description of the leaf.
    LeafDescription desc_;
    /// Material of the leaf geometry.
    SharedPtr<Material> leafMaterial_;
};

}

}
