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
    SharedPtr<ModelFactory> factory = CreateModelFromScript(*script, entryPoint);
    if (!factory)
    {
        URHO3D_LOGERRORF("Failed to call entry point of procedural geometry script '%s'", scriptName.CString());
        return;
    }

    // Build model
    SharedPtr<Model> model = factory->BuildModel();

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
