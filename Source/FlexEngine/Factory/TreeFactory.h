#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/BezierCurve.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/MathFunction.h>
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
};

/// Branch shape settings.
struct BranchShapeSettings
{
    /// Scale of texture.
    Vector2 textureScale_;
    /// Multiplier for quality settings.
    float quality_ = 0.0f;
    /// The length of the branch is randomly taken from range [begin, max(begin, end)].
    FloatRange length_ = 1.0f;
    /// Specifies whether the length of the branch depends on the length of the parent branch.
    bool relativeLength_ = false;
    /// Specifies whether the end of parent branch is also interpreted as child branch.
    bool fakeEnding_ = false;
    /// Radius of the branch.
    CubicCurveWrapper radius_;

    /// Intensity of deformation caused by gravity.
    float gravityIntensity_ = 0.0f;
    /// Measure of geometry bending caused by gravity. Should be in range (0, 1].
    float gravityResistance_ = 0.0f;
};

/// Branch description.
struct BranchDescription
{
    /// Fake branch provides no geometry but can be used for nesting.
    bool fake_ = false;
    /// Index of this branch on parent.
    unsigned index_ = 0;
    /// Positions of branch knots.
    PODVector<Vector3> positions_;
    /// Bezier curve for positions.
    BezierCurve3D positionsCurve_;
    /// Radiuses of branch knots.
    PODVector<float> radiuses_;
    /// Bezier curve for radiuses.
    BezierCurve1D radiusesCurve_;
    /// Branch length.
    float length_ = 0.0f;
};

/// Generate branch using specified parameters. Return Bezier curve knots. Number of knots is computed automatically.
BranchDescription GenerateBranch(const Vector3& initialPosition, const Vector3& initialDirection, float length, float baseRadius,
    const BranchShapeSettings& shape, unsigned minNumKnots);

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
};

/// Tessellated branch points.
using TessellatedBranchPoints = PODVector<TessellatedBranchPoint>;

/// Tessellate branch with specified quality. Return array of points with position on curve in w component.
TessellatedBranchPoints TessellateBranch(const BezierCurve3D& positions, const BezierCurve1D& radiuses,
    const BranchTessellationQualityParameters& quality);

/// Generate branch geometry vertices.
PODVector<DefaultVertex> GenerateBranchVertices(const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments);

/// Generate branch geometry vertices.
PODVector<unsigned> GenerateBranchIndices(const PODVector<unsigned>& numRadialSegments, unsigned maxVertices);

/// Generate branch geometry.
void GenerateBranchGeometry(ModelFactory& factory, const TessellatedBranchPoints& points,
    const BranchShapeSettings& shape, unsigned numRadialSegments);

/// Tree element distribution settings.
struct TreeElementDistribution
{
    /// Seed of random generator. Re-initializes main generator if non-zero.
    unsigned seed_ = 0;
    /// Number of branches. Set zero to use explicit position and direction.
    unsigned frequency_ = 0;

    /// Branch position (if explicit).
    Vector3 position_;
    /// Branch direction (if explicit).
    Vector3 direction_;

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

    /// Scale of child elements size or length.
    CubicCurveWrapper growthScale_;
    /// Angle between children and parent branch.
    CubicCurveWrapper growthAngle_;
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
    const TreeElementDistribution& distribution, const BranchShapeSettings& shape, unsigned minNumKnots);

