#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{

class Material;

}

namespace FlexEngine
{

class WindObject;
class WindZone;

/// Wind direction uniform. Name is 'WindDirection'. XYZ is wind direction. W is foliage oscillation magnitude.
extern const String VSP_WINDDIRECTION;
/// Wind parameters uniform. Name is 'WindParam'. X is main wind. Y is turbulence wind. Z is pulse magnitude. W is pulse frequency.
extern const String VSP_WINDPARAM;

/// Wind sample data.
struct WindSample
{
    /// Direction.
    Vector3 direction_;
    /// Main wind.
    float main_ = 0.0f;
    /// Turbulence wind.
    float turbulence_ = 0.0f;
    /// Pulse magnitude.
    float pulseMagnitude_ = 0.0f;
    /// Pulse frequency.
    float pulseFrequency_ = 0.0f;
};

/// Wind system.
class WindSystem : public Component
{
    URHO3D_OBJECT(WindSystem, Component);

public:
    /// Construct.
    WindSystem(Context* context);
    /// Destruct.
    virtual ~WindSystem();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Add wind zone.
    void AddWindZone(WindZone* windZone);
    /// Remove wind zone.
    void RemoveWindZone(WindZone* windZone);
    /// Mark wind zone dirty.
    void MarkWindZoneDirty(WindZone* windZone);

    /// Reference material.
    void ReferenceMaterial(Material* material);
    /// Dereference material.
    void DereferenceMaterial(Material* material);

private:
    /// Set wind parameters to material.
    static void SetMaterialWind(Material& material, const WindSample& wind);
    /// Set wind parameters to all referenced materials.
    void SetReferencedMaterialsWind(const WindSample& wind);

protected:
    /// Active directional wind zones.
    HashSet<WindZone*> directionalWindZones_;
    /// Global wind.
    WindSample directionalWind_;

    /// Referenced materials.
    HashMap<Material*, unsigned> referencedMaterials_;
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

    /// Add wind zone to wind system.
    void AddZoneToWindSystem();
    /// Remove wind zone to wind system.
    void RemoveZoneFromWindSystem();

    /// Sample wind zone at specified position.
    WindSample Sample(const Vector3& position = Vector3::ZERO) const;

private:
    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene) override;
    /// Handle scene node enabled status changing.
    virtual void OnSetEnabled() override;
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;
    /// Visualize the component as debug geometry.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

protected:
    /// Wind system.
    WeakPtr<WindSystem> windSystem_;

    /// Main wind intensity.
    float main_ = 0.0f;
    /// Wind turbulence intensity.
    float turbulence_ = 0.0f;
    /// Wind pulse magnitude.
    float pulseMagnitude_ = 0.0f;
    /// Wind pulse frequency.
    float pulseFrequency_ = 0.0f;


};

}
