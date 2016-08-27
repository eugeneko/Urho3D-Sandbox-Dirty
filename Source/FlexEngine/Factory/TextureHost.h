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
class TextureElement;

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

    /// Set preview texture.
    void SetPreviewTexture(SharedPtr<Texture2D> texture);

    /// Set preview material attribute.
    void SetPreviewMaterialAttr(const ResourceRef& value);
    /// Get preview material attribute.
    ResourceRef GetPreviewMaterialAttr() const;

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Material for preview textures.
    SharedPtr<Material> previewMaterial_;

    /// Source of cloned material for preview textures.
    SharedPtr<Material> previewMaterialCached_;
    /// Cloned material for preview textures.
    SharedPtr<Material> clonedPreviewMaterial_;
    /// Generated texture to preview.
    SharedPtr<Texture2D> generatedTextureToPreview_;

    /// Previewed texture element.
    SharedPtr<Texture2D> previewTexture_;
};

/// Element of procedural texture.
class TextureElement : public ProceduralComponentAgent
{
    URHO3D_OBJECT(TextureElement, ProceduralComponentAgent);

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

    /// Mark texture element as dirty.
    void MarkNeedUpdate(bool updatePreview);
    /// Show this in the host's preview.
    void ShowInPreview();
    /// Update this element and its children.
    void Update();
    /// Get generated texture.
    SharedPtr<Texture2D> GetGeneratedTexture() const { return generatedTexture_; }

    /// Set destination texture attribute.
    void SetDestinationTextureAttr(const ResourceRef& value);
    /// Get destination texture attribute.
    ResourceRef GetDestinationTextureAttr() const;

private:
    /// Show this previewed.
    void DoShowInPreview(bool) { ShowInPreview(); }
    /// Implementation of texture generator.
    virtual SharedPtr<Texture2D> DoGenerateTexture() = 0;
    /// Generate texture.
    void GenerateTexture();

protected:
    /// Get host component.
    TextureHost* GetHostComponent();
    /// Get dependencies.
    PODVector<TextureElement*> GetDependencies() const;

private:
    /// Name of destination texture.
    String destinationTextureName_;

    /// Does this texture need update?
    bool dirty_ = true;
    /// Should this element set its texture to preview?
    bool needPreview_ = false;
    /// Source of cloned material for preview textures.
    SharedPtr<Material> previewMaterialCached_;
    /// Cloned material for preview textures.
    SharedPtr<Material> clonedPreviewMaterial_;
    /// Generated texture.
    SharedPtr<Texture2D> generatedTexture_;

    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;
};

/// Rendered model as procedural texture.
class RenderedModelTexture : public TextureElement
{
    URHO3D_OBJECT(RenderedModelTexture, TextureElement);

public:
    /// Max number of input parameters.
    static const unsigned MaxInputParameters = 1;
    /// Max number of input textures.
    static const unsigned MaxInputTextures = 4;

public:
    /// Construct.
    RenderedModelTexture(Context* context);
    /// Destruct.
    virtual ~RenderedModelTexture();
    /// Register object factory.
    static void RegisterObject(Context* context);

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
    /// Set materials attribute.
    void SetMaterialsAttr(const ResourceRefList& value);
    /// Get materials attribute.
    const ResourceRefList& GetMaterialsAttr() const;

private:
    /// Get source model.
    SharedPtr<Model> GetOrCreateModel() const;
    /// Create texture description.
    TextureDescription CreateTextureDescription() const;
    /// Create input texture map.
    HashMap<String, SharedPtr<Texture2D>> CreateInputTextureMap() const;
    /// Generate texture.
    virtual SharedPtr<Texture2D> DoGenerateTexture() override;

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
    /// Name of destination texture.
    String destinationTextureName_;

    /// Material list attribute.
    mutable ResourceRefList materialsAttr_;
};

}
