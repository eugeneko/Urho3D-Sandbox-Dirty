#include <FlexEngine/Factory/TextureHost.h>

#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>

namespace FlexEngine
{

namespace
{

static const char* textureToPreviewNames[] =
{
    "Output 0",
    "Output 1",
    "Output 2",
    "Output 3",
    "Output 4",
    "Output 5",
    "Output 6",
    "Output 7",
    "Output 8",
    "Output 9",
    "Output 10",
    "Output 11",
    0
};

static const unsigned maxTextureInputs = 16;

static const char* textureInputsNames[] =
{
    "None",
    "Input 0",
    "Input 1",
    "Input 2",
    "Input 3",
    "Input 4",
    "Input 5",
    "Input 6",
    "Input 7",
    "Input 8",
    "Input 9",
    "Input 10",
    "Input 11",
    "Input 12",
    "Input 13",
    "Input 14",
    "Input 15",
    0
};

static_assert(sizeof(textureInputsNames) / sizeof(textureInputsNames[0]) == maxTextureInputs + 2, "Mismatch of texture inputs count");
}

TextureHost::TextureHost(Context* context)
    : ProceduralComponent(context)
    , texturesAttr_(Texture2D::GetTypeStatic())
{
    ResetToDefault();
}

TextureHost::~TextureHost()
{

}

void TextureHost::RegisterObject(Context* context)
{
    context->RegisterFactory<TextureHost>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Description", GetDescriptionAttr, SetDescriptionAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Texture", GetTexturesAttr, SetTexturesAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Preview Material", GetPreviewMaterialAttr, SetPreviewMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("[Texture to preview]", GetTextureToPreviewAttr, SetTextureToPreviewAttr, unsigned, textureToPreviewNames, 0, AM_EDIT);

}

//////////////////////////////////////////////////////////////////////////
void TextureHost::SetDescriptionAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    description_ = cache->GetResource<XMLFile>(value.name_);
    MarkNeedUpdate();
}

ResourceRef TextureHost::GetDescriptionAttr() const
{
    return GetResourceRef(description_, XMLFile::GetTypeStatic());
}

void TextureHost::SetPreviewMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    previewMaterial_ = cache->GetResource<Material>(value.name_);
    UpdateViews();
}

ResourceRef TextureHost::GetPreviewMaterialAttr() const
{
    return GetResourceRef(previewMaterial_, Material::GetTypeStatic());
}

void TextureHost::SetTexturesAttr(const ResourceRefList& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    const unsigned numTextures = value.names_.Size();
    textureNames_.Resize(numTextures);
    textures_.Resize(numTextures);
    for (unsigned i = 0; i < numTextures; ++i)
    {
        textureNames_[i] = value.names_[i];
        if (!textureNames_[i].Empty())
        {
            textures_[i] = cache->GetResource<Texture2D>(textureNames_[i]);
        }
    }

    MarkNeedUpdate();
}

const ResourceRefList& TextureHost::GetTexturesAttr() const
{
    texturesAttr_.names_.Resize(textureNames_.Size());
    for (unsigned i = 0; i < textureNames_.Size(); ++i)
    {
        texturesAttr_.names_[i] = textureNames_[i];
    }

    return texturesAttr_;
}

void TextureHost::DoUpdate()
{
    if (description_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        TextureFactory textureFactory(context_);
        textureFactory.SetName(description_->GetName());
        textureFactory.Load(description_->GetRoot());
        textureFactory.Generate();
        const Vector<SharedPtr<Texture2D>> generatedTextures = textureFactory.GetTextures();
        const unsigned numTextures = generatedTextures.Size();
        textures_.Resize(numTextures);
        textureNames_.Resize(numTextures);
        for (unsigned i = 0; i < numTextures; ++i)
        {
            if (generatedTextures[i])
            {
                if (!textureNames_[i].Empty())
                {
                    // Fairly write image to file and re-load
                    generatedTextures[i]->SetName(textureNames_[i]);
                    const SharedPtr<Image> image = ConvertTextureToImage(*generatedTextures[i]);

                    if (SaveImage(cache, *image))
                    {
                        cache->ReloadResourceWithDependencies(image->GetName());
                    }
                }
                else
                {
                    // Just set temporary texture to allow preview
                    textures_[i] = generatedTextures[i];
                }
            }
        }
        UpdateViews();
    }
}

