#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

namespace Urho3D
{

class Resource;
class ScriptFile;

}

namespace FlexEngine
{

class Hash;
class ProceduralComponent;

/// Script generation context.
struct ScriptContext
{
    /// Context.
    Context* context_ = nullptr;
    /// Items shared between caller and callee.
    Vector<Variant> items_;
    /// Held objects.
    Vector<SharedPtr<RefCounted>> objects_;
};

/// Base class of host component of procedurally generated resource.
class ScriptedResource : public ProceduralComponent
{
    URHO3D_OBJECT(ScriptedResource, ProceduralComponent);

public:
    /// Construct.
    ScriptedResource(Context* context);
    /// Destruct.
    virtual ~ScriptedResource();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes();
    /// Enumerate resources.
    virtual void EnumerateResources(Vector<ResourceRef>& resources) override;

    /// Set script attribute.
    void SetScriptAttr(const ResourceRef& value);
    /// Return script attribute.
    ResourceRef GetScriptAttr() const;
    /// Set entry point.
    void SetEntryPoint(const String& entryPoint) { entryPoint_ = entryPoint; }
    /// Return entry point.
    const String& GetEntryPoint() const { return entryPoint_; }

    /// Set resources attribute.
    void SetResourcesAttr(const ResourceRefList& resources) { resources_ = resources; MarkResourceListDirty(); }
    /// Return resources attribute.
    const ResourceRefList& GetResourcesAttr() const { return resources_; }

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const;
    /// Generate resources.
    virtual void DoGenerateResources(Vector<SharedPtr<Resource>>& resources);

private:
    /// Script.
    SharedPtr<ScriptFile> script_;
    /// Entry point.
    String entryPoint_;
    /// Resource names and types.
    ResourceRefList resources_;

};

}