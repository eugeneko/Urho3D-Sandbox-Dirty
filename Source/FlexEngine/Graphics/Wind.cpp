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

namespace
{

static const char* windZoneTypesNames[] =
{
    "Directional",
    "Spherical",
    0
};

/// Wind sample accumulator.
struct WindSampleAccumulator
{
    /// Main.
    Vector3 main_;
    /// Turbulence.
    float turbulence_ = 0.0f;
    /// Pulse magnitude.
    Vector3 pulseMagnitude_;

    /// Accumulate wind zone with specified attenuation.
    void Accumulate(const WindSample& sample)
    {
        main_ += sample.direction_ * sample.main_ * sample.attenuation_;
        turbulence_ = Max(turbulence_, sample.turbulence_ * sample.attenuation_);
        pulseMagnitude_ += sample.direction_ * sample.pulseMagnitude_ * sample.attenuation_;
    }

    /// Get result wind sample.
    WindSample GetResult()
    {
        WindSample sample;
        sample.direction_ = main_.Normalized();
        sample.main_ = main_.Length();
        sample.turbulence_ = turbulence_;
        sample.pulseMagnitude_ = pulseMagnitude_.Length();
        return sample;
    }
};

}

//////////////////////////////////////////////////////////////////////////
WindSystem::WindSystem(Context* context)
    : LogicComponent(context)
{

}

WindSystem::~WindSystem()
{

}

void WindSystem::RegisterObject(Context* context)
{
    context->RegisterFactory<WindSystem>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(LogicComponent);
    URHO3D_ACCESSOR_ATTRIBUTE("Pulse Frequency", GetPulseFreqency, SetPulseFrequency, float, 0.0f, AM_DEFAULT | AM_NOEDIT);
}

void WindSystem::Update(float timeStep)
{
    for (WindZone* windZone : directionalWindZones_)
    {
        windZone->CheckTransform();
    }
}

void WindSystem::AddWindZone(WindZone* windZone)
{
    if (windZone)
    {
        if (windZone->GetWindZoneType() == WindZoneType::Directional)
        {
            directionalWindZones_.Insert(windZone);
            UpdateGlobalWind();
        }
        else
            localWindZones_.Insert(windZone);
    }
}

void WindSystem::RemoveWindZone(WindZone* windZone)
{
    if (windZone)
    {
        directionalWindZones_.Erase(windZone);
        localWindZones_.Erase(windZone);
        UpdateGlobalWind();
    }
}

void WindSystem::MarkWindZoneDirty(WindZone* windZone)
{
    if (windZone && windZone->GetWindZoneType() == WindZoneType::Directional)
        UpdateGlobalWind();
}

void WindSystem::ReferenceMaterial(Material* material)
{
    if (material)
        referencedMaterials_.Insert(WeakPtr<Material>(material));
}

Pair<WindSample, bool> WindSystem::GetWindSample(const Vector3& position) const
{
    // Early return if no local wind zones
    Pair<WindSample, bool> result(directionalWind_, false);
    if (!HasLocalWindZones())
        return result;

    // Accumulate local wind zones info
    WindSampleAccumulator accum;
    accum.Accumulate(directionalWind_);
    for (WindZone* windZone : localWindZones_)
    {
        const Pair<WindSample, bool> localWindSample = windZone->GetWindSample(position);
        if (localWindSample.second_)
        {
            result.second_ = true;
            accum.Accumulate(localWindSample.first_);
        }
    }

    result.first_ = accum.GetResult();
    result.first_.pulseFrequency_ = pulseFrequency_;
    return result;
}

void WindSystem::SetMaterialWind(Material& material, const WindSample& wind)
{
    material.SetShaderParameter(VSP_WINDDIRECTION, Vector4(wind.direction_, 0.0f));
    material.SetShaderParameter(VSP_WINDPARAM, Vector4(wind.main_, wind.turbulence_, wind.pulseMagnitude_, wind.pulseFrequency_));
}