void TextureHost::UpdateViews()
{
    if (!node_)
    {
        return;
    }

    StaticModel* staticModel = node_->GetComponent<StaticModel>();
    if (staticModel)
    {
        if (sourceMaterial_ != previewMaterial_)
        {
            sourceMaterial_ = previewMaterial_;
            material_ = sourceMaterial_->Clone();
        }

        if (material_)
        {
            material_->SetTexture(TU_DIFFUSE, textureToPreview_ < textures_.Size() ? textures_[textureToPreview_] : nullptr);
            staticModel->SetMaterial(0, material_);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
TextureElement::TextureElement(Context* context)
    : ProceduralComponent(context)
    , materialsAttr_(Material::GetTypeStatic())
{
    ResetToDefault();
}

TextureElement::~TextureElement()
{

}

void TextureElement::RegisterObject(Context* context)
{
    context->RegisterFactory<TextureElement>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);

    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColorAttr, Color, Color::TRANSPARENT, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Width", GetWidthAttr, SetWidthAttr, unsigned, 1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Height", GetHeightAttr, SetHeightAttr, unsigned, 1, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model Script", GetScriptAttr, SetScriptAttr, ResourceRef, ResourceRef(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entry Point", GetEntryPointAttr, SetEntryPointAttr, String, "Main", AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Materials", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Model Position", GetModelPositionAttr, SetModelPositionAttr, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Parameter 0", GetInputParameter0Attr, SetInputParameter0Attr, Vector4, Vector4::ONE, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Input 0", GetInputTexture0Attr, SetInputTexture0Attr, unsigned, textureInputsNames, 0, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Input 1", GetInputTexture1Attr, SetInputTexture1Attr, unsigned, textureInputsNames, 0, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Input 2", GetInputTexture2Attr, SetInputTexture2Attr, unsigned, textureInputsNames, 0, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Input 3", GetInputTexture3Attr, SetInputTexture3Attr, unsigned, textureInputsNames, 0, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Preview Material", GetPreviewMaterialAttr, SetPreviewMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Destination Texture", GetDestinationTextureAttr, SetDestinationTextureAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);

}

void TextureElement::ApplyAttributes()
{
    Update(false);
}

//////////////////////////////////////////////////////////////////////////
void TextureElement::SetRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    renderPath_ = cache->GetResource<XMLFile>(value.name_);
    MarkNeedUpdate();
}

ResourceRef TextureElement::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, XMLFile::GetTypeStatic());
}

void TextureElement::SetModelAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(value.name_);
    if (model_)
    {
        materials_.Resize(model_->GetNumGeometries());
    }
    MarkNeedUpdate();
}

ResourceRef TextureElement::GetModelAttr() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

void TextureElement::SetScriptAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    script_ = cache->GetResource<ScriptFile>(value.name_);
    if (script_)
    {
        materials_.Resize(1);
    }
    MarkNeedUpdate();
}

ResourceRef TextureElement::GetScriptAttr() const
{
    return GetResourceRef(script_, ScriptFile::GetTypeStatic());
}

void TextureElement::SetMaterialsAttr(const ResourceRefList& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    for (unsigned i = 0; i < value.names_.Size(); ++i)
    {
        if (i <= materials_.Size())
        {
            materials_[i] = cache->GetResource<Material>(value.names_[i]);
        }
    }
    MarkNeedUpdate();
}

const ResourceRefList& TextureElement::GetMaterialsAttr() const
{
    materialsAttr_.names_.Resize(materials_.Size());
    for (unsigned i = 0; i < materials_.Size(); ++i)
    {
        materialsAttr_.names_[i] = GetResourceName(materials_[i]);
    }

    return materialsAttr_;
}

void TextureElement::SetPreviewMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    previewMaterial_ = cache->GetResource<Material>(value.name_);
    UpdateViews();
}

ResourceRef TextureElement::GetPreviewMaterialAttr() const
{
    return GetResourceRef(previewMaterial_, Material::GetTypeStatic());
}

void TextureElement::SetDestinationTextureAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    destinationTextureName_ = value.name_;
    MarkNeedUpdate();
}

ResourceRef TextureElement::GetDestinationTextureAttr() const
{
    return ResourceRef(Texture2D::GetTypeStatic(), destinationTextureName_);
}

