#include <FlexEngine/Common.h>

namespace Urho3D
{

class ResourceCache;
class XMLElement;
class XMLFile;

}

namespace FlexEngine
{

struct FactoryContext;

/// Generate procedural resources using XML description.
void GenerateResourcesFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

/// Generate procedural resources from XML file.
void GenerateResourcesFromXML(XMLFile& xmlFile, bool forceGeneration, unsigned seed_);

}
