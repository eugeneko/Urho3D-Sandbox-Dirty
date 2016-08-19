#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Scene/DynamicComponent.h>

namespace FlexEngine
{

/// Base class of host component of procedurally generated object.
class ProceduralComponent : public DynamicComponent
{
    URHO3D_OBJECT(ProceduralComponent, DynamicComponent);

public:
    /// Construct.
    ProceduralComponent(Context* context);
    /// Destruct.
    ~ProceduralComponent();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set seed attribute.
    void SetSeedAttr(unsigned seed);
    /// Get seed attribute.
    unsigned GetSeedAttr() const;

protected:
    /// Seed for random generation.
    unsigned seed_ = 0;

};

/// Base class of agent component of procedurally generated object.
class ProceduralComponentAgent : public Component, public EnableTriggers
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

};

}
