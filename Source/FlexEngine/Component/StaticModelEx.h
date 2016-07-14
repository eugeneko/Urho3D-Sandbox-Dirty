#include <FlexEngine/Common.h>
#include <Urho3D/Graphics/StaticModel.h>

namespace Urho3D
{

class Model;

}

namespace FlexEngine
{

/// Static model per-instance data
using StaticModelInstancingData = Vector4[3];

/// Static model per-geometry extra data (extended).
/// @see StaticModelGeometryData
struct StaticModelGeometryDataEx
{
    /// Geometry center.
    Vector3 center_;
    /// Current or fading-in LOD level.
    unsigned primaryLodLevel_;
    /// Fading-out LOD level.
    unsigned secondaryLodLevel_;
    /// LOD level mix factor.
    float lodLevelMixFactor_;
    /// Fade-in and fade-out LOD level distances.
    PODVector<Vector2> lodDistances_;
    /// Instance data for primary LOD level.
    StaticModelInstancingData primaryInstanceData_;
    /// Instance data for primary LOD level.
    StaticModelInstancingData secondaryInstanceData_;
};

/// Static Model (extended).
class StaticModelEx : public StaticModel
{
    URHO3D_OBJECT(StaticModelEx, StaticModel);

public:
    /// Construct.
    StaticModelEx(Context* context);
    /// Destruct.
    ~StaticModelEx();
    /// Register object factory. StaticModel must be registered first.
    static void RegisterObject(Context* context);

    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame) override;

    /// Check that smooth LOD switch is enabled.
    bool IsSmoothLodEnabled() const;
    /// Enable smooth LOD switch. Should be re-called when model or geometry LOD distance changes.
    void EnableSmoothLod(float switchOffsetFactor, float switchTime);

protected:
    /// Reset LOD levels.
    void ResetLodLevels();
    /// Choose LOD levels based on distance.
    void CalculateLodLevels(float timeStep);
    /// Compute best LOD level
    static unsigned ComputeBestLod(float distance, unsigned currentLod, const PODVector<Vector2>& distances);

    /// Extra per-geometry data.
    Vector<StaticModelGeometryDataEx> geometryDataEx_;
    /// Time of LOD switching.
    float lodSwitchDuration_;
    /// Number of LOD switch animations in-process.
    int numSwitchAnimations_;
};

}
