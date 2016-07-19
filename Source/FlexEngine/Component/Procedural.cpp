#include <FlexEngine/Component/Procedural.h>

#include <FlexEngine/Factory/ProceduralFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

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
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE(
        "Description", GetDescriptionAttr, SetDescriptionAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Force generation", GetForceGenerationAttr, SetForceGenerationAttr, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Seed", GetSeedAttr, SetSeedAttr, unsigned, 0, AM_DEFAULT);
}

void Procedural::SetDescriptionAttr(const ResourceRef& value)
{
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();
    description_ = resourceCache->GetResource<XMLFile>(value.name_);
    GenerateResources(forceGeneration_, seed_);
}

Urho3D::ResourceRef Procedural::GetDescriptionAttr() const
{
    return GetResourceRef(description_, XMLFile::GetTypeStatic());
}

void Procedural::SetForceGenerationAttr(bool forceGeneration)
{
    forceGeneration_ = forceGeneration;
    GenerateResources(forceGeneration_, seed_);
}

bool Procedural::GetForceGenerationAttr() const
{
    return forceGeneration_;
}

void Procedural::SetSeedAttr(unsigned seed)
{
    seed_ = seed;
    GenerateResources(forceGeneration_, seed_);
}

unsigned Procedural::GetSeedAttr() const
{
    return seed_;
}

void Procedural::GenerateResources(bool forceGeneration, unsigned seed)
{
    if (description_)
    {
        GenerateResourcesFromXML(*description_, forceGeneration, seed);
    }
}

}
