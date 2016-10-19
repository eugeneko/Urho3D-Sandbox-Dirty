#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Graphics/GrassPatch.h>

#include <Urho3D/Graphics/Drawable.h>

namespace Urho3D
{

class IndexBuffer;
class Terrain;
class VertexBuffer;
class WorkQueue;

}

namespace FlexEngine
{

/// Grass billboard set.
class Grass : public Drawable
{
    URHO3D_OBJECT(Grass, Drawable);

public:
    /// Construct.
    Grass(Context* context);
    /// Destruct.
    virtual ~Grass();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering.
    virtual void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    virtual UpdateGeometryType GetUpdateGeometryType() override { return UPDATE_MAIN_THREAD; }

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    using PatchMap = HashMap<IntVector2, SharedPtr<GrassPatch>>;

    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate() override;
    /// Handle update event and update component if needed.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Make child node name.
    static String MakeChildName(const IntVector2& index);
    /// Get number of vertices in billboard.
    unsigned GetNumBillboardVertices() const;
    /// Calculate optimal patch size by average distance between billboards.
    static int ComputeNumPatches(float terrainSize, float distance, unsigned numVertices);
    /// Compute local relative position.
    static Vector2 ComputeLocalPosition(const BoundingBox& worldBoundingBox, const Vector3& position, int numPatches);
    /// Compute patches region.
    static IntRect ComputePatchRegion(const BoundingBox& worldBoundingBox, const Vector3& position, float distance, int numPatches);
    /// Update bounding box.
    void UpdateBoundingBox();
    /// Update pattern.
    void UpdatePattern();

    /// Update patch.
    static void UpdatePatchAsync(const WorkItem* workItem, unsigned threadIndex);
    /// Handle update patch.
    void HandleUpdatePatchFinished(StringHash eventType, VariantMap& eventData);
    /// Schedule patch update.
    void SchedulePatchUpdate(GrassPatch& patch);
    /// Cancel patch update.
    void CancelPatchUpdate(GrassPatch& patch);
    /// Add patch.
    GrassPatch* AddPatch(const IntVector2& index);
    /// Remove patch.
    PatchMap::Iterator RemovePatch(const IntVector2& index);
    /// Remove all patches.
    void RemoveAllPatches();
    /// Update patches.
    void UpdatePatches(const Vector3& origin);
    /// Update patches if distance between previous origin and new origin is larger than threshold.
    void UpdatePatchesThreshold(const Vector3& origin);
    /// Setup source drawable. Returns true if ready to use.
    bool SetupSource();
    /// Update buffers data.
    void UpdateBuffersData();

private:
    /// Terrain.
    SharedPtr<Terrain> terrain_;
    /// World-space bounding box.
    BoundingBox worldBoundingBox_;
    /// Local-space bounding box.
    BoundingBox boundingBox_;
    /// Pattern.
    PODVector<Vector2> pattern_;
    /// Scale of pattern.
    float patternScale_ = 1.0f;

    /// Work queue.
    WorkQueue* workQueue_;
    /// Whether patches are dirty.
    bool patchesDirty_ = false;
    /// Patches.
    PatchMap patches_;
    /// Current origin.
    Vector3 origin_;
    /// Unused patches.
    Vector<SharedPtr<GrassPatch>> patchesPool_;

    /// Per-instance data.
    Vector4 instanceData_;

    /// Material.
    SharedPtr<Material> material_;
    /// Density.
    float denisty_;
    /// Draw distance.
    float drawDistance_;
    /// Update patches step.
    float updateThreshold_;

};

}
