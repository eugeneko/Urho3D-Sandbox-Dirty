#pragma once

#include <FlexEngine/Common.h>
// #include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/XMLFile.h>

namespace Urho3D
{

}

namespace FlexEngine
{

struct TextureDescription;
class ModelFactory;

/// Host component of procedural texture.
class TextureHost : public ProceduralComponent
{
    URHO3D_OBJECT(TextureHost, ProceduralComponent);

public:
    /// Construct.
    TextureHost(Context* context);
    /// Destruct.
    virtual ~TextureHost();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set description attribute.
    void SetDescriptionAttr(const ResourceRef& value);
    /// Get description attribute.
    ResourceRef GetDescriptionAttr() const;
    /// Set preview material attribute.
    void SetPreviewMaterialAttr(const ResourceRef& value);
    /// Get preview material attribute.
    ResourceRef GetPreviewMaterialAttr() const;
    /// Set texture to preview attribute.
    void SetTextureToPreviewAttr(unsigned textureToPreview) { textureToPreview_ = textureToPreview; UpdateViews(); }
    /// Get texture to preview attribute.
    unsigned GetTextureToPreviewAttr() const { return textureToPreview_; }
    /// Set textures attribute.
    void SetTexturesAttr(const ResourceRefList& value);
    /// Return textures attribute.
    const ResourceRefList& GetTexturesAttr() const;

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Description of procedural textures.
    SharedPtr<XMLFile> description_;
    /// Material for preview textures.
    SharedPtr<Material> previewMaterial_;
    /// Textures names.
    StringVector textureNames_;
    /// Procedural textures.
    Vector<SharedPtr<Texture2D>> textures_;

    /// Current texture to preview.
    unsigned textureToPreview_ = 0;

    /// Source of cloned material for preview textures.
    SharedPtr<Material> sourceMaterial_;
    /// Cloned material for preview textures.
    SharedPtr<Material> material_;

    /// Textures list attribute.
    mutable ResourceRefList texturesAttr_;

};

/// Element of procedural texture.
class TextureElement : public ProceduralComponent
{
    URHO3D_OBJECT(TextureElement, ProceduralComponent);

public:
    /// Max number of input parameters.
    static const unsigned MaxInputParameters = 1;
    /// Max number of input textures.
    static const unsigned MaxInputTextures = 4;

public:
    /// Construct.
    TextureElement(Context* context);
    /// Destruct.
    virtual ~TextureElement();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Get generated texture.
    SharedPtr<Texture2D> GetGeneratedTexture() const { return generatedTexture_; }

    /// Set color attribute.
    void SetColorAttr(const Color& color) { color_ = color; MarkNeedUpdate(); }
    /// Get color attribute.
    const Color& GetColorAttr() const { return color_; }
    /// Set width attribute.
    void SetWidthAttr(unsigned width) { width_ = width; MarkNeedUpdate(); }
    /// Get width attribute.
    unsigned GetWidthAttr() const { return width_; }
    /// Set height attribute.
    void SetHeightAttr(unsigned height) { height_ = height; MarkNeedUpdate(); }
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
    /// Set script attribute.
    void SetScriptAttr(const ResourceRef& value);
    /// Get script attribute.
    ResourceRef GetScriptAttr() const;
    /// Set entry point attribute.
    void SetEntryPointAttr(const String& entryPoint) { entryPoint_ = entryPoint; MarkNeedUpdate(); }
    /// Get entry point attribute.
    const String& GetEntryPointAttr() const { return entryPoint_; }
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Get materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;
    /// Set model position attribute.
    void SetModelPositionAttr(const Vector3& modelPosition) { modelPosition_ = modelPosition; MarkNeedUpdate(); }
    /// Get model position attribute.
    const Vector3& GetModelPositionAttr() const { return modelPosition_; }
    /// Set input parameter0 attribute.
    void SetInputParameter0Attr(const Vector4& inputParameter0) { inputParameter_[0] = inputParameter0; MarkNeedUpdate(); }
    /// Get input parameter0 attribute.
    const Vector4& GetInputParameter0Attr() const { return inputParameter_[0]; }
    /// Set input texture 0 attribute.
    void SetInputTexture0Attr(unsigned inputTexture0) { inputTexture_[0] = inputTexture0; MarkNeedUpdate(); }
    /// Get input texture 0 attribute.
    unsigned GetInputTexture0Attr() const { return inputTexture_[0]; }
    /// Set input texture 1 attribute.
    void SetInputTexture1Attr(unsigned inputTexture1) { inputTexture_[1] = inputTexture1; MarkNeedUpdate(); }
    /// Get input texture 1 attribute.
    unsigned GetInputTexture1Attr() const { return inputTexture_[1]; }
    /// Set input texture 2 attribute.
    void SetInputTexture2Attr(unsigned inputTexture2) { inputTexture_[2] = inputTexture2; MarkNeedUpdate(); }
    /// Get input texture 2 attribute.
    unsigned GetInputTexture2Attr() const { return inputTexture_[2]; }
    /// Set input texture 3 attribute.
    void SetInputTexture3Attr(unsigned inputTexture3) { inputTexture_[3] = inputTexture3; MarkNeedUpdate(); }
    /// Get input texture 3 attribute.
    unsigned GetInputTexture3Attr() const { return inputTexture_[3]; }

    /// Set preview material attribute.
    void SetPreviewMaterialAttr(const ResourceRef& value);
    /// Get preview material attribute.
    ResourceRef GetPreviewMaterialAttr() const;
    /// Set destination texture attribute.
    void SetDestinationTextureAttr(const ResourceRef& value);
    /// Get destination texture attribute.
    ResourceRef GetDestinationTextureAttr() const;

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;
    /// Get source model.
    SharedPtr<Model> GetOrCreateModel() const;
    /// Create texture description.
    TextureDescription CreateTextureDescription() const;
    /// Get dependencies.
    PODVector<TextureElement*> GetDependencies() const;
    /// Create input texture map.
    HashMap<String, SharedPtr<Texture2D>> CreateInputTextureMap() const;

    /// Update views with generated resource.
    void UpdateViews();

private:
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
    /// Source model script.
    SharedPtr<ScriptFile> script_;
    /// Source model script entry point.
    String entryPoint_;
    /// Source materials.
    Vector<SharedPtr<Material>> materials_;
    /// Base model offset.
    Vector3 modelPosition_;
    /// Input parameters.
    Vector4 inputParameter_[MaxInputParameters];
    /// Input textures.
    unsigned inputTexture_[MaxInputTextures];
    /// Material for preview textures.
    SharedPtr<Material> previewMaterial_;
    /// Name of destination texture.
    String destinationTextureName_;

    /// Source of cloned material for preview textures.
    SharedPtr<Material> previewMaterialCached_;
    /// Cloned material for preview textures.
    SharedPtr<Material> clonedPreviewMaterial_;
    /// Generated texture.
    SharedPtr<Texture2D> generatedTexture_;

    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;
};

}