void TextureElement::DoUpdate()
{
    if (!node_)
    {
        return;
    }

    // Update all dependencies
    const PODVector<TextureElement*> dependencies = GetDependencies();
    for (TextureElement* dependency : dependencies)
    {
        dependency->Update(false);
    }

    // Render texture
    const TextureDescription desc = CreateTextureDescription();
    const HashMap<String, SharedPtr<Texture2D>> inputMap = CreateInputTextureMap();
    generatedTexture_ = RenderTexture(context_, desc, inputMap);
    if (!destinationTextureName_.Empty() && generatedTexture_)
    {
        // Write texture to file and re-load it
        generatedTexture_->SetName(destinationTextureName_);
        const SharedPtr<Image> image = ConvertTextureToImage(*generatedTexture_);

        ResourceCache* cache = GetSubsystem<ResourceCache>();
        if (SaveImage(cache, *image))
        {
            cache->ReloadResourceWithDependencies(destinationTextureName_);
        }
    }

    // Update views
    UpdateViews();
}

SharedPtr<Model> TextureElement::GetOrCreateModel() const
{
    SharedPtr<Model> model = model_;
    if (!model_)
    {
        if (script_)
        {
            SharedPtr<ModelFactory> factory = CreateModelFromScript(*script_, entryPoint_);
            model = factory->BuildModel(factory->GetMaterials());
            if (!model)
            {
                URHO3D_LOGERROR("Failed to create procedural model");
            }
        }
    }
    return model;
}

TextureDescription TextureElement::CreateTextureDescription() const
{
    TextureDescription desc;
    desc.renderPath_ = renderPath_;
    desc.color_ = color_;
    desc.width_ = Max(1u, width_);
    desc.height_ = Max(1u, height_);

    GeometryDescription geometryDesc;
    geometryDesc.model_ = GetOrCreateModel();
    geometryDesc.materials_ = materials_;
    if (geometryDesc.model_)
    {
        desc.geometries_.Push(geometryDesc);
    }

    OrthoCameraDescription cameraDesc;
    cameraDesc.position_ = Vector3(0.5f, 0.5f, 0.0f) - modelPosition_;
    cameraDesc.farClip_ = 1.0f;
    cameraDesc.size_ = Vector2(1.0f, 1.0f);
    cameraDesc.viewport_ = IntRect(0, 0, desc.width_, desc.height_);
    desc.cameras_.Push(cameraDesc);

    static const TextureUnit units[MaxInputTextures] = { TU_DIFFUSE, TU_NORMAL, TU_SPECULAR, TU_EMISSIVE };
    for (unsigned i = 0; i < MaxInputTextures; ++i)
    {
        if (inputTexture_[i] && inputTexture_[i] < maxTextureInputs)
        {
            desc.textures_.Populate(units[i], textureInputsNames[inputTexture_[i]]);
        }
    }

    desc.parameters_.Populate("MatDiffColor", inputParameter_[0]);

    return desc;
}

PODVector<TextureElement*> TextureElement::GetDependencies() const
{
    PODVector<TextureElement*> result;
    if (node_)
    {
        for (const SharedPtr<Node> child : node_->GetChildren())
        {
            PODVector<TextureElement*> dependencies;
            child->GetComponents(dependencies);
            result += dependencies;
        }
    }
    return result;
}

HashMap<String, SharedPtr<Texture2D>> TextureElement::CreateInputTextureMap() const
{
    HashMap<String, SharedPtr<Texture2D>> result;
    if (node_)
    {
        const PODVector<TextureElement*> inputs = GetDependencies();
        for (unsigned i = 0; i < Min(maxTextureInputs, inputs.Size()); ++i)
        {
            result.Populate(textureInputsNames[i + 1], inputs[i]->GetGeneratedTexture());
        }
    }
    return result;
}

void TextureElement::UpdateViews()
{
    if (!node_)
    {
        return;
    }

    StaticModel* staticModel = node_->GetComponent<StaticModel>();
    if (staticModel)
    {
        if (previewMaterialCached_ != previewMaterial_)
        {
            previewMaterialCached_ = previewMaterial_;
            clonedPreviewMaterial_ = previewMaterial_->Clone();
        }

        if (clonedPreviewMaterial_)
        {
            clonedPreviewMaterial_->SetTexture(TU_DIFFUSE, generatedTexture_);
            staticModel->SetMaterial(0, clonedPreviewMaterial_);
        }
    }
}

}
