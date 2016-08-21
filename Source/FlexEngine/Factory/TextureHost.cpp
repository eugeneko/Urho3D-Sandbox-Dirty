#include <FlexEngine/Factory/TextureHost.h>

#include <FlexEngine/Factory/ModelFactory.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
// #include <Urho3D/Graphics/StaticModel.h>
// #include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
// #include <Urho3D/Scene/Node.h>

namespace FlexEngine
{

namespace
{

static const char* textureHostCameraTypeNames[] =
{
    "BoundingBox +Z",
    0
};

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

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponentAgent);

    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColorAttr, Color, Color::TRANSPARENT, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Width", GetWidthAttr, SetWidthAttr, unsigned, 1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Height", GetHeightAttr, SetHeightAttr, unsigned, 1, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Source Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Source Material", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Render Camera", GetCameraTypeAttr, SetCameraTypeAttr, unsigned, textureHostCameraTypeNames, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Model Position", GetModelPositionAttr, SetModelPositionAttr, Vector3, Vector3::ZERO, AM_DEFAULT);

}

//////////////////////////////////////////////////////////////////////////
void TextureHost::SetRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    renderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef TextureHost::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, XMLFile::GetTypeStatic());
}

void TextureHost::SetModelAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(value.name_);
    if (model_)
    {
        materials_.Resize(model_->GetNumGeometries());
    }
}

ResourceRef TextureHost::GetModelAttr() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

void TextureHost::SetMaterialsAttr(const ResourceRefList& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    for (unsigned i = 0; i < value.names_.Size(); ++i)
    {
        if (i <= materials_.Size())
        {
            materials_[i] = cache->GetResource<Material>(value.names_[i]);
        }
    }
}

const ResourceRefList& TextureHost::GetMaterialsAttr() const
{
    const unsigned numMaterials = model_ ? model_->GetNumGeometries() : 0;
    materialsAttr_.names_.Resize(numMaterials);
    for (unsigned i = 0; i < numMaterials; ++i)
    {
        materialsAttr_.names_[i] = GetResourceName(materials_[i]);
    }

    return materialsAttr_;
}

}
