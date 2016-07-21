#include <FlexEngine/Factory/ProceduralFactory.h>

#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Factory/MaterialFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>
#include <FlexEngine/Resource/XMLHelpers.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

namespace FlexEngine
{

namespace
{

void GenerateProceduralFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    const String proceduralFileName = factoryContext.ExpandName(node.GetValue());
    const SharedPtr<XMLFile> proceduralXml(resourceCache.GetResource<XMLFile>(proceduralFileName));
    if (!proceduralXml)
    {
        URHO3D_LOGERRORF("Cannot find linked procedural resource '%s'", proceduralFileName.CString());
        return;
    }

    FactoryContext newFactoryContext;
    newFactoryContext.outputDirectory_ = factoryContext.outputDirectory_;
    newFactoryContext.currentDirectory_ = GetFilePath(proceduralFileName);
    newFactoryContext.forceGeneration_ = node.HasAttribute("force") ? node.GetBool("force") : factoryContext.forceGeneration_;
    newFactoryContext.seed_ = node.HasAttribute("seed") ? node.GetUInt("seed") : factoryContext.seed_;

    GenerateResourcesFromXML(proceduralXml->GetRoot(), resourceCache, newFactoryContext);
}

/// Signature of factory function.
using FactoryFunction = void (*) (XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

/// Map of all factories.
static const HashMap<String, FactoryFunction> factories =
{
    { "procedural", &GenerateProceduralFromXML },

    { "material",   &GenerateMaterialsFromXML  },
    { "texture",    &GenerateTexturesFromXML   },
};

}

void GenerateResourcesFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    // Iterate over elements
    for (XMLElement resourceNode = node.GetChild(); resourceNode; resourceNode = resourceNode.GetNext())
    {
        FactoryFunction* ptr = factories[resourceNode.GetName()];
        if (ptr)
        {
            (**ptr)(resourceNode, resourceCache, factoryContext);
        }
        else
        {
            URHO3D_LOGWARNINGF("Unknown procedural resource type <%s>", resourceNode.GetName().CString());
        }
    }
}

void GenerateResourcesFromXML(XMLFile& xmlFile, bool forceGeneration, unsigned seed_)
{
    if (xmlFile.GetName().Empty())
    {
        URHO3D_LOGERROR("Input XML file must have non-empty name");
        return;
    }

    ResourceCache* resourceCache = xmlFile.GetSubsystem<ResourceCache>();
    if (!resourceCache)
    {
        URHO3D_LOGERROR("Resource cache subsystem must be initialized");
        return;
    }

    if (xmlFile.GetRoot().GetName() != "procedural")
    {
        URHO3D_LOGWARNINGF("XML root node name must be <procedural>, current name is <%s>", xmlFile.GetRoot().GetName());
    }

    FactoryContext factoryContext;
    factoryContext.outputDirectory_ = GetOutputResourceCacheDir(*resourceCache);
    factoryContext.currentDirectory_ = GetFilePath(xmlFile.GetName());
    factoryContext.forceGeneration_ = forceGeneration;
    factoryContext.seed_ = seed_;

    GenerateResourcesFromXML(xmlFile.GetRoot(), *resourceCache, factoryContext);
}

}
