#include <FlexEngine/Graphics/LineRenderer.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{


LineRenderer::LineRenderer(Context* context)
    : Drawable(context, DRAWABLE_GEOMETRY)
    , geometry_(MakeShared<Geometry>(context))
    , vertexBuffer_(MakeShared<VertexBuffer>(context))
    , indexBuffer_(MakeShared<IndexBuffer>(context))
{
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(LineRenderer, HandleEndFrame));

    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);
    batches_.Resize(1);
    batches_[0].distance_ = 0.0f;
    batches_[0].geometry_ = geometry_;
    batches_[0].geometryType_ = GEOM_STATIC;
    batches_[0].numWorldTransforms_ = 1;
    batches_[0].worldTransform_ = &transform_;
}

LineRenderer::~LineRenderer()
{
}

void LineRenderer::RegisterObject(Context* context)
{
    context->RegisterFactory<LineRenderer>(FLEXENGINE_CATEGORY);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable);
}

void LineRenderer::UpdateBatches(const FrameInfo& frame)
{

}

void LineRenderer::UpdateGeometry(const FrameInfo& frame)
{
    if (!lines_.Empty())
    {
        UpdateBufferSize();
        UpdateVertexBuffer();
    }

    geometry_->SetDrawRange(TRIANGLE_LIST, 0, lines_.Size() * 6, false);
}

void LineRenderer::AddLine(const Vector3& start, const Vector3& end, const Color& color, float thickness)
{
    LineDesc line;
    line.start_ = start;
    line.end_ = end;
    line.color_ = color;
    line.thickness_ = thickness;
    lines_.Push(line);
}

void LineRenderer::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = BoundingBox(-M_LARGE_VALUE, M_LARGE_VALUE);
}

void LineRenderer::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    lines_.Clear();
}

void LineRenderer::UpdateBufferSize()
{
    static const float allocationFactor = 1.1f;
    const unsigned newNumElements = static_cast<unsigned>(lines_.Size() * allocationFactor);

    if (vertexBuffer_->GetVertexCount() < lines_.Size() * 4)
    {
        const PODVector<VertexElement> elements =
        {
            VertexElement(TYPE_VECTOR3, SEM_POSITION, 0),
            VertexElement(TYPE_VECTOR3, SEM_POSITION, 1),
            VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0),
            VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 1),
            VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR),
        };
        vertexBuffer_->SetSize(newNumElements * 4, elements, true);
    }

    if (indexBuffer_->GetIndexCount() < lines_.Size() * 6)
    {
        indexBuffer_->SetSize(newNumElements * 6, false, true);

        unsigned short* indexPtr = static_cast<unsigned short*>(indexBuffer_->Lock(0, newNumElements * 6, true));
        if (indexPtr)
        {
            unsigned vertexIndex = 0;
            for (unsigned i = 0; i < newNumElements; ++i)
            {
                indexPtr[0] = (unsigned short)vertexIndex;
                indexPtr[1] = (unsigned short)(vertexIndex + 1);
                indexPtr[2] = (unsigned short)(vertexIndex + 2);
                indexPtr[3] = (unsigned short)(vertexIndex + 2);
                indexPtr[4] = (unsigned short)(vertexIndex + 3);
                indexPtr[5] = (unsigned short)vertexIndex;

                indexPtr += 6;
                vertexIndex += 4;
            }
            indexBuffer_->Unlock();
        }
    }
}

void LineRenderer::UpdateVertexBuffer()
{
    float* dest = static_cast<float*>(vertexBuffer_->Lock(0, lines_.Size() * 4, true));
    if (!dest)
        return;

    for (const LineDesc& line : lines_)
    {
        dest[0] = line.start_.x_;
        dest[1] = line.start_.y_;
        dest[2] = line.start_.z_;
        dest[3] = line.end_.x_;
        dest[4] = line.end_.y_;
        dest[5] = line.end_.z_;
        dest[6] = 0.0f;
        dest[7] = 0.0f;
        dest[8] = -line.thickness_;
        dest[9] = line.thickness_;
        ((unsigned&)dest[10]) = line.color_.ToUInt();
        dest += 11;

        dest[0] = line.end_.x_;
        dest[1] = line.end_.y_;
        dest[2] = line.end_.z_;
        dest[3] = line.start_.x_;
        dest[4] = line.start_.y_;
        dest[5] = line.start_.z_;
        dest[6] = 0.0f;
        dest[7] = 1.0f;
        dest[8] = dest[9] = line.thickness_;
        ((unsigned&)dest[10]) = line.color_.ToUInt();
        dest += 11;

        dest[0] = line.end_.x_;
        dest[1] = line.end_.y_;
        dest[2] = line.end_.z_;
        dest[3] = line.start_.x_;
        dest[4] = line.start_.y_;
        dest[5] = line.start_.z_;
        dest[6] = 1.0f;
        dest[7] = 1.0f;
        dest[8] = -line.thickness_;
        dest[9] = line.thickness_;
        ((unsigned&)dest[10]) = line.color_.ToUInt();
        dest += 11;

        dest[0] = line.start_.x_;
        dest[1] = line.start_.y_;
        dest[2] = line.start_.z_;
        dest[3] = line.end_.x_;
        dest[4] = line.end_.y_;
        dest[5] = line.end_.z_;
        dest[6] = 1.0f;
        dest[7] = 0.0f;
        dest[8] = dest[9] = line.thickness_;
        ((unsigned&)dest[10]) = line.color_.ToUInt();
        dest += 11;
    }
    vertexBuffer_->Unlock();
}

void LineRenderer::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    batches_[0].material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef LineRenderer::GetMaterialAttr() const
{
    return GetResourceRef(batches_[0].material_, Material::GetTypeStatic());
}

}