/// Leaf shape settings.
struct LeafShapeSettings
{
    /// Size of the leaf is randomly taken from the range.
    FloatRange size_ = 1.0f;
    /// Scale of leaf geometry along dimensions.
    Vector3 scale_ = Vector3::ONE;
    /// Adjusts whether the leaves are orientated in the global world space instead of local space of parent branch. Should be in range [0, 1].
    FloatRange adjustToGlobal_ = 0.0f;
    /// Adjusts whether the leaves are orientated vertically.
    FloatRange alignVertical_ = 0.0f;
    /// Offset of the leaf junction point.
    Vector3 junctionOffset_ = Vector3::ZERO;
    /// Intensity of deformation caused by gravity along specified dimensions.
    Vector3 gravityIntensity_ = Vector3::ZERO;
    /// Measure of frond geometry bending along specified dimensions. Should be in range (0, 1].
    Vector3 gravityResistance_ = Vector3::ONE;
    /// Adjusts intensity of bumping fake leaf normals.
    float bumpNormals_ = 0.0f;
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




/// Tree level of detail description.
struct TreeLodDescription
{
    BranchTessellationQualityParameters branchTessellationQuality_;
};

/// Triangulate branches and fronds.
void TriangulateBranchesAndLeaves(Context* context, const Vector<TreeLodDescription>& lods,
    const Vector<BranchDescription>& branches, const PODVector<LeafDescription>& leaves, unsigned numMaterials);





// 
// /// Read branch group description from XML.
// BranchGroupDescription ReadBranchGroupDescriptionFromXML(const XMLElement& element, const Vector<String>& materials);
// 
// /// Branch Group.
// struct BranchGroup
// {
//     /// Fake groups are not generated and must have only explicit children.
//     bool fake_ = false;
//     /// Description of branch group.
//     BranchGroupDescription desc_;
//     /// Children branch groups.
//     Vector<BranchGroup> children_;
// };
// 
// /// Read branch group from XML.
// BranchGroup ReadBranchGroupFromXML(const XMLElement& element, const Vector<String>& materials);








/// Procedural Tree Factory.
// #TODO Add branch levels
// #TODO Indexed materials
class URHO3D_API TreeFactory : public Object
{
    URHO3D_OBJECT(TreeFactory, Object);

public:
    /// Material parameters.
    struct Material
    {
        /// Material name.
        String materialName_;
        /// Material index.
        int materialIndex_ = 0;
        /// Texture scale.
        Vector2 textureScale_ = Vector2::ONE;
    };
    /// Branch Distribution Type.
    enum class DistributionType
    {
        /// Unknown.
        Unknown,
        /// Generate branches with specified rotation step.
        Alternate,
    };
    /// Alternate Branch Distribution Parameters.
    struct AlternateBranchDistributionParameters
    {
        /// Angle step.
        float angleStep_ = 180.0f;
        /// Angle random.
        float angleRandomness_ = 0.0f;
    };
    /// Branch Distribution.
    struct BranchDistribution
    {
        /// Distribution type.
        DistributionType type_ = DistributionType::Unknown;
        /// Alternate distribution parameters
        AlternateBranchDistributionParameters alternateParam_;
    };
    /// Frond Type.
    enum class FrondType
    {
        /// Simple frond.
        Simple,
        /// Rag-like volume fronds at the end of branch.
        ///
        /// Geometry:
        /// @code
        ///   ___ __     branch is here                  
        /// _|___|__| __/      x
        ///  |___|__|          |_z
        ///     ^              
        ///  branch ends here  
        ///
        ///           branch is here
        ///   /|\  __/ 
        ///  |/|\|
        ///  | | |  y
        ///  |/ \|  |_x
        ///
        ///            branch is here
        ///  ___    __/
        /// |   |\
        /// |___| |  y
        ///      \|  |_z
        ///     ^              
        ///  branch ends here  
        ///
        /// @endcode
        ///
        /// Vertices:
        /// @code
        ///           
        ///  6__7__8  u
        ///  |  |  |  |_v
        ///  |__|__|  
        ///  |3 |4 |5 x
        ///  |__|__|  |_z
        ///  0  1  2
        ///
        /// @endcode
        RagEnding,
        /// Brush-like volume fronds at the end of branch.
        BrushEnding
    };
    /// Frond parameters.
    // #TODO Use vectors
    struct FrondParameters
    {
        /// Factor that adjusts Y axis from local to global space. Should be in range [0, 1].
        float adjustUpToGlobal_ = 0.0f;
        /// Roll rotation.
        float roll_ = 0.0f;
        /// Pitch rotation.
        float pitch_ = 0.0f;
        /// Yaw rotation.
        float yaw_ = 0.0f;
        /// Junction offset.
        Vector3 junctionOffset_ = Vector3::ZERO;
        /// Intensity of deformation caused by gravity along specified dimensions.
        Vector3 gravityIntensity_ = Vector3::ZERO;
        /// Measure of frond geometry bending along specified dimensions. Should be in range (0, 1].
        Vector3 gravityResistance_ = Vector3::ONE;
    };
    /// Rag Ending Frond Parameters.
    struct RagEndingFrondParameters
    {
        /// Absolute offset from branch ending.
        float locationOffset_ = 0.0f;
        /// Offset of the center vertex.
        float centerOffset_ = 0.0f;
        /// Bending factor along Z axis, set 0 to keep horizontal.
        float zBendingFactor_ = 0.5f;
        /// Bending factor along X axis, set 0 to keep horizontal.
        float xBendingFactor_ = 0.5f;

