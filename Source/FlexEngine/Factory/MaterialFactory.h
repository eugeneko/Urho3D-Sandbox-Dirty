#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

class ResourceCache;
class XMLElement;

}

namespace FlexEngine
{

struct FactoryContext;

/// Generate materials from XML config.
void GenerateMaterialsFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

}