#include <FlexEngine/Component/Procedural.h>

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{

Procedural::Procedural(Context* context)
    : Component(context)
{

}

Procedural::~Procedural()
{

}

void Procedural::RegisterObject(Context* context)
{
    context->RegisterFactory<Procedural>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(Component);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Description",
        GetDescriptionAttr, SetDescriptionAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Force generation", bool, forceGeneration_, false, AM_DEFAULT);
}

void Procedural::SetDescriptionAttr(const ResourceRef& value)
{
    // Load XML config
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();
    description_ = resourceCache->GetResource<XMLFile>(value.name_);
}

Urho3D::ResourceRef Procedural::GetDescriptionAttr() const
{
    return GetResourceRef(description_, XMLFile::GetTypeStatic());
}

void Procedural::ApplyAttributes()
{
}

}
