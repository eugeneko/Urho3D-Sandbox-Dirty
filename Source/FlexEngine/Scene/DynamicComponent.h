#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Scene/TriggerAttribute.h>

#include <Urho3D/Scene/Component.h>

namespace FlexEngine
{

/// Dynamic component receives update events and could be automatically updated.
class DynamicComponent : public Component, public EnableTriggers
{
    URHO3D_OBJECT(DynamicComponent, Component);

public:
    /// Construct.
    DynamicComponent(Context* context);
    /// Destruct.
    ~DynamicComponent();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Set update period attribute.
    void SetUpdatePeriodAttr(float updatePeriod);
    /// Get update period attribute.
    float GetUpdatePeriodAttr() const;

    /// Mark resource as dirty.
    void MarkNeedUpdate();
    /// Is dirty?
    bool DoesNeedUpdate() const;

    /// Forcibly update component.
    void Update();

private:
    /// Implementation of procedural generator. May be called from a worker thread.
    virtual void DoUpdate() = 0;
    /// Completely re-generate resource.
    void OnUpdate(bool);
    /// Handle update event and update component if needed.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

protected:
    /// Dirty flag is set if component need update.
    bool dirty_ = false;
    /// Update period.
    float updatePeriod_ = 0.1f;

    /// Accumulated time for update.
    float elapsedTime_ = 0.0f;

};

}
