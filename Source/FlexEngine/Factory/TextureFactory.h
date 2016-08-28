#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Container/Ptr.h>

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{

enum TextureUnit;
class BoundingBox;
class Camera;
class Image;
class Material;
class Model;
class Node;
class ResourceCache;
class XMLElement;
class XMLFile;

}

namespace FlexEngine
{

struct FactoryContext;

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

    /// Container of temporary objects.
    Vector<SharedPtr<Object>> objects_;
};

/// Render views to texture with specified format.
SharedPtr<Texture2D> RenderViews(Context* context, unsigned width, unsigned height, const Vector<ViewDescription>& views);

/// Convert RGBA8 texture to image.
SharedPtr<Image> ConvertTextureToImage(const Texture2D& texture);

/// Save RGBA8 image to DDS file.
bool SaveImageToDDS(const Image& image, const String& fileName);

/// Save RGBA8 image to BMP,JPG,PNG,TGA or DDS file depending on file extension.
bool SaveImage(ResourceCache* cache, const Image& image);

/// Orthogonal camera description.
struct OrthoCameraDescription
{
    /// Identity camera that covers box from (0, 0, 0) to (1, 1, 1) in Z direction.
    static OrthoCameraDescription Identity(unsigned width, unsigned height, const Vector3& offset = Vector3::ZERO);
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

/// Geometry description.
struct GeometryDescription
{
    /// Model to render.
    SharedPtr<Model> model_;
    /// Material to render.
    Vector<SharedPtr<Material>> materials_;
};

/// Texture description.
struct TextureDescription
{
    /// Fill color.
    Color color_;
    /// Width.
    unsigned width_;
    /// Height.
    unsigned height_;
    /// Render path.
    SharedPtr<XMLFile> renderPath_;
    /// Cameras to render from.
    Vector<OrthoCameraDescription> cameras_;
    /// Geometries to render.
    Vector<GeometryDescription> geometries_;
    /// Texture units to override.
    HashMap<TextureUnit, String> textures_;
    /// Input parameters passed to material.
    HashMap<String, Variant> parameters_;
};

/// Texture mapping from name to object.
using TextureMap = HashMap<String, SharedPtr<Texture2D>>;

/// Construct views for texture description.
Vector<ViewDescription> ConstructViewsForTexture(Context* context, const TextureDescription& desc, const TextureMap& textures);

/// Render texture using description.
SharedPtr<Texture2D> RenderTexture(Context* context, const TextureDescription& desc, const TextureMap& textures);

/// Texture factory.
class URHO3D_API TextureFactory : public Resource
{
    URHO3D_OBJECT(TextureFactory, Resource);

public:
    /// Construct.
    TextureFactory(Context* context);
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad() override;

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);

    /// Add texture.
    bool AddTexture(const String& name, const TextureDescription& desc);
    /// Remove all textures.
    void RemoveAllTextures();
    /// Add output.
    void AddOutput(const String& name, const String& fileName);
    /// Remove all outputs.
    void RemoveAllOutputs();
    /// Check that all outputs exist.
    bool CheckAllOutputs(const String& outputDirectory) const;
    /// Generate textures.
    bool Generate();
    /// Save outputs.
    bool Save(const String& outputDirectory);
    /// Get all generated textures.
    Vector<SharedPtr<Texture2D>> GetTextures() const;

private:
    /// Get texture index.
    int FindTexture(const String& name) const;

private:
    /// Resource cache.
    ResourceCache* resourceCache_ = nullptr;
    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;

    /// Current directory.
    String currectDirectory_;
    /// Textures generation descriptions.
    Vector<Pair<String, TextureDescription>> textureDescs_;
    /// Vector of outputs. Output is a pair of internal texture name and output file name.
    Vector<Pair<String, String>> outputs_;

    /// Map with generated textures.
    HashMap<String, SharedPtr<Texture2D>> textureMap_;
};

/// Generate textures using XML description.
void GenerateTexturesFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext);

}
