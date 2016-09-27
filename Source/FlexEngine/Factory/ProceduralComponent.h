#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Scene/TriggerAttribute.h>

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{

class Resource;

}

namespace FlexEngine
{

class Hash;
class ProceduralComponent;

/// Procedural resource generation system.
class ProceduralSystem : public Component
{
    URHO3D_OBJECT(ProceduralSystem, Component);

public:
    /// Construct.
    ProceduralSystem(Context* context);
    /// Destruct.
    virtual ~ProceduralSystem();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Update system. Generate all dirty resources.
    void Update();

    /// Set update period.
    void SetUpdatePeriod(float updatePeriod) { updatePeriod_ = updatePeriod; }
    /// Return update period.
    float GetUpdatePeriod() const { return updatePeriod_; }

    /// Add resource.
    void AddResource(ProceduralComponent* component);
    /// Remove resource.
    void RemoveResource(ProceduralComponent* component);
    /// Mark component dirty.
    void MarkComponentDirty(ProceduralComponent* component);
    /// Mark resource list dirty.
    void MarkResourceListDirty();

private:
    /// Handle update event and update component if needed.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Return resource list attribute.
    const VariantVector& GetResourceListAttr() const;
    /// Set resource list attribute.
    void SetResourceListAttr(const VariantVector& list);

    /// Update resource list.
    void UpdateResourceList() const;
    /// Check all procedural resources.
    void CheckProceduralResources();

private:
    /// Procedural components.
    HashSet<ProceduralComponent*> components_;
    /// Set of dirty components.
    HashSet<ProceduralComponent*> dirtyComponents_;

    /// Update period.
    float updatePeriod_ = 0.1f;
    /// Accumulated time for update.
    float elapsedTime_ = 0.0f;

    /// Is resource list dirty?
    mutable bool resourceListDirty_ = false;
    /// Resource list.
    mutable VariantVector resourceList_;
};

/// Base class of host component of procedurally generated resource.
class ProceduralComponent : public Component
{
    URHO3D_OBJECT(ProceduralComponent, Component);

public:
    /// Construct.
    ProceduralComponent(Context* context);
    /// Destruct.
    virtual ~ProceduralComponent();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes();

    /// Check existing resources.
    void CheckResources();
    /// Generate resources.
    void GenerateResources();
    /// Enumerate resources.
    virtual void EnumerateResources(Vector<ResourceRef>& resources);

    /// Mark procedural resource dirty. This always lead to re-generation.
    void MarkNeedGeneration();
    /// Mark parameters dirty.
    void MarkParametersDirty();
    /// Mark resource list dirty.
    void MarkResourceListDirty();
    /// Compute hash of component and all children agents.
    Variant ToHash() const;

    /// Set seed attribute.
    void SetSeedAttr(unsigned seed);
    /// Get seed attribute.
    unsigned GetSeedAttr() const;

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const;
    /// Generate resources.
    virtual void DoGenerateResources(Vector<SharedPtr<Resource>>& resources);

    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene) override;

    /// Always returns false.
    bool GetFalse() const { return false; }
    /// Handle update action.
    void OnUpdateTrigger(bool);

    /// Return resources hashes attribute.
    const VariantVector& GetResourcesHashesAttr() const { return resourcesHashes_; }
    /// Set resources hashes attribute.
    void SetResourcesHashesAttr(const VariantVector& hashes) { resourcesHashes_ = hashes; }
    /// Set hash attribute.
    void SetHashAttr(unsigned hash) { cachedHash_ = hash; }
    /// Get hash attribute.
    unsigned GetHashAttr() const { return cachedHash_; }

private:
    /// Seed for random generation.
    unsigned seed_ = 0; // #TODO Remove

    /// Hashes of generated resources.
    VariantVector resourcesHashes_;
    /// Cached hash.
    unsigned cachedHash_ = 0;

    /// Are resources checked?
    bool resourcesChecked_ = false;
    /// Is resource list dirty?
    bool resourceListDirty_ = false;
    /// Procedural system.
    WeakPtr<ProceduralSystem> proceduralSystem_;
};

/// Base class of agent component of procedurally generated object.
class ProceduralComponentAgent : public Component
{
    URHO3D_OBJECT(ProceduralComponentAgent, Component);

public:
    /// Construct.
    ProceduralComponentAgent(Context* context);
    /// Destruct.
    virtual ~ProceduralComponentAgent();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Get parent component.
    ProceduralComponent* GetParent();

    /// Mark resource list dirty.
    void MarkResourceListDirty();

    /// Compute hash of procedural agent.
    Variant ToHash() const;

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const;

    /// Set hash attribute.
    void SetHashAttr(unsigned hash) { cachedHash_ = hash; }
    /// Get hash attribute.
    unsigned GetHashAttr() const { return cachedHash_; }

private:
    /// Cached hash.
    unsigned cachedHash_ = 0;

    /// Is resource list dirty?
    bool resourceListDirty_ = false;

};

}
