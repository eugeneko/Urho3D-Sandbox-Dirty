#include <FlexEngine/Factory/TextureHost.h>

#include <FlexEngine/Container/Utility.h>
#include <FlexEngine/Core/Attribute.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Math/MathDefs.h>
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

void TextureHost::DoGenerateResources(Vector<SharedPtr<Resource>>& resources)
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
}

TextureElement::~TextureElement()
{

}

void TextureElement::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponentAgent);

    URHO3D_TRIGGER_ATTRIBUTE("<Preview>", DoShowInPreview);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Destination Texture", GetDestinationTextureAttr, SetDestinationTextureAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Adjust Alpha", float, adjustAlpha_, 1.0f, AM_DEFAULT);
}

void TextureElement::ApplyAttributes()
{
    MarkNeedUpdate(true);
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
        node_->GetParent()->GetDerivedComponents(parentElements);
        for (TextureElement* parent : parentElements)
        {
            parent->MarkNeedUpdate(false);
        }

        // Mark host
        if (TextureHost* host = GetHostComponent())
        {
            host->MarkNeedGeneration();
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
            host->MarkNeedGeneration();
        }
    }
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

void TextureElement::SetDestinationTextureAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    destinationTextureName_ = value.name_;
}

ResourceRef TextureElement::GetDestinationTextureAttr() const
{
    return ResourceRef(Texture2D::GetTypeStatic(), destinationTextureName_);
}

void TextureElement::GenerateTexture()
{
    generatedTexture_ = DoGenerateTexture();
    if (!destinationTextureName_.Empty() && generatedTexture_)
    {
        // Write texture to file and re-load it
        generatedTexture_->SetName(destinationTextureName_);
        SharedPtr<Image> image = ConvertTextureToImage(generatedTexture_);
        image->PrecalculateLevels();
        AdjustImageLevelsAlpha(*image, adjustAlpha_);

        ResourceCache* cache = GetSubsystem<ResourceCache>();
        if (SaveImage(cache, *image))
        {
            cache->ReloadResourceWithDependencies(destinationTextureName_);
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

PODVector<TextureElement*> TextureElement::GetDependencies() const
{
    PODVector<TextureElement*> result;
    if (node_)
    {
        for (const SharedPtr<Node> child : node_->GetChildren())
        {
            PODVector<TextureElement*> dependencies;
            child->GetDerivedComponents(dependencies);
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

//////////////////////////////////////////////////////////////////////////
InputTexture::InputTexture(Context* context)
    : TextureElement(context)
{
}

InputTexture::~InputTexture()
{

}

void InputTexture::RegisterObject(Context* context)
{
    context->RegisterFactory<InputTexture>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TextureElement);

    URHO3D_MEMBER_ATTRIBUTE("Color", Color, color_, Color::BLACK, AM_DEFAULT);
}

SharedPtr<Texture2D> InputTexture::DoGenerateTexture()
{
    TextureDescription desc;
    desc.color_ = color_;
    desc.width_ = 1;
    desc.height_ = 1;
    return RenderTexture(context_, desc, TextureMap());
}

//////////////////////////////////////////////////////////////////////////
RenderedModelTexture::RenderedModelTexture(Context* context)
    : TextureElement(context)
    , materials_(1)
    , materialsAttr_(Material::GetTypeStatic())
{
    ResetToDefault();
}

RenderedModelTexture::~RenderedModelTexture()
{

}

void RenderedModelTexture::RegisterObject(Context* context)
{
    context->RegisterFactory<RenderedModelTexture>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TextureElement);

    URHO3D_MEMBER_ATTRIBUTE("Color", Color, color_, Color::BLACK, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Width", unsigned, width_, 1, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Height", unsigned, height_, 1, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model Script", GetScriptAttr, SetScriptAttr, ResourceRef, ResourceRef(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Entry Point", String, entryPoint_, "Main", AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Materials", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Model Position", Vector3, modelPosition_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Parameter 0", Vector4, inputParameter_[0], Vector4::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Input 0", unsigned, inputTexture_[0], textureInputsNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Input 1", unsigned, inputTexture_[1], textureInputsNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Input 2", unsigned, inputTexture_[2], textureInputsNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Input 3", unsigned, inputTexture_[3], textureInputsNames, 0, AM_DEFAULT);
}

void RenderedModelTexture::SetRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    renderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef RenderedModelTexture::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, XMLFile::GetTypeStatic());
}

void RenderedModelTexture::SetModelAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(value.name_);
    if (model_)
    {
        materials_.Resize(model_->GetNumGeometries());
    }
    else
    {
        materials_.Resize(1u);
    }
}

ResourceRef RenderedModelTexture::GetModelAttr() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

void RenderedModelTexture::SetScriptAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    script_ = cache->GetResource<ScriptFile>(value.name_);
}

ResourceRef RenderedModelTexture::GetScriptAttr() const
{
    return GetResourceRef(script_, ScriptFile::GetTypeStatic());
}

void RenderedModelTexture::SetMaterialsAttr(const ResourceRefList& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    for (unsigned i = 0; i < value.names_.Size(); ++i)
    {
        if (i < materials_.Size())
        {
            materials_[i] = cache->GetResource<Material>(value.names_[i]);
        }
    }
}

const ResourceRefList& RenderedModelTexture::GetMaterialsAttr() const
{
    materialsAttr_.names_.Resize(materials_.Size());
    for (unsigned i = 0; i < materials_.Size(); ++i)
    {
        materialsAttr_.names_[i] = GetResourceName(materials_[i]);
    }

    return materialsAttr_;
}

SharedPtr<Model> RenderedModelTexture::GetOrCreateModel() const
{
    SharedPtr<Model> model = model_;
    if (!model_)
    {
        if (script_)
        {
            if (SharedPtr<ModelFactory> factory = CreateModelFromScript(*script_, entryPoint_))
            {
                model = factory->BuildModel();
                if (!model)
                {
                    URHO3D_LOGERROR("Failed to create procedural model");
                }
            }
        }
    }
    return model ? model : GetOrCreateQuadModel(context_);
}

TextureDescription RenderedModelTexture::CreateTextureDescription() const
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

    desc.cameras_.Push(OrthoCameraDescription::Identity(desc.width_, desc.height_, -modelPosition_));

    static const TextureUnit units[MaxInputTextures] = { TU_DIFFUSE, TU_NORMAL, TU_SPECULAR, TU_EMISSIVE };
    for (unsigned i = 0; i < MaxInputTextures; ++i)
    {
        if (inputTexture_[i] && inputTexture_[i] < maxTextureInputs)
        {
            desc.textures_.Populate(units[i], textureInputsNames[inputTexture_[i]]);
        }
    }

    desc.parameters_.Populate(inputParameterUniform[0], inputParameter_[0]);

    return desc;
}