        /// Junction oscillation.
        float junctionOscillation_ = 0.1f;
        /// Edge oscillation.
        float edgeOscillation_ = 1.0f;
    };
    /// Brush Ending Frond Parameters.
    struct BrushEndingFrondParameters
    {
        /// Absolute offset from branch ending.
        float locationOffset_ = 0.0f;
        /// Offset of the center vertex along the normal.
        float centerOffset_ = 0.0f;
        /// Spread angle.
        float spreadAngle_ = 60.0f;

        /// Junction oscillation.
        float junctionOscillation_ = 0.1f;
        /// Edge oscillation.
        float edgeOscillation_ = 1.0f;
    };
    /// Frond Group.
    struct FrondGroup
    {
        /// Frond material.
        Material material_;
        /// Frond type.
        FrondType type_ = FrondType::Simple;
        /// Distribution of frond sizes.
        MathFunctionSPtr size_ = CreateMathFunction("1.0");

        /// Frond parameters.
        FrondParameters param_;
        /// Rag volume ending frond parameters.
        RagEndingFrondParameters ragEndingParam_;
        /// Brush volume ending frond parameters.
        BrushEndingFrondParameters brushEndingParam_;
    };
    /// @brief Single branch frequency constant
    static const unsigned RootGroup = 0xffffffff;
    /// @brief Branch Group Parameters
    /// @note Set @a frequency to SingleBranch to create branch with specified position and direction
    struct BranchGroupParam
    {
        /// Material for branches.
        Material material_;

        /// Seed of random generator. Re-initializes main generator if non-zero.
        unsigned seed_ = 0;
        /// Number of branches. Set to zero to avoid random and use position and direction.
        unsigned frequency_ = 0;
        /// Multiplier for geometry details.
        float geometryMultiplier_ = 1.0f;

        /// Branch position.
        Vector3 position_ = Vector3(0.0f, 0.0f, 0.0f);
        /// Branch direction.
        Vector3 direction_ = Vector3(0.0f, 1.0f, 0.0f);

        BranchDistribution distrib; ///< Branches distribution
        MathFunctionSPtr distribFunction = CreateMathFunction("one"); ///< Branches distribution function
        Vector2 location = Vector2(0.0f, 1.0f); ///< Location on parent branch

        MathFunctionSPtr length = CreateMathFunction("one"); ///< Length function
        MathFunctionSPtr radius = CreateMathFunction("one"); ///< Radius function
        MathFunctionSPtr angle = CreateMathFunction("90"); ///< Angle between branch and parent
        MathFunctionSPtr weight = CreateMathFunction("zero"); ///< Bends branch down

        MathFunctionSPtr branchAdherence = CreateMathFunction("linear"); ///< Adherence of branches in group
        float phaseRandom = 0.0f; ///< Set to randomize branch phase
    };
    /// @brief Branch Group
    class BranchGroup
    {
    public:
        /// @brief Add children branch group
        BranchGroup& addBranchGroup();
        /// @brief Add children frond group
        FrondGroup& addFrondGroup();
        /// @brief Clear
        void clear()
        {
            m_childrenBranchGroups.Clear();
        }
        /// @brief Get branches
        auto branches() const
        {
            return MakePair(m_childrenBranchGroups.Begin(), m_childrenBranchGroups.End());
        }
        /// @brief Get fronds
        auto fronds() const
        {
            return MakePair(m_childrenFrondGroups.Begin(), m_childrenFrondGroups.End());
        }
    public:
        BranchGroupParam p; ///< Parameters
    private:
        Vector<BranchGroup> m_childrenBranchGroups; ///< Children branch groups
        Vector<FrondGroup> m_childrenFrondGroups; ///< Children frond groups
    };
    /// @brief LOD info
    struct LodInfo
    {
        unsigned numLengthSegments_; ///< Number of length segments
        unsigned numRadialSegments_; ///< Number of radial segments
    };
    /// @brief Compute main adherence
    static float computeMainAdherence(const float relativeHeight, const float trunkStrength);
public:
    /// Construct.
    TreeFactory(Context* context);
    /// Destruct.
    ~TreeFactory();

