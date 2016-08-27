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

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Preview Material", GetPreviewMaterialAttr, SetPreviewMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);

}

void TextureHost::SetPreviewTexture(SharedPtr<Texture2D> texture)
{
    previewTexture_ = texture;
    UpdateViews();
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

void TextureHost::DoUpdate()
{
    if (node_)
    {
        for (SharedPtr<Node> child : node_->GetChildren())
        {
            PODVector<TextureElement*> elements;
            child->GetDerivedComponents<TextureElement>(elements);
            for (TextureElement* element : elements)
            {
                element->Update();
            }
        }
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
        if (previewMaterialCached_ != previewMaterial_)
        {
            previewMaterialCached_ = previewMaterial_;
            clonedPreviewMaterial_ = previewMaterial_->Clone();
        }

        if (clonedPreviewMaterial_)
        {
            clonedPreviewMaterial_->SetTexture(TU_DIFFUSE, previewTexture_);
            staticModel->SetMaterial(0, clonedPreviewMaterial_);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
TextureElement::TextureElement(Context* context)
    : ProceduralComponentAgent(context)
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

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponentAgent);
    URHO3D_TRIGGER_ATTRIBUTE("<Preview>", DoShowInPreview);
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
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Destination Texture", GetDestinationTextureAttr, SetDestinationTextureAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);

}

void TextureElement::MarkNeedUpdate(bool updatePreview)
{
    dirty_ = true;
    if (updatePreview)
    {
        ShowInPreview();
    }

    if (node_ && node_->GetParent())
    {
        // Mark parents
        PODVector<TextureElement*> parentElements;
        node_->GetParent()->GetComponents(parentElements);
        for (TextureElement* parent : parentElements)
        {
            parent->MarkNeedUpdate(false);
        }

        // Mark host
        if (TextureHost* host = GetHostComponent())
        {
            host->MarkNeedUpdate();
        }
    }
}

void TextureElement::ShowInPreview()
{
    if (node_ && node_->GetParent())
    {
        needPreview_ = true;

        // Mark host
        if (TextureHost* host = GetHostComponent())
        {
            host->MarkNeedUpdate();
        }
    }
}

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

void TextureElement::Update()
{
    if (node_)
    {
        // Update all dependencies
        const PODVector<TextureElement*> dependencies = GetDependencies();
        for (TextureElement* dependency : dependencies)
        {
            dependency->Update();
        }

        // Re-generate if dirty
        if (dirty_)
        {
            dirty_ = false;
            GenerateTexture();
        }

        // Update preview if needed
        if (needPreview_)
        {
            needPreview_ = false;
            if (TextureHost* host = GetHostComponent())
            {
                host->SetPreviewTexture(generatedTexture_);
            }
        }
    }

}

TextureHost* TextureElement::GetHostComponent()
{
    TextureHost* host = node_->GetComponent<TextureHost>();
    if (!host)
    {
        host = node_->GetParentComponent<TextureHost>(true);
    }
    return host;

}

SharedPtr<Model> TextureElement::GetOrCreateModel() const
{
    SharedPtr<Model> model = model_;
    if (!model_)
    {
        if (script_)
        {
            SharedPtr<ModelFactory> factory = CreateModelFromScript(*script_, entryPoint_);
            model = factory ? factory->BuildModel(factory->GetMaterials()) : nullptr;
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

void TextureElement::GenerateTexture()
{
    assert(node_);

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
}

}
