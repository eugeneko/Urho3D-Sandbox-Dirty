#include <FlexEngine/Factory/ProceduralComponent.h>

#include <FlexEngine/Math/Hash.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>
#include <FlexEngine/Resource/ResourceHash.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace FlexEngine
{

namespace
{

/// Convert variant hash to uint32
unsigned GetOptionalHash(const Variant& hash)
{
    return hash.IsEmpty() ? 0 : Max(0u, hash.GetUInt());
}

/// Convert hash to uint32
unsigned GetOptionalHash(const Hash& hash)
{
    return hash.GetHash();
}

/// Check that specified resource exists. If resource doesn't exist, create stub resource and save it on drive.
/// Returns true if matching resource is found.
bool CheckResource(Context* context, const ResourceRef& resourceRef, bool checkHash, unsigned hash)
{
    // Try to find file
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    if (cache->Exists(resourceRef.name_))
    {
        // Try to load resource
        if (Resource* resource = cache->GetResource(resourceRef.type_, resourceRef.name_))
        {
            // Check resource hash
            if (!checkHash || (hash && GetOptionalHash(HashResource(resource)) == hash))
            {
                return true;
            }
        }
    }

    // Create new resource
    if (SharedPtr<Resource> resource = DynamicCast<Resource>(context->CreateObject(resourceRef.type_)))
    {
        resource->SetName(resourceRef.name_);
        InitializeStubResource(resource);
        SaveResource(*resource);
    }
    else
    {
        URHO3D_LOGERROR("Cannot create resource of specified type");
    }

    return false;
}

}

ProceduralSystem::ProceduralSystem(Context* context)
    : Component(context)
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ProceduralSystem, HandleUpdate));
}

ProceduralSystem::~ProceduralSystem()
{
}

void ProceduralSystem::RegisterObject(Context* context)
{
    context->RegisterFactory<ProceduralSystem>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(Component);

    URHO3D_ACCESSOR_ATTRIBUTE("Resource List", GetResourceListAttr, SetResourceListAttr, VariantVector, Variant::emptyVariantVector, AM_FILE | AM_NOEDIT);

    URHO3D_ACCESSOR_ATTRIBUTE("Update Period", GetUpdatePeriod, SetUpdatePeriod, float, 0.1f, AM_DEFAULT);
}

void ProceduralSystem::Update()
{
    for (ProceduralComponent* component : dirtyComponents_)
        component->GenerateResources();
    dirtyComponentsSet_.Clear();
    dirtyComponents_.Clear();
}

void ProceduralSystem::AddResource(ProceduralComponent* component)
{
    if (component)
        components_.Insert(component);
}

void ProceduralSystem::RemoveResource(ProceduralComponent* component)
{
    if (component)
        components_.Erase(component);
}

void ProceduralSystem::MarkComponentDirty(ProceduralComponent* component)
{
    if (component && !dirtyComponentsSet_.Contains(component))
    {
        dirtyComponents_.Push(component);
        dirtyComponentsSet_.Insert(component);
    }
}

void ProceduralSystem::MarkResourceListDirty()
{
    resourceListDirty_ = true;
}

void ProceduralSystem::UpdateResourceList() const
{
    if (resourceListDirty_)
    {
        resourceListDirty_ = false;
        Vector<ResourceRef> list;
        for (ProceduralComponent* component : components_)
            component->EnumerateResources(list);

        resourceList_.Resize(list.Size());
        for (unsigned i = 0; i < list.Size(); ++i)
            resourceList_[i] = list[i];
    }
}

void ProceduralSystem::CheckProceduralResources()
{
    for (const Variant& resource : resourceList_)
        CheckResource(context_, resource.GetResourceRef(), false, 0);
}

void ProceduralSystem::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    elapsedTime_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();
    if (elapsedTime_ >= updatePeriod_ && !dirtyComponents_.Empty())
    {
        elapsedTime_ = 0.0f;
        Update();
    }
}

const VariantVector& ProceduralSystem::GetResourceListAttr() const
{
    UpdateResourceList();
    return resourceList_;
}

void ProceduralSystem::SetResourceListAttr(const VariantVector& list)
{
    resourceList_.Resize(list.Size());
    for (unsigned i = 0; i < list.Size(); ++i)
        resourceList_[i] = list[i].GetResourceRef();

    CheckProceduralResources();
}

//////////////////////////////////////////////////////////////////////////
ProceduralComponent::ProceduralComponent(Context* context)
    : Component(context)
{

}

ProceduralComponent::~ProceduralComponent()
{
    if (proceduralSystem_)
        proceduralSystem_->RemoveResource(this);
}

void ProceduralComponent::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(Component);
    
    URHO3D_ACCESSOR_ATTRIBUTE("Resources Hashes", GetResourcesHashesAttr, SetResourcesHashesAttr, VariantVector, Variant::emptyVariantVector, AM_FILE | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Hash", GetHashAttr, SetHashAttr, unsigned, 0, AM_FILE | AM_NOEDIT);

    URHO3D_TRIGGER_ATTRIBUTE("<Update>", OnUpdateTrigger);
    URHO3D_ACCESSOR_ATTRIBUTE("Seed", GetSeedAttr, SetSeedAttr, unsigned, 0, AM_DEFAULT);
}

void ProceduralComponent::ApplyAttributes()
{
    if (proceduralSystem_ && !resourcesChecked_)
    {
        resourcesChecked_ = true;
        CheckResources();
    }
    MarkParametersDirty();
}

