#include <FlexEngine/Factory/GeometryFactory.h>

#include <FlexEngine/Factory/ModelFactory.h>

#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Math/MathDefs.h>
// #include <FlexEngine/Factory/FactoryContext.h>
// #include <FlexEngine/Factory/FatVertex.h>
// #include <FlexEngine/Factory/GeometryUtils.h>
// #include <FlexEngine/Resource/ResourceCacheHelpers.h>
// 
// #include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
// #include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>
// #include <Urho3D/Resource/XMLFile.h>

namespace FlexEngine
{

void AdjustIndicesBase(unsigned char* indexData, unsigned indexDataSize, bool largeIndices, unsigned baseIndex)
{
    const unsigned numIndices = indexDataSize / (largeIndices ? 4 : 2);
    if (largeIndices)
    {
        AdjustIndicesBase(reinterpret_cast<unsigned*>(indexData), numIndices, baseIndex);
    }
    else
    {
        AdjustIndicesBase(reinterpret_cast<unsigned short*>(indexData), numIndices, baseIndex);
    }
}

//////////////////////////////////////////////////////////////////////////
const PODVector<VertexElement>& DefaultVertex::GetVertexElements()
{
    static_assert(MAX_VERTEX_BONES == 4, "Update vertex elements!");
    static_assert(MAX_VERTEX_TEXCOORD == 4, "Update vertex elements!");
    static const PODVector<VertexElement> elements =
    {
        VertexElement(TYPE_VECTOR3, SEM_POSITION),
        VertexElement(TYPE_VECTOR3, SEM_TANGENT),
        VertexElement(TYPE_VECTOR3, SEM_BINORMAL),
        VertexElement(TYPE_VECTOR3, SEM_NORMAL),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 0),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 1),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 2),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, 3),
        VertexElement(TYPE_VECTOR4, SEM_COLOR, 0),
        VertexElement(TYPE_VECTOR4, SEM_COLOR, 1),
        VertexElement(TYPE_VECTOR4, SEM_COLOR, 2),
        VertexElement(TYPE_VECTOR4, SEM_COLOR, 3),
        VertexElement(TYPE_UBYTE4,  SEM_BLENDINDICES),
        VertexElement(TYPE_VECTOR4, SEM_BLENDWEIGHTS),
    };
    return elements;
}

Vector4 DefaultVertex::GetPackedTangentBinormal() const
{
    return Vector4(tangent_, DotProduct(CrossProduct(tangent_, normal_), binormal_) > 0 ? 1.0f : -1.0f);
}

DefaultVertex LerpVertices(const DefaultVertex& lhs, const DefaultVertex& rhs, float factor)
{
    DefaultVertex result;

    result.position_ = Lerp(lhs.position_, rhs.position_, factor);
    result.tangent_ = Lerp(lhs.tangent_, rhs.tangent_, factor);
    result.binormal_ = Lerp(lhs.binormal_, rhs.binormal_, factor);
    result.normal_ = Lerp(lhs.normal_, rhs.normal_, factor);
    for (unsigned i = 0; i < MAX_VERTEX_TEXCOORD; ++i)
    {
        result.uv_[i] = Lerp(lhs.uv_[i], rhs.uv_[i], factor);
    }
    for (unsigned i = 0; i < MAX_VERTEX_BONES; ++i)
    {
        result.boneIndices_[i] = lhs.boneIndices_[i];
        result.boneWeights_[i] = Lerp(lhs.boneWeights_[i], rhs.boneWeights_[i], factor);
    }

    return result;
}

DefaultVertex QLerpVertices(const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3,
    const float factor1, const float factor2)
{
    return LerpVertices(LerpVertices(v0, v1, factor1), LerpVertices(v2, v3, factor1), factor2);
}

//////////////////////////////////////////////////////////////////////////
ModelFactory::ModelFactory(Context* context)
    : Object(context)
{

}

ModelFactory::~ModelFactory()
{

}

void ModelFactory::Reset()
{
    vertexElements_.Clear();
    vertexSize_ = 0;
    largeIndices_ = false;
    currentLevel_ = 0;
    currentMaterial_ = nullptr;
    geometry_.Clear();
}

