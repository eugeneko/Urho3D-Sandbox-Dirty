#include <FlexEngine/Factory/GeometryFactory.h>

#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Factory/GeometryUtils.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

namespace FlexEngine
{

namespace
{

/// Load geometry from script.
SharedPtr<XMLFile> ConstructGeometryFromScript(ScriptFile& scriptFile, const String& entryPoint)
{
    SharedPtr<XMLFile> geometry = MakeShared<XMLFile>(scriptFile.GetContext());
    geometry->CreateRoot("geometry");

    const VariantVector param = { Variant(geometry) };
    if (!scriptFile.Execute(ToString("void %s(XMLFile@ dest)", entryPoint.CString()), param))
    {
        return nullptr;
    }

    return geometry;
}

}

SyntheticVertex SyntheticVertex::Construct(const DefaultVertex& vertex)
{
    SyntheticVertex result;
    result.position_ = vertex.position_;
    result.normal_ = vertex.normal_;
    result.tangent_ = vertex.GetPackedTangentBinormal();
    result.uv_ = vertex.uv_[0];
    result.color_ = vertex.uv_[1];
    return result;
}

PODVector<VertexElement> SyntheticVertex::Format()
{
    static const PODVector<VertexElement> format =
    {
        VertexElement(TYPE_VECTOR3, SEM_POSITION),
        VertexElement(TYPE_VECTOR3, SEM_NORMAL),
        VertexElement(TYPE_VECTOR4, SEM_TANGENT),
        VertexElement(TYPE_VECTOR4, SEM_TEXCOORD),
        VertexElement(TYPE_VECTOR4, SEM_COLOR),
    };
    return format;
}

void GenerateTempGeometryFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    // Get destination name
    const String outputModelName = factoryContext.ExpandName(node.GetAttribute("dest"));
    if (outputModelName.Empty())
    {
        URHO3D_LOGERROR("Procedural geometry output file name mustn't be empty");
        return;
    }

    // Skip generation if possible
    const bool alreadyGenerated = !!resourceCache.GetFile(outputModelName);
    if (!factoryContext.forceGeneration_ && alreadyGenerated)
    {
        return;
    }

    // Load script
    const String entryPoint = node.HasAttribute("entry") ? node.GetAttribute("entry") : "Main";
    const String scriptName = factoryContext.ExpandName(node.GetAttribute("script"));
    if (scriptName.Empty())
    {
        URHO3D_LOGERROR("Procedural geometry script name mustn't be empty");
        return;
    }

    resourceCache.ReloadResourceWithDependencies(scriptName);
    SharedPtr<ScriptFile> script(resourceCache.GetResource<ScriptFile>(scriptName));
    if (!script)
    {
        URHO3D_LOGERRORF("Procedural geometry script '%s' was not found", scriptName.CString());
        return;
    }

    // Run script
    SharedPtr<XMLFile> buffer = ConstructGeometryFromScript(*script, entryPoint);
    if (!buffer)
    {
        URHO3D_LOGERRORF("Failed to call entry point of procedural geometry script '%s'", scriptName.CString());
        return;
    }

    // Load geometry
    BoundingBox boundingBox;
    Vector<SyntheticVertex> vertices;
    const XMLElement verticesNode = buffer->GetRoot().GetChild("vertices");
    for (XMLElement vertexNode = verticesNode.GetChild("vertex"); vertexNode; vertexNode = vertexNode.GetNext("vertex"))
    {
        const SyntheticVertex vertex = SyntheticVertex::Construct(DefaultVertex::ConstructFromXML(vertexNode));
        boundingBox.Merge(Vector3(vertex.position_.x_, vertex.position_.y_, vertex.position_.z_));
        vertices.Push(vertex);
    }

    PODVector<unsigned> indices;
    const XMLElement indicesNode = buffer->GetRoot().GetChild("indices");
    for (XMLElement triangleNode = indicesNode.GetChild("triangle"); triangleNode; triangleNode = triangleNode.GetNext("triangle"))
    {
        indices.Push(triangleNode.GetUInt("i0"));
        indices.Push(triangleNode.GetUInt("i1"));
        indices.Push(triangleNode.GetUInt("i2"));
    }

    // Create model
    SharedPtr<VertexBuffer> vertexBuffer = MakeShared<VertexBuffer>(resourceCache.GetContext());
    vertexBuffer->SetShadowed(true);
    vertexBuffer->SetSize(vertices.Size(), SyntheticVertex::Format());
    if (!vertices.Empty())
    {
        vertexBuffer->SetData(vertices.Buffer());
    }

    SharedPtr<IndexBuffer> indexBuffer = MakeShared<IndexBuffer>(resourceCache.GetContext());
    indexBuffer->SetShadowed(true);
    indexBuffer->SetSize(indices.Size(), true);
    if (!indices.Empty())
    {
        indexBuffer->SetData(indices.Buffer());
    }

    SharedPtr<Model> model = MakeShared<Model>(resourceCache.GetContext());
    model->SetVertexBuffers({ vertexBuffer }, { 0 }, { 0 });
    model->SetIndexBuffers({ indexBuffer });

    SharedPtr<Geometry> geometry = MakeShared<Geometry>(resourceCache.GetContext());
    geometry->SetVertexBuffer(0, vertexBuffer);
    geometry->SetIndexBuffer(indexBuffer);
    geometry->SetDrawRange(TRIANGLE_LIST, 0, indices.Size());

    model->SetNumGeometries(1);
    model->SetNumGeometryLodLevels(0, 1);
    model->SetGeometry(0, 0, geometry);

    model->SetBoundingBox(boundingBox);

    // Save model
    // #TODO Add some checks
    const String& outputFileName = factoryContext.outputDirectory_ + outputModelName;
    CreateDirectoriesToFile(resourceCache, outputFileName);
    File outputFile(resourceCache.GetContext(), outputFileName, FILE_WRITE);
    model->Save(outputFile);
    outputFile.Close();
    resourceCache.ReloadResourceWithDependencies(outputModelName);
}

}
