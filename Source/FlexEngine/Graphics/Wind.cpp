#include <FlexEngine/Graphics/Wind.h>

#include <FlexEngine/Core/Attribute.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

const String VSP_WINDDIRECTION("WindDirection");
const String VSP_WINDPARAM("WindParam");

//////////////////////////////////////////////////////////////////////////
WindSystem::WindSystem(Context* context)
    : Component(context)
{

}

WindSystem::~WindSystem()
{

}

void WindSystem::RegisterObject(Context* context)
{
    context->RegisterFactory<WindSystem>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(Component);
}

void WindSystem::AddWindZone(WindZone* windZone)
{
    if (windZone)
    {
        directionalWindZones_.Insert(windZone);
        // #TODO Rewrite it
        directionalWind_ = windZone->Sample();
        SetReferencedMaterialsWind(directionalWind_);
    }
}

void WindSystem::RemoveWindZone(WindZone* windZone)
{
    if (windZone)
        directionalWindZones_.Erase(windZone);
}

void WindSystem::MarkWindZoneDirty(WindZone* windZone)
{
    if (windZone)
    {
        // #TODO Rewrite it
        directionalWind_ = windZone->Sample();
        SetReferencedMaterialsWind(directionalWind_);
    }
}

void WindSystem::ReferenceMaterial(Material* material)
{
    if (material)
    {
        unsigned& counter = ++referencedMaterials_[material];
        if (counter == 1)
            SetMaterialWind(*material, directionalWind_);
    }
}

void WindSystem::DereferenceMaterial(Material* material)
{
    if (material)
    {
        const HashMap<Material*, unsigned>::Iterator iter = referencedMaterials_.Find(material);
        if (iter == referencedMaterials_.End())
            URHO3D_LOGERROR("Try to dereference non-referenced material");
        else
        {
            --iter->second_;
            if (!iter->second_)
                referencedMaterials_.Erase(iter);
        }
    }
}

void WindSystem::SetMaterialWind(Material& material, const WindSample& wind)
{
    material.SetShaderParameter(VSP_WINDDIRECTION, Vector4(wind.direction_, 0.0f));
    material.SetShaderParameter(VSP_WINDPARAM, Vector4(wind.main_, wind.turbulence_, wind.pulseMagnitude_, wind.pulseFrequency_));
}

void WindSystem::SetReferencedMaterialsWind(const WindSample& wind)
{
    for (HashMap<Material*, unsigned>::KeyValue& elem : referencedMaterials_)
    {
        assert(elem.first_);
        SetMaterialWind(*elem.first_, wind);
    }
}

//////////////////////////////////////////////////////////////////////////
WindZone::WindZone(Context* context)
    : Component(context)
{

}

WindZone::~WindZone()
{
    RemoveZoneFromWindSystem();
}

void WindZone::RegisterObject(Context* context)
{
    context->RegisterFactory<WindZone>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(Component);
    URHO3D_MEMBER_ATTRIBUTE("Main", float, main_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Turbulence", float, turbulence_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Pulse Magnitude", float, pulseMagnitude_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Pulse Frequency", float, pulseFrequency_, 0.0f, AM_DEFAULT);
}

void WindZone::AddZoneToWindSystem()
{
    if (windSystem_)
        windSystem_->AddWindZone(this);
}

void WindZone::RemoveZoneFromWindSystem()
{
    if (windSystem_)
        windSystem_->RemoveWindZone(this);
}

WindSample WindZone::Sample(const Vector3& position /*= Vector3::ZERO*/) const
{
    WindSample sample;
    if (node_)
    {
        sample.direction_ = node_->GetDirection();
        sample.main_ = main_;
        sample.turbulence_ = turbulence_;
        sample.pulseMagnitude_ = pulseMagnitude_;
        sample.pulseFrequency_ = pulseFrequency_;
    }
    return sample;
}

void WindZone::ApplyAttributes()
{
    if (windSystem_)
        windSystem_->MarkWindZoneDirty(this);
}

void WindZone::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        Vector3 start = node_->GetWorldPosition();
        Vector3 end = start + node_->GetWorldDirection() * 10.f;
        for (int i = -1; i < 2; ++i)
        {
            for (int j = -1; j < 2; ++j)
            {
                Vector3 offset = Vector3::UP * (5.f * i) + Vector3::RIGHT * (5.f * j);
                debug->AddSphere(Sphere(start + offset, 0.1f), Color::WHITE, depthTest);
                debug->AddLine(start + offset, end + offset, Color::WHITE, depthTest);
            }
        }
    }
}

void WindZone::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        windSystem_ = scene->GetOrCreateComponent<WindSystem>();
        AddZoneToWindSystem();
    }
    else
        RemoveZoneFromWindSystem();
}

void WindZone::OnSetEnabled()
{
    const bool enabled = IsEnabledEffective();
    if (enabled)
        AddZoneToWindSystem();
    else
        RemoveZoneFromWindSystem();
}

}