void ModelFactory::Initialize(const PODVector<VertexElement>& vertexElements, bool largeIndices)
{
    Reset();

    vertexElements_ = vertexElements;
    largeIndices_ = largeIndices;

    // Use temporary buffer to compute vertex stride
    VertexBuffer buffer(context_);
    buffer.SetSize(0, vertexElements_);
    vertexSize_ = buffer.GetVertexSize();
}

void ModelFactory::SetLevel(unsigned level)
{
    currentLevel_ = level;
}

void ModelFactory::SetMaterial(SharedPtr<Material> material)
{
    currentMaterial_ = material;
}

void ModelFactory::PushNothing()
{
    Vector<ModelGeometryBuffer>& perLevelGeometry = geometry_[currentMaterial_];
    if (currentLevel_ >= perLevelGeometry.Size())
    {
        perLevelGeometry.Resize(currentLevel_ + 1);
    }
}

void ModelFactory::Push(const void* vertexData, unsigned numVertices, const void* indexData, unsigned numIndices, bool adjustIndices)
{
    // Get destination buffers
    PushNothing();
    ModelGeometryBuffer& geometryBuffer = geometry_[currentMaterial_][currentLevel_];

    // Copy vertex data
    geometryBuffer.vertexData.Insert(geometryBuffer.vertexData.End(),
        static_cast<const unsigned char*>(vertexData),
        static_cast<const unsigned char*>(vertexData) + numVertices * GetVertexSize());

    // Copy index data
    geometryBuffer.indexData.Insert(geometryBuffer.indexData.End(),
        static_cast<const unsigned char*>(indexData),
        static_cast<const unsigned char*>(indexData) + numIndices * GetIndexSize());

    // Adjust indices
    if (adjustIndices)
    {
        const unsigned base = geometryBuffer.vertexData.Size() / GetVertexSize() - numVertices;
        const unsigned offset = geometryBuffer.indexData.Size() - numIndices * GetIndexSize();
        AdjustIndicesBase(geometryBuffer.indexData.Buffer() + offset, numIndices * GetIndexSize(), largeIndices_, base);
    }
}

unsigned ModelFactory::GetNumVerticesInBucket() const
{
    // Get destination buffers
    Vector<ModelGeometryBuffer>* perLevelGeometry = geometry_[currentMaterial_];
    if (!perLevelGeometry)
    {
        return 0;
    }

    if (currentLevel_ >= perLevelGeometry->Size())
    {
        return 0;
    }

    ModelGeometryBuffer& geometryBuffer = perLevelGeometry->At(currentLevel_);
    return geometryBuffer.vertexData.Size() / vertexSize_;
}