void ProceduralComponent::CheckResources()
{
    Vector<ResourceRef> resourceRefs;
    EnumerateResources(resourceRefs);

    bool dirty = false;
    for (unsigned i = 0; i < resourceRefs.Size(); ++i)
    {
        const unsigned referenceHash = i < resourcesHashes_.Size() ? resourcesHashes_[i].GetUInt() : 0;
        if (!CheckResource(context_, resourceRefs[i], true, referenceHash))
        {
            dirty = true;
            break;
        }
    }

    if (dirty && proceduralSystem_)
    {
        proceduralSystem_->MarkComponentDirty(this);
    }
}

void ProceduralComponent::EnumerateResources(Vector<ResourceRef>& /*resources*/)
{
}

void ProceduralComponent::GenerateResources()
{
    // Generate resources
    Vector<SharedPtr<Resource>> resources;
    DoGenerateResources(resources);

    // Enumerate resources
    Vector<ResourceRef> resourceRefs;
    EnumerateResources(resourceRefs);
    if (resources.Size() != resourceRefs.Size())
    {
        URHO3D_LOGERROR("Mismatch of enumerated and generated resources");
        return;
    }

    // Save
    resourcesHashes_.Resize(resources.Size(), 0u);
    for (unsigned i = 0; i < resources.Size(); ++i)
    {
        if (resources[i] && !resourceRefs[i].name_.Empty())
        {
            resourcesHashes_[i] = Max(1u, HashResource(resources[i]).GetHash());
            resources[i]->SetName(resourceRefs[i].name_);
            SaveResource(*resources[i]);
        }
    }
}

void ProceduralComponent::MarkNeedGeneration()
{
    if (proceduralSystem_)
        proceduralSystem_->MarkComponentDirty(this);
}

void ProceduralComponent::MarkParametersDirty()
{
    if (proceduralSystem_)
    {
        const unsigned newHash = GetOptionalHash(ToHash());
        if (!newHash || newHash != cachedHash_)
        {
            cachedHash_ = newHash;
            proceduralSystem_->MarkComponentDirty(this);
        }
    }
}

void ProceduralComponent::MarkResourceListDirty()
{
    if (proceduralSystem_)
        proceduralSystem_->MarkResourceListDirty();
}

Variant ProceduralComponent::ToHash() const
{
    Hash hash;
    if (!node_ || !ComputeHash(hash))
        return Variant::EMPTY;

    PODVector<ProceduralComponentAgent*> agents;
    node_->GetDerivedComponents(agents, true);
    for (ProceduralComponentAgent* agent : agents)
    {
        const Variant agentHash = agent->ToHash();
        if (agentHash.IsEmpty())
            return Variant::EMPTY;
        hash.HashUInt(agentHash.GetUInt());
    }

    return hash.GetHash();
}

bool ProceduralComponent::ComputeHash(Hash& hash) const
{
    return false;
}

void ProceduralComponent::SetSeedAttr(unsigned seed)
{
    seed_ = seed;
    MarkParametersDirty();
}

unsigned ProceduralComponent::GetSeedAttr() const
{
    return seed_;
}

void ProceduralComponent::DoGenerateResources(Vector<SharedPtr<Resource>>& /*resources*/)
{
}

void ProceduralComponent::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        proceduralSystem_ = scene->GetOrCreateComponent<ProceduralSystem>();
        proceduralSystem_->AddResource(this);
    }
    else if (proceduralSystem_)
    {
        proceduralSystem_->RemoveResource(this);
    }
}

void ProceduralComponent::OnUpdateTrigger(bool)
{
    MarkNeedGeneration();
}

//////////////////////////////////////////////////////////////////////////
ProceduralComponentAgent::ProceduralComponentAgent(Context* context)
    : Component(context)
{

}

ProceduralComponentAgent::~ProceduralComponentAgent()
{

}

void ProceduralComponentAgent::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(Component);

    URHO3D_ACCESSOR_ATTRIBUTE("Hash", GetHashAttr, SetHashAttr, unsigned, 0, AM_FILE | AM_NOEDIT);
}

void ProceduralComponentAgent::ApplyAttributes()
{
    if (resourceListDirty_)
    {
        resourceListDirty_ = false;
        if (ProceduralComponent* parent = GetParent())
            parent->MarkResourceListDirty();
    }

    const unsigned hash = GetOptionalHash(ToHash());
    if (!hash || hash != cachedHash_)
    {
        cachedHash_ = hash;
        if (ProceduralComponent* parent = GetParent())
            parent->MarkParametersDirty();
    }
}

ProceduralComponent* ProceduralComponentAgent::GetParent()
{
    if (node_)
    {
        if (ProceduralComponent* parent = node_->GetDerivedComponent<ProceduralComponent>())
            return parent;
        if (ProceduralComponent* parent = node_->GetParentDerivedComponent<ProceduralComponent>(true))
            return parent;
    }
    return nullptr;
}

void ProceduralComponentAgent::MarkResourceListDirty()
{
    resourceListDirty_ = true;
}

Variant ProceduralComponentAgent::ToHash() const
{
    Hash hash;
    if (ComputeHash(hash))
        return hash.GetHash();
    return Variant::EMPTY;
}

bool ProceduralComponentAgent::ComputeHash(Hash& /*hash*/) const
{
    return false;
}

}