void WindSystem::SetReferencedMaterialsWind(const WindSample& wind)
{
    for (ReferencedMaterialSet::Iterator it = referencedMaterials_.Begin(); it != referencedMaterials_.End(); ++it)
    {
        // Remove materials those are not referenced by any object except resource cache.
        if (it->Refs() <= 1)
            it = referencedMaterials_.Erase(it);
        else
            SetMaterialWind(**it, wind);
    }
}

void WindSystem::UpdateGlobalWind()
{
    WindSampleAccumulator accum;
    for (WindZone* windZone : directionalWindZones_)
        accum.Accumulate(windZone->GetWindSample().first_);
    directionalWind_ = accum.GetResult();
    directionalWind_.pulseFrequency_ = pulseFrequency_;
    SetReferencedMaterialsWind(directionalWind_);
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
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Type", GetWindZoneType, SetWindZoneType, WindZoneType, windZoneTypesNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Radius", float, radius_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Main", float, main_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Turbulence", float, turbulence_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Pulse Magnitude", float, pulseMagnitude_, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Pulse Frequency", GetPulseFreqency, SetPulseFrequency, float, 0.0f, AM_EDIT);
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
        switch (type_)
        {
        case WindZoneType::Directional:
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
            break;

        case WindZoneType::Spherical:
            debug->AddSphere(Sphere(node_->GetWorldPosition(), radius_), Color::WHITE, depthTest);
            break;

        }
    }
}

void WindZone::SetWindZoneType(WindZoneType type)
{
    if (type_ != type)
    {
        type_ = type;
        RemoveZoneFromWindSystem();
        AddZoneToWindSystem();
    }
}

void WindZone::CheckTransform()
{
    if (windSystem_ && node_ && type_ == WindZoneType::Directional)
    {
        const Vector3 direction = node_->GetDirection();
        if ((cachedDirection_ - direction).LengthSquared() > M_EPSILON)
        {
            cachedDirection_ = direction;
            windSystem_->MarkWindZoneDirty(this);
        }
    }
}

Pair<WindSample, bool> WindZone::GetWindSample(const Vector3& position /*= Vector3::ZERO*/) const
{
    bool isNonZero = false;
    WindSample sample;
    if (node_)
    {
        switch (type_)
        {
        case WindZoneType::Directional:
            sample.main_ = main_;
            sample.turbulence_ = turbulence_;
            sample.pulseMagnitude_ = pulseMagnitude_;
            sample.direction_ = node_->GetDirection();
            isNonZero = true;
            break;

        case WindZoneType::Spherical:
            {
                const Vector3 direction = position - node_->GetWorldPosition();
                const float reverseAtten = Clamp(direction.Length() / radius_, 0.0f, 1.0f);
                const float mainAtten = 1.0f - reverseAtten;
                sample.main_ = 4 * reverseAtten * mainAtten * main_;
                sample.turbulence_ = mainAtten * turbulence_;
                sample.pulseMagnitude_ = 4 * reverseAtten * mainAtten * pulseMagnitude_;
                sample.direction_ = (direction * Vector3(1, 0, 1)).Normalized();
                isNonZero = mainAtten > 0;
            }
            break;

        default:
            break;
        }
    }
    return MakePair(sample, isNonZero);
}

void WindZone::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        windSystem_ = scene->GetOrCreateComponent<WindSystem>();
        AddZoneToWindSystem();
    }
    else
    {
        RemoveZoneFromWindSystem();
    }
}

void WindZone::OnSetEnabled()
{
    const bool enabled = IsEnabledEffective();
    if (enabled)
        AddZoneToWindSystem();
    else
        RemoveZoneFromWindSystem();
}

void WindZone::AddZoneToWindSystem()
{
    if (windSystem_ && IsEnabledEffective())
        windSystem_->AddWindZone(this);
}

void WindZone::RemoveZoneFromWindSystem()
{
    if (windSystem_)
        windSystem_->RemoveWindZone(this);
}

}
