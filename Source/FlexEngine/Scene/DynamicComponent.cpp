#include <FlexEngine/Scene/DynamicComponent.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace FlexEngine
{

DynamicComponent::DynamicComponent(Context* context)
    : Component(context)
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(DynamicComponent, HandleUpdate));
}

DynamicComponent::~DynamicComponent()
{

}

void DynamicComponent::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(Component);
    URHO3D_TRIGGER_ATTRIBUTE("<Update>", OnUpdate);
    URHO3D_ACCESSOR_ATTRIBUTE("Update Period", GetUpdatePeriodAttr, SetUpdatePeriodAttr, float, 0.1f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Dirty", bool, dirty_, false, AM_DEFAULT | AM_NOEDIT);
}

void DynamicComponent::SetUpdatePeriodAttr(float updatePeriod)
{
    updatePeriod_ = updatePeriod;
}

float DynamicComponent::GetUpdatePeriodAttr() const
{
    return updatePeriod_;
}

void DynamicComponent::MarkNeedUpdate()
{
    dirty_ = true;
}

bool DynamicComponent::DoesNeedUpdate() const
{
    return dirty_;
}

void DynamicComponent::Update(bool forceUpdate /*= true*/)
{
    if (forceUpdate || dirty_)
    {
        DoUpdate();
        dirty_ = false;
    }
}

void DynamicComponent::OnUpdate(bool)
{
    Update();
}

void DynamicComponent::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    elapsedTime_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();
    if (elapsedTime_ >= updatePeriod_ && dirty_)
    {
        elapsedTime_ = 0.0f;
        Update();
    }
}

}
