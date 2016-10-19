#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Graphics/Drawable.h>

namespace Urho3D
{

class IndexBuffer;
class Terrain;
class VertexBuffer;
struct WorkItem;
class WorkQueue;

}

namespace FlexEngine
{

/// Grass patch data.
class GrassPatch : public Drawable
{
    URHO3D_OBJECT(GrassPatch, Drawable);

public:
    /// Construct.
    GrassPatch(Context* context);
    /// Destruct.
    virtual ~GrassPatch();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Set pattern.
    void SetPattern(float scale, const PODVector<Vector2>& pattern);
    /// Set range.
    void SetRange(const Vector3& origin, const Rect& localRange);
    /// Set material.
    void SetMaterial(Material* material);

    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame);

    /// Set work item. Must be called from the host grass component only.
    void SetWorkItem(SharedPtr<WorkItem> item);
    /// Return work item. Must be called from the host grass component only.
    SharedPtr<WorkItem> GetWorkItem() const;
    /// Asynchronously update patch data. May be called from worker thread(s).
    void UpdatePatch(Terrain& terrain);
    /// Finish update patch data.
    void FinishUpdatePatch();

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate() override;

private:
    /// Pattern.
    const PODVector<Vector2>* pattern_ = nullptr;
    /// Scale of pattern.
    float patternScale_ = 1.0f;
    /// Range covered by patch (in local space)
    Rect localRange_;
    /// Origin of local space.
    Vector3 origin_;

    /// Geometry.
    SharedPtr<Geometry> geometry_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Index buffer.
    SharedPtr<IndexBuffer> indexBuffer_;
    /// Per-instance data.
    Vector4 instanceData_;

    /// Work item.
    SharedPtr<WorkItem> workItem_;
    /// Vertex data.
    PODVector<float> vertexData_;
    /// Index data.
    PODVector<unsigned short> indexData_;
};

}