SharedPtr<Texture2D> RenderedModelTexture::DoGenerateTexture()
{
    assert(node_);

    const TextureDescription desc = CreateTextureDescription();
    const TextureMap inputMap = CreateInputTextureMap();
    return RenderTexture(context_, desc, inputMap);
}

//////////////////////////////////////////////////////////////////////////
PerlinNoiseTexture::PerlinNoiseTexture(Context* context)
    : TextureElement(context)
{
    ResetToDefault();
}

PerlinNoiseTexture::~PerlinNoiseTexture()
{

}

void PerlinNoiseTexture::RegisterObject(Context* context)
{
    context->RegisterFactory<PerlinNoiseTexture>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TextureElement);

    URHO3D_MEMBER_ATTRIBUTE("Width", unsigned, width_, 1, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Height", unsigned, height_, 1, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Color 1", Color, firstColor_, Color::BLACK, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Color 2", Color, secondColor_, Color::WHITE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Base Scale", Vector2, baseScale_, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Bias", float, bias_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Range", Vector2, range_, GetVector, SetVector, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Contrast", float, contrast_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Number of Octaves", unsigned, numOctaves_, 1, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Octaves", VariantMap, octaves_, Variant::emptyVariantMap, AM_DEFAULT);
}

void PerlinNoiseTexture::ApplyAttributes()
{
    TextureElement::ApplyAttributes();
    ApplyNumberOfOctaves();
    width_ = Max(1u, width_);
    height_ = Max(1u, height_);
}

void PerlinNoiseTexture::SetRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    renderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef PerlinNoiseTexture::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, XMLFile::GetTypeStatic());
}

