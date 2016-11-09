#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Graphics/Drawable.h>

namespace Urho3D
{

class Material;
class IndexBuffer;
class VertexBuffer;

}

namespace FlexEngine
{

/// Line rendering system.
class LineRenderer : public Drawable
{
    URHO3D_OBJECT(LineRenderer, Drawable);

public:
    /// Construct.
    LineRenderer(Context* context);
    /// Destruct.
    virtual ~LineRenderer();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering.
    virtual void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    virtual UpdateGeometryType GetUpdateGeometryType() override { return UPDATE_MAIN_THREAD; }

    /// Add line for rendering.
    void AddLine(const Vector3& start, const Vector3& end, const Color& color, float thickness);

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate() override;

    /// Handle end frame.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);
    /// Update size of buffers.
    void UpdateBufferSize();
    /// Update vertex buffer data.
    void UpdateVertexBuffer();

private:
    /// Line description.
    struct LineDesc
    {
        Vector3 start_;
        Vector3 end_;
        Color color_;
        float thickness_;
    };

    /// Lines to draw.
    PODVector<LineDesc> lines_;

    /// Geometry.
    SharedPtr<Geometry> geometry_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Index buffer.
    SharedPtr<IndexBuffer> indexBuffer_;
    /// Transform.
    Matrix3x4 transform_ = Matrix3x4::IDENTITY;
};

}
