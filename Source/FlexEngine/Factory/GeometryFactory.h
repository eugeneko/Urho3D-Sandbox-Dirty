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

/// Synthetic vertex type for helper geometry.
struct SyntheticVertex
{
    /// Position.
    Vector3 position_;
    /// Normal.
    Vector3 normal_;
    /// Tangent.
    Vector4 tangent_;
    /// Texture coordinates.
    Vector4 uv_;
    /// Custom data.
    Vector4 color_;

    /// Construct from fat vertex.
    static SyntheticVertex Construct(const DefaultVertex& vertex);
    /// Returns vertex format.
    static PODVector<VertexElement> Format();

};

/// Generate geometry from XML config.
void GenerateTempGeometryFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

}