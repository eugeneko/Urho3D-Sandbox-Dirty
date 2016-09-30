#include <FlexEngine/Resource/ResourceHash.h>

#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Math/Hash.h>

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/Resource.h>

namespace FlexEngine
{

Hash HashResource(Resource* resource)
{
    Hash hash;
    if (resource->IsInstanceOf<Model>())
    {
        Model* model = static_cast<Model*>(resource);

        hash.HashUInt(model->GetNumGeometries());
        for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
        {
            hash.HashUInt(model->GetNumGeometryLodLevels(i));
            for (unsigned j = 0; j < model->GetNumGeometryLodLevels(i); ++j)
            {
                Geometry* geom = model->GetGeometry(i, j);
                hash.HashUInt(geom->GetVertexStart());
                hash.HashUInt(geom->GetVertexCount());
                hash.HashUInt(geom->GetIndexStart());
                hash.HashUInt(geom->GetIndexCount());
                hash.HashFloat(geom->GetLodDistance());
                hash.HashUInt(geom->GetPrimitiveType());
                hash.HashUInt(geom->GetNumVertexBuffers());
            }
        }

        const Vector<SharedPtr<VertexBuffer>>& vertexBuffers = model->GetVertexBuffers();
        hash.HashUInt(vertexBuffers.Size());
        for (unsigned i = 0; i < vertexBuffers.Size(); ++i)
        {
            hash.HashUInt64(vertexBuffers[i]->GetBufferHash(0));
            hash.HashUInt(vertexBuffers[i]->GetVertexCount());
            hash.HashUInt(vertexBuffers[i]->GetVertexSize());
        }

        const Vector<SharedPtr<IndexBuffer>>& indexBuffers = model->GetIndexBuffers();
        hash.HashUInt(indexBuffers.Size());
        for (unsigned i = 0; i < indexBuffers.Size(); ++i)
        {
            hash.HashUInt(indexBuffers[i]->GetIndexCount());
            hash.HashUInt(indexBuffers[i]->GetIndexSize());
        }
    }
    else if (resource->IsInstanceOf<Image>())
    {
        Image* image = static_cast<Image*>(resource);

        hash.HashUInt(image->GetWidth());
        hash.HashUInt(image->GetHeight());
        hash.HashUInt(image->GetDepth());
        hash.HashUInt(image->GetCompressedFormat());
        hash.HashUInt(image->GetComponents());
    }
    else if (resource->IsInstanceOf<Texture2D>())
    {
        Texture2D* texture = static_cast<Texture2D*>(resource);

        hash.HashUInt(texture->GetFormat());
        hash.HashUInt(texture->GetWidth());
        hash.HashUInt(texture->GetHeight());
        hash.HashUInt(texture->GetLevels());
        hash.HashUInt(texture->GetComponents());
        hash.HashUInt(texture->GetAnisotropy());
        hash.HashUInt(texture->GetAddressMode(COORD_U));
        hash.HashUInt(texture->GetAddressMode(COORD_V));
        hash.HashColor(texture->GetBorderColor());
    }
    return hash;
}

void InitializeStubResource(Resource* resource)
{
    Context* context = resource->GetContext();
    if (resource->IsInstanceOf<Model>())
    {
        SharedPtr<VertexBuffer> vertexBuffer = MakeShared<VertexBuffer>(context);
        vertexBuffer->SetShadowed(true);
        vertexBuffer->SetSize(0, 1 << ELEMENT_POSITION);
        SharedPtr<IndexBuffer> indexBuffer = MakeShared<IndexBuffer>(context);
        indexBuffer->SetShadowed(true);
        indexBuffer->SetSize(0, false);

        Model* model = static_cast<Model*>(resource);
        model->SetNumGeometries(NUM_DEFAULT_MODEL_GEOMETRIES);
        model->SetVertexBuffers({ vertexBuffer }, { 0 }, { 0 });
        model->SetIndexBuffers({ indexBuffer });
        for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
            model->SetGeometry(i, 0, new Geometry(context));
    }
    else if (resource->IsInstanceOf<Image>())
    {
        Image* image = static_cast<Image*>(resource);
        image->SetSize(1, 1, 4);
    }
}

}
