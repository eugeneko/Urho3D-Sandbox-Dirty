#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class Material;

namespace FlexEngine
{

class WindObject;
class WindZone;

/// Wind direction uniform. Name is 'WindDirection'. XYZ is wind direction. Y component is ignored.
extern const String VSP_WINDDIRECTION;
/// Wind parameters uniform. Name is 'WindParam'. X is main wind. Y is turbulence. Z is pulse magnitude. W is pulse frequency.
extern const String VSP_WINDPARAM;

/// Wind sample data.
struct WindSample
{
    /// Attenuation coefficient of wind sample.
    float attenuation_ = 1.0f;
    /// Direction.
    Vector3 direction_;
    /// Main wind.
    float main_ = 0.0f;
    /// Turbulence.
    float turbulence_ = 0.0f;
    /// Pulse magnitude.
    float pulseMagnitude_ = 0.0f;
    /// Pulse frequency.
    float pulseFrequency_ = 0.0f;
};

/// Wind system.
class WindSystem : public LogicComponent
{
    URHO3D_OBJECT(WindSystem, LogicComponent);

public:
    /// Construct.
    WindSystem(Context* context);
    /// Destruct.
    virtual ~WindSystem();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Called on scene update, variable timestep.
    virtual void Update(float timeStep) override;

    /// Set pulse frequency.
    void SetPulseFrequency(float pulseFrequency) { pulseFrequency_ = pulseFrequency; }
    /// Get pulse frequency.
    float GetPulseFreqency() const { return pulseFrequency_; }

    /// Add wind zone.
    void AddWindZone(WindZone* windZone);
    /// Remove wind zone.
    void RemoveWindZone(WindZone* windZone);
    /// Mark wind zone dirty.
    void MarkWindZoneDirty(WindZone* windZone);

    /// Reference material.
    void ReferenceMaterial(Material* material);
    /// Return whether there are any local wind zones.
    bool HasLocalWindZones() const { return !localWindZones_.Empty(); }
    /// Return wind sample at specified world position. Flag shows whether the local wind zones affect specified position.
    Pair<WindSample, bool> GetWindSample(const Vector3& position = Vector3::ZERO) const;
    /// Set wind parameters to material.
    static void SetMaterialWind(Material& material, const WindSample& wind);

private:
    /// Set wind parameters to all referenced materials.
    void SetReferencedMaterialsWind(const WindSample& wind);
    /// Update global wind parameters.
    void UpdateGlobalWind();

protected:
    /// Wind pulse frequency is shared between wind zones.
    float pulseFrequency_ = 0.0f;

    /// Active directional wind zones.
    HashSet<WindZone*> directionalWindZones_;
    /// Active non-directional wind zones.
    HashSet<WindZone*> localWindZones_;
    /// Global wind.
    WindSample directionalWind_;

    /// Referenced materials set.
    using ReferencedMaterialSet = HashSet<WeakPtr<Material>>;
    /// Referenced materials.
    ReferencedMaterialSet referencedMaterials_;
};

/// Wind zone type.
enum class WindZoneType
{
    /// Directional wind zone.
    Directional,
    /// Spherical wind zone.
    Spherical
};

/// Wind zone.
class WindZone : public Component
{
    URHO3D_OBJECT(WindZone, Component);

public:
    /// Construct.
    WindZone(Context* context);
    /// Destruct.
    virtual ~WindZone();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;
    /// Visualize the component as debug geometry.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Set wind zone type.
    void SetWindZoneType(WindZoneType type);
    /// Return wind zone type.
    WindZoneType GetWindZoneType() const { return type_; }
    /// Set pulse frequency.
    void SetPulseFrequency(float pulseFrequency) { if (windSystem_) windSystem_->SetPulseFrequency(pulseFrequency); }
    /// Get pulse frequency.
    float GetPulseFreqency() const { return windSystem_ ? windSystem_->GetPulseFreqency() : 0.0f; }

    /// Check for no dirty and update wind system if needed.
    void CheckTransform();
    /// Return wind zone parameters at specified position. Flag shows whether the wind zone affects specified position.
    Pair<WindSample, bool> GetWindSample(const Vector3& position = Vector3::ZERO) const;

private:
    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene) override;
    /// Handle scene node enabled status changing.
    virtual void OnSetEnabled() override;

    /// Add wind zone to wind system.
    void AddZoneToWindSystem();
    /// Remove wind zone from wind system.
    void RemoveZoneFromWindSystem();

protected:
    /// Wind system.
    WeakPtr<WindSystem> windSystem_;

    /// Wind zone type.
    WindZoneType type_ = WindZoneType::Directional;
    /// Wind zone radius.
    float radius_ = 0.0f;
    /// Main wind intensity.
    float main_ = 0.0f;
    /// Wind turbulence intensity.
    float turbulence_ = 0.0f;
    /// Wind pulse magnitude.
    float pulseMagnitude_ = 0.0f;

    /// Cached wind zone direction.
    Vector3 cachedDirection_;

};

}

}
