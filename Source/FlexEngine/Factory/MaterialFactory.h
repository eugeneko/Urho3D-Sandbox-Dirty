#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/FatVertex.h>
#include <FlexEngine/Math/MathFunction.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace Urho3D
{

class Model;
class ResourceCache;
class XMLElement;

}

namespace FlexEngine
{

struct FactoryContext;

/// Generate materials from XML config.
void GenerateMaterialsFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

}