SharedPtr<Model> ModelFactory::BuildModel(const Vector<SharedPtr<Material>>& materials, const PODVector<float>& distances /*= {}*/) const
{
    // Prepare buffers for accumulated geometry data
    SharedPtr<VertexBuffer> vertexBuffer = MakeShared<VertexBuffer>(context_);
    vertexBuffer->SetShadowed(true);

    SharedPtr<IndexBuffer> indexBuffer = MakeShared<IndexBuffer>(context_);
    indexBuffer->SetShadowed(true);

    SharedPtr<Model> model = MakeShared<Model>(context_);
    model->SetVertexBuffers({ vertexBuffer }, { 0 }, { 0 });
    model->SetIndexBuffers({ indexBuffer });

    // Number of geometries is equal to number of materials
    model->SetNumGeometries(materials.Size());

    for (unsigned i = 0; i < materials.Size(); ++i)
    {
        Vector<ModelGeometryBuffer>* perLodGeometry = geometry_[materials[i]];
        if (!perLodGeometry)
        {
            continue;
        }

        model->SetNumGeometryLodLevels(i, perLodGeometry->Size());
    }

    // Merge all arrays into one
    PODVector<unsigned char> vertexData;
    PODVector<unsigned char> indexData;
    PODVector<unsigned> geometryIndexOffset;
    PODVector<unsigned> geometryIndexCount;

    for (unsigned i = 0; i < materials.Size(); ++i)
    {
        Vector<ModelGeometryBuffer>* perLodGeometry = geometry_[materials[i]];
        if (!perLodGeometry)
        {
            continue;
        }

        for (unsigned j = 0; j < perLodGeometry->Size(); ++j)
        {
            const ModelGeometryBuffer& geometryBuffer = perLodGeometry->At(j);

            // Merge buffers
            geometryIndexOffset.Push(indexData.Size() / GetIndexSize());
            vertexData += geometryBuffer.vertexData;
            indexData += geometryBuffer.indexData;
            geometryIndexCount.Push(geometryBuffer.indexData.Size() / GetIndexSize());

            // Adjust indices
            const unsigned base = (vertexData.Size() - geometryBuffer.vertexData.Size()) / GetVertexSize();
            const unsigned offset = indexData.Size() - geometryBuffer.indexData.Size();
            AdjustIndicesBase(indexData.Buffer() + offset, geometryBuffer.indexData.Size(), largeIndices_, base);

            // Create geometry
            SharedPtr<Geometry> geometry = MakeShared<Geometry>(context_);
            geometry->SetVertexBuffer(0, vertexBuffer);
            geometry->SetIndexBuffer(indexBuffer);
            geometry->SetLodDistance(j < distances.Size() ? distances[j] : 0.0f);
            model->SetGeometry(i, j, geometry);
        }
    }

    // Flush data to buffers
    vertexBuffer->SetSize(vertexData.Size() / GetVertexSize(), vertexElements_);
    vertexBuffer->SetData(vertexData.Buffer());
    indexBuffer->SetSize(indexData.Size() / GetIndexSize(), largeIndices_);
    indexBuffer->SetData(indexData.Buffer());

    // Setup ranges
    unsigned group = 0;
    for (unsigned i = 0; i < materials.Size(); ++i)
    {
        Vector<ModelGeometryBuffer>* perLodGeometry = geometry_[materials[i]];
        if (!perLodGeometry)
        {
            continue;
        }
        for (unsigned lod = 0; lod < perLodGeometry->Size(); ++lod)
        {
            model->GetGeometry(i, lod)->SetDrawRange(TRIANGLE_LIST, geometryIndexOffset[group], geometryIndexCount[group]);
            ++group;
        }
    }

    // Try to compute bounding box.
    int positionOffset = -1;
    for (const VertexElement& element : vertexBuffer->GetElements())
    {
        if (element.semantic_ == SEM_POSITION && element.index_ == 0)
        {
            if (element.type_ != TYPE_VECTOR3 && element.type_ != TYPE_VECTOR4)
            {
                URHO3D_LOGERROR("Position attribute must have type Vector3 or Vector4");
            }
            else
            {
                positionOffset = element.offset_;
            }
            break;
        }
    }
    if (positionOffset < 0)
    {
        URHO3D_LOGERROR("Position was not found");
    }
    else
    {
        const unsigned char* data = vertexBuffer->GetShadowData();
        BoundingBox boundingBox;
        for (unsigned i = 0; i < vertexBuffer->GetVertexCount(); ++i)
        {
            boundingBox.Merge(*reinterpret_cast<const Vector3*>(data + positionOffset + vertexSize_ * i));
        }
        model->SetBoundingBox(boundingBox);
    }

    return model;
}

Vector<SharedPtr<Material>> ModelFactory::GetMaterials() const
{
    Vector<SharedPtr<Material>> result;
    for (const GeometryBufferMap::KeyValue& geometry : geometry_)
    {
        result.Push(geometry.first_);
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
SharedPtr<ModelFactory> CreateModelFromScript(ScriptFile& scriptFile, const String& entryPoint)
{
    SharedPtr<ModelFactory> factory = MakeShared<ModelFactory>(scriptFile.GetContext());
    factory->Initialize(DefaultVertex::GetVertexElements(), true);

    const VariantVector param = { Variant(factory) };
    if (!scriptFile.Execute(ToString("void %s(ModelFactory@ dest)", entryPoint.CString()), param))
    {
        return nullptr;
    }

    return factory;
}

SharedPtr<Model> CreateQuadModel(Context* context)
{
    static const unsigned indices[6] = { 0, 2, 3, 0, 3, 1 };
    static const Vector2 positions[4] = { Vector2(0, 0), Vector2(1, 0), Vector2(0, 1), Vector2(1, 1) };
    DefaultVertex vertices[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        vertices[i].position_ = Vector3(positions[i], 0.5f);
        vertices[i].uv_[0] = Vector4(positions[i].x_, 1.0f - positions[i].y_, 0.0f, 0.0f);
        vertices[i].uv_[1] = Vector4::ONE;
    }

    ModelFactory factory(context);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);
    factory.Push(vertices, indices, true);
    return factory.BuildModel(factory.GetMaterials());
}

}
