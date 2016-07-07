#include "FlexEnginePlayer.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Resource/ResourceCache.h>

URHO3D_DEFINE_APPLICATION_MAIN(FlexEnginePlayer);

FlexEnginePlayer::FlexEnginePlayer(Context* context) :
    Urho3DPlayer(context)
{
}

/// @{
struct VertexOrig
{
    Vector3 pos;
    Vector2 uv;
    Vector3 normal;
    Vector3 tangnet;
    Vector3 binormal;
    Vector4 wind;
};
struct VertexDest
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
    Vector4 tangent;
};
/// @}

void FlexEnginePlayer::Start()
{
    FileSystem* fileSystem = GetSubsystem<FileSystem>();
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();

    /// @{
    {
        auto file = resourceCache->GetFile("proxy.bin");
        unsigned m_numVertices = file->ReadUInt();
        unsigned m_numIndices = file->ReadUInt();
        unsigned m_vertexStride = file->ReadUInt();
        unsigned m_indexStride = file->ReadUInt();
        unsigned m_indexType = file->ReadUInt();
        unsigned m_numSubsets = file->ReadUInt();
        struct Subset { unsigned offset, numIndices, materialIndex; } m_subsets[8];
        file->Read(m_subsets, sizeof(m_subsets));
        double m_aabb[6];
        file->Read(m_aabb, sizeof(m_aabb));
        float m_uniformBuffer[16];
        file->Read(m_uniformBuffer, sizeof(m_uniformBuffer));

        unsigned m_parameterDataSize = file->ReadUInt();
        char* m_parameterData = new char[m_parameterDataSize];
        file->Read(m_parameterData, m_parameterDataSize);

        unsigned m_vertexDataSize = file->ReadUInt();
        PODVector<VertexOrig> m_vertexData(m_numVertices);
        file->Read(&m_vertexData[0], m_vertexDataSize);

        PODVector<VertexDest> m_vertexData2(m_numVertices);
        for (unsigned idx = 0; idx < m_numVertices; ++idx)
        {
            m_vertexData2[idx].pos = m_vertexData[idx].pos;
            m_vertexData2[idx].normal = m_vertexData[idx].normal;
            m_vertexData2[idx].uv = m_vertexData[idx].uv;
            m_vertexData2[idx].tangent = Vector4(m_vertexData[idx].tangnet, 1.0);
        }

        unsigned m_indexDataSize = file->ReadUInt();
        PODVector<unsigned short> m_indexData(m_numIndices);
        file->Read(&m_indexData[0], m_indexDataSize);

        PODVector<VertexElement> elements;
        //     elements.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));
        //     elements.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
        //     elements.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD));
        //     elements.Push(VertexElement(TYPE_VECTOR4, SEM_TANGENT));
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));
        elements.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD, 0));
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_TANGENT));
        elements.Push(VertexElement(TYPE_VECTOR3, SEM_BINORMAL));
        elements.Push(VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 1));

        Geometry* geometry = new Geometry(context_);
        SharedPtr<VertexBuffer> vertexBuffer(new VertexBuffer(context_));
        vertexBuffer->SetShadowed(true);
        vertexBuffer->SetSize(m_numVertices, elements);
        vertexBuffer->SetData(&m_vertexData[0]);
        geometry->SetVertexBuffer(0, vertexBuffer);

        SharedPtr<IndexBuffer> indexBuffer(new IndexBuffer(context_));
        indexBuffer->SetShadowed(true);
        indexBuffer->SetSize(m_numIndices, false);
        indexBuffer->SetData(&m_indexData[0]);
        geometry->SetIndexBuffer(indexBuffer);

        geometry->SetDrawRange(TRIANGLE_LIST, 0, m_numIndices);

        Model model(context_);
        model.SetNumGeometries(1);
        model.SetGeometry(0, 0, geometry);
        model.SetVertexBuffers({ vertexBuffer }, PODVector<unsigned>{ 0 }, PODVector<unsigned>{ 0 });
        model.SetIndexBuffers({ indexBuffer });
        model.SetBoundingBox(BoundingBox(Vector3(m_aabb[0], m_aabb[1], m_aabb[2]), Vector3(m_aabb[3], m_aabb[4], m_aabb[5])));

        File destFile(context_, fileSystem->GetProgramDir() + "Asset/Architect/proxy.mdl", FILE_WRITE);
        model.Save(destFile);

        auto testModel = resourceCache->GetResource<Model>("Models/Box.mdl");
    }
    /// @}
    Urho3DPlayer::Start();
}
