#pragma once

#include <FlexEngine/Common.h>
// #include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/XMLFile.h>

namespace Urho3D
{

}

namespace FlexEngine
{

class ModelFactory;

/// Host component of procedural texture.
class TextureHost : public ProceduralComponent
{
    URHO3D_OBJECT(TextureHost, ProceduralComponent);

public:
    /// Type of camera.
    enum class CameraType
    {
        /// Camera that covers box (0, 0, 0) - (1, 1, 1) and oriented in +Z direction.
        BoundingBoxZp
    };

public:
    /// Construct.
    TextureHost(Context* context);
    /// Destruct.
    virtual ~TextureHost();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set color attribute.
    void SetColorAttr(const Color& color) { color_ = color; }
    /// Get color attribute.
    const Color& GetColorAttr() const { return color_; }
    /// Set width attribute.
    void SetWidthAttr(unsigned width) { width_ = width; }
    /// Get width attribute.
    unsigned GetWidthAttr() const { return width_; }
    /// Set height attribute.
    void SetHeightAttr(unsigned height) { height_ = height; }
    /// Get height attribute.
    unsigned GetHeightAttr() const { return height_; }
    /// Set render path attribute.
    void SetRenderPathAttr(const ResourceRef& value);
    /// Get render path attribute.
    ResourceRef GetRenderPathAttr() const;
    /// Set model attribute.
    void SetModelAttr(const ResourceRef& value);
    /// Get model attribute.
    ResourceRef GetModelAttr() const;
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Get materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;
    /// Set camera type attribute.
    void SetCameraTypeAttr(unsigned cameraType) { cameraType_ = static_cast<CameraType>(cameraType); }
    /// Get camera type attribute.
    unsigned GetCameraTypeAttr() const { return static_cast<unsigned>(cameraType_); }
    /// Set model position attribute.
    void SetModelPositionAttr(const Vector3& modelPosition) { modelPosition_ = modelPosition; }
    /// Get model position attribute.
    const Vector3& GetModelPositionAttr() const { return modelPosition_; }

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override { };

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Base color of texture.
    Color color_ = Color::TRANSPARENT;
    /// Output texture width.
    unsigned width_ = 0;
    /// Output texture height.
    unsigned height_ = 0;
    /// Render path for texture rendering.
    SharedPtr<XMLFile> renderPath_;
    /// Source model.
    SharedPtr<Model> model_;
    /// Source materials.
    Vector<SharedPtr<Material>> materials_;
    /// Camera type.
    CameraType cameraType_;
    /// Base model offset.
    Vector3 modelPosition_;

    /// Generated texture.
    SharedPtr<Texture2D> texture_;

    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;

};

}
