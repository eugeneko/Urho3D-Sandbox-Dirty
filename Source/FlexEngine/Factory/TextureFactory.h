#include <FlexEngine/Common.h>
#include <FlexEngine/Container/Ptr.h>

namespace Urho3D
{

class Camera;
class Image;
class Material;
class Model;
class Node;
class ResourceCache;
class Texture2D;
class XMLElement;
class XMLFile;

}

namespace FlexEngine
{

/// View description.
struct ViewDescription
{
    /// Node to be rendered.
    SharedPtr<Node> node_;
    /// Camera to render from.
    SharedPtr<Node> camera_;
    /// Render path that is used for rendering.
    SharedPtr<XMLFile> renderPath_;
    /// Destination viewport.
    IntRect viewport_;
};

/// Render views to texture with specified format.
SharedPtr<Texture2D> RenderViews(Context* context, unsigned width, unsigned height, const Vector<ViewDescription>& views);

/// Convert RGBA8 texture to image.
SharedPtr<Image> ConvertTextureToImage(SharedPtr<Texture2D> texture);

/// Orthogonal camera description.
struct OrthoCameraDescription 
{
    /// Camera position.
    Vector3 position_;
    /// Camera direction.
    Quaternion rotation_;
    /// Camera size.
    Vector2 size_;
    /// Far clip plane.
    float farClip_;
    /// Destination viewport.
    IntRect viewport_;
};

Vector<SharedPtr<Image>> RenderStaticModel(Context* context, SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials,
    unsigned width, unsigned height, const Vector<SharedPtr<XMLFile>>& renderPaths, const Vector<OrthoCameraDescription>& cameras);

/// Generate textures using XML description.
void GenerateTexturesFromXML(XMLElement& node, ResourceCache& resourceCache, const String& currentDir, const String& outputDir);

}
