#pragma once

#include <FlexEngine/Common.h>
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
    /// Set parameters.
    void SetParameters(const Vector<Vector4>& parameters) { parameters_ = parameters; }
    /// Return parameters.
    const Vector<Vector4>& GetParameters() const { return parameters_; }

    /// Set resources attribute.
    void SetResourcesAttr(const ResourceRefList& resources) { resources_ = resources; MarkResourceListDirty(); }
    /// Return resources attribute.
    const ResourceRefList& GetResourcesAttr() const { return resources_; }
    /// Set number of parameters attribute.
    void SetNumParametersAttr(unsigned numParameters) { parameters_.Resize(numParameters, Vector4::ZERO); }
    /// Return number of parameters attribute.
    unsigned GetNumParametersAttr() const { return parameters_.Size(); }
    /// Set parameters attribute.
    void SetParametersAttr(const VariantMap& parameters);
    /// Return parameters attribute.
    const VariantMap& GetParametersAttr() const;

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const;
    /// Generate resources.
    virtual void DoGenerateResources(Vector<SharedPtr<Resource>>& resources);

    /// Set resources attribute.
    void SetTypeAttr(const StringHash& type) { type_ = type; }
    /// Return resources attribute.
    const StringHash& GetTypeAttr() const { return type_; }

private:
    /// Script.
    SharedPtr<ScriptFile> script_;
    /// Entry point.
    String entryPoint_;
    /// Actual resource type.
    StringHash type_;
    /// Resource names and editor type.
    ResourceRefList resources_;
    /// Input parameters.
    Vector<Vector4> parameters_;

    /// Input parameters attribute.
    mutable VariantMap parametersAttr_;

};

}
