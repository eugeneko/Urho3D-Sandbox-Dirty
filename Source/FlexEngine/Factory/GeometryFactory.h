#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

struct VertexElement;
class ResourceCache;
class XMLElement;

}

namespace FlexEngine
{

struct DefaultVertex;
struct FactoryContext;

/// Generate geometry from XML config.
void GenerateTempGeometryFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

}