void PerlinNoiseTexture::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef PerlinNoiseTexture::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void PerlinNoiseTexture::ApplyNumberOfOctaves()
{
    const unsigned oldSize = octaves_.Size();
    for (unsigned i = oldSize; i < numOctaves_; ++i)
    {
        const float scale = Pow(2.0f, static_cast<float>(i));
        const float magnitude = 1.0f / scale;
        octaves_.Populate(StringHash(i), Vector4(scale, scale, magnitude, 0.0f));
    }
    for (unsigned i = numOctaves_; i < oldSize; ++i)
    {
        octaves_.Erase(StringHash(i));
    }
}

SharedPtr<Texture2D> PerlinNoiseTexture::GenerateOctaveTexture(const Vector2& scale, float seed) const
{
    return GeneratePerlinNoiseOctave(renderPath_, GetOrCreateQuadModel(context_), material_, width_, height_, scale, seed);
}

void PerlinNoiseTexture::AddOctaveToBuffer(unsigned i, PODVector<float>& buffer, float& totalMagnitude) const
{
    // Compute base scale
    const Vector2 textureScale = width_ > height_
        ? Vector2(static_cast<float>(width_) / height_, 1.0f)
        : Vector2(1.0f, static_cast<float>(height_) / width_);

    // Read parameters
    const Variant* varParam = octaves_[StringHash(i)];
    const Vector4& param = varParam ? varParam->GetVector4() : Vector4::ZERO;
    const Vector2 scale(param.x_, param.y_);
    const float magnitude = param.z_;
    const float seed = param.w_;

    // Generate texture
    const SharedPtr<Texture2D> texture = GenerateOctaveTexture(scale * textureScale * baseScale_, seed);
    const SharedPtr<Image> image = texture ? ConvertTextureToImage(texture) : nullptr;
    if (!image)
    {
        return;
    }

    // Write to buffer
    assert(image->GetWidth() == width_);
    assert(image->GetHeight() == height_);
    assert(buffer.Size() == width_ * height_);
    totalMagnitude += magnitude;
    for (unsigned y = 0; y < height_; ++y)
    {
        for (unsigned x = 0; x < width_; ++x)
        {
            const float value = image->GetPixel(x, y).r_;
            buffer[y * width_ + x] += value * magnitude;
        }
    }
}

SharedPtr<Texture2D> PerlinNoiseTexture::DoGenerateTexture()
{
    PODVector<Vector4> octaves;
    for (unsigned i = 0; i < numOctaves_; ++i)
        octaves.Push(octaves_[StringHash(i)].GetVector4() * Vector4(baseScale_.x_, baseScale_.y_, 1.0f, 1.0f));
    return ConvertImageToTexture(GeneratePerlinNoise(renderPath_, GetOrCreateQuadModel(context_), material_, width_, height_,
        firstColor_, secondColor_, octaves, bias_, contrast_, range_));
}

//////////////////////////////////////////////////////////////////////////
FillGapFilter::FillGapFilter(Context* context)
    : TextureElement(context)
{
    ResetToDefault();
}

FillGapFilter::~FillGapFilter()
{

}

void FillGapFilter::RegisterObject(Context* context)
{
    context->RegisterFactory<FillGapFilter>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TextureElement);

    URHO3D_MEMBER_ENUM_ATTRIBUTE("Input Texture", unsigned, inputTextureIndex_, textureInputsNames, 0, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Depth", unsigned, depth_, 1, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Is Transparent", bool, isTransparent_, true, AM_DEFAULT);
}

void FillGapFilter::SetRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    renderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef FillGapFilter::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, XMLFile::GetTypeStatic());
}

void FillGapFilter::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef FillGapFilter::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

SharedPtr<Texture2D> FillGapFilter::DoGenerateTexture()
{
    // Try to get input
    const PODVector<TextureElement*> inputs = GetDependencies();
    if (!inputTextureIndex_ || inputTextureIndex_ > inputs.Size())
    {
        return nullptr;
    }

    const SharedPtr<Texture2D> inputTexture = inputs[inputTextureIndex_ - 1]->GetGeneratedTexture();
    if (!inputTexture)
    {
        return nullptr;
    }

    // Fill gaps
    SharedPtr<Model> model = GetOrCreateQuadModel(context_);
    return ConvertImageToTexture(FillTextureGaps(ConvertTextureToImage(inputTexture), depth_, isTransparent_,
        renderPath_, model, material_, inputParameterUniform[0]));
}

}