    /// Set trunk strength.
    void SetTrunkStrength(const float trunkStrength);
    /// Set materials names.
    void SetMaterials(const Vector<String>& materials);
    /// Find material index by name.
    int GetMaterialIndex(const String& name) const;
    /// Get number of materials.
    unsigned GetNumMaterials() const;
    /// Set root branch group.
//     void SetRootBranchGroup(const FlexEngine::BranchGroup& root);
    /// Get root branch group.
    BranchGroup& GetRoot();

    /// Generate model using level-of-detail information.
    SharedPtr<Model> GenerateModel(const PODVector<LodInfo>& lodInfos);
private:
    /// @name Generator
    /// @{

    struct Point
    {
        Point() = default;
        Point(const Point& from, const Point& to, const float factor);

        Vector3 position; ///< Position
        float radius; ///< Radius
        float location; ///< Point location on branch

        float distance; ///< Distance from branch begin
        Vector3 direction; ///< This-to-next direction
        float segmentLength; ///< This-to-next distance
        Vector3 zAxis; ///< Average previous-to-this-to-next direction
        Vector3 xAxis; ///< X-axis
        Vector3 yAxis; ///< Y-axis

        float relativeDistance; ///< Relative distance from branch begin

        float branchAdherence; ///< Branch point adherence
        float phase; ///< Branch oscillation phase at point
    };
    struct Branch
    {
        Vector3 position; ///< Real branch position
        Vector3 direction; ///< Real branch direction
        float totalLength = 0.0f; ///< Total branch length
        float location = 1.0f; ///< Relative position on parent branch

        float parentRadius = 1.0f; ///< Parent radius at child location
        float parentLength = 1.0f; ///< Parent length
        float parentBranchAdherence = 0.0f; ///< Parent branch adherence
        float parentPhase = 0.0f; ///< Parent branch phase

        BranchGroup groupInfo; ///< Branch group info
        Vector<Point> points; ///< Branch points
        Vector<Branch> children; ///< Children branches

                                    /// @brief Add point
        void addPoint(const float location, const Vector3& position, const float radius,
                        const float phase, const float adherence);
        /// @brief Get point at specified location
        Point getPoint(const float location) const;
    };
    struct InternalGenerator;
    struct InternalBranchTriangulator;
    struct InternalFrondTriangulator;
    /// @brief Generate skeleton
    /// @param lodInfo
    ///   LOD info
    /// @param [out] trunksArray
    ///   Generated array of trunks
    /// @throw ArgumentException if branch group has no parent
    /// @throw ArgumentException if empty branch group has child group
    void generateSkeleton(const LodInfo& lodInfo, Vector<Branch>& trunksArray) const;
//     /// @brief Triangulate branches
//     unsigned triangulateBranches(unsigned materialIndex, const LodInfo& lodInfo, const Vector<Branch>& trunksArray,
//                                 PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices) const;
//     /// @brief Triangulate fronds
//     unsigned triangulateFronds(unsigned materialIndex, const LodInfo& lodInfo, const Vector<Branch>& trunksArray,
//         PODVector<FatVertex>& vertices, PODVector<FatIndex>& indices) const;
//     /// @brief Compute main adherence
//     void computeMainAdherence(PODVector<FatVertex>& vertices);
//     /// @brief Normalize branch adherence
//     void normalizeBranchesAdherence(PODVector<FatVertex>& vertices);
//     /// @brief Normalize phases
//     void normalizePhases(PODVector<FatVertex>& vertices);
    /// @}
private:
    /// Root of group hierarchy.
//     FlexEngine::BranchGroup REAL_ROOT;
    /// Root of group hierarchy.
    BranchGroup root_;
    /// List of materials.
    Vector<String> materials_;

    /// Trunk strength.
    float trunkStrength_ = 0.5f;
    /// Radius of bounding cylinder.
    float modelRadius_ = 0.0f;
};

/// Initialize tree factory via XML config.
void InitializeTreeFactory(TreeFactory& factory, XMLElement& node);

/// Generate tree from XML config.
void GenerateTreeFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

// 
// /// @brief Parse string as function type
// std::istream& operator >>(std::istream& stm, TreeAsset::FunctionType& value);
// /// @brief Parse string as distribution type
// std::istream& operator >>(std::istream& stm, TreeAsset::DistributionType& value);
// /// @brief Parse string as frond type
// std::istream& operator >>(std::istream& stm, TreeAsset::FrondType& value);
// /// @brief Parse string as function
// std::istream& operator >>(std::istream& stm, TreeAsset::Function& value);
/// @}

}