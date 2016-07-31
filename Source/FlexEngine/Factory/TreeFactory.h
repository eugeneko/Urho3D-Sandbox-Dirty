#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/FatVertex.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/MathFunction.h>

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
    static VegetationVertex Construct(const FatVertex& vertex);
    /// Returns vertex format.
    static PODVector<VertexElement> Format();
};

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
        /// Degree of deformation caused by gravity along specified dimensions.
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
    /// @brief Triangulate branches
    unsigned triangulateBranches(unsigned materialIndex, const LodInfo& lodInfo, const Vector<Branch>& trunksArray,
                                Vector<FatVertex>& vertices, Vector<unsigned>& indices) const;
    /// @brief Triangulate fronds
    unsigned triangulateFronds(unsigned materialIndex, const LodInfo& lodInfo, const Vector<Branch>& trunksArray,
        Vector<FatVertex>& vertices, Vector<unsigned>& indices) const;
    /// @brief Compute main adherence
    void computeMainAdherence(Vector<FatVertex>& vertices);
    /// @brief Normalize branch adherence
    void normalizeBranchesAdherence(Vector<FatVertex>& vertices);
    /// @brief Normalize phases
    void normalizePhases(Vector<FatVertex>& vertices);
    /// @}
private:
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