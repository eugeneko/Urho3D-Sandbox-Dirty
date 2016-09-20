#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Graphics/Wind.h>

#include <Urho3D/Graphics/StaticModel.h>

namespace FlexEngine
{

/// Wind zone.
class StaticModelEx : public StaticModel
{
    URHO3D_OBJECT(StaticModelEx, StaticModel);

public:
    /// Construct.
    StaticModelEx(Context* context);
    /// Destruct.
    virtual ~StaticModelEx();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Set model.
    virtual void SetModel(Model* model) override;
    /// Set material on all geometries.
    virtual void SetMaterial(Material* material) override;
    /// Set material on one geometry. Return true if successful.
    virtual bool SetMaterial(unsigned index, Material* material) override;

    /// Set apply wind.
    void SetApplyWind(bool applyWind);
    /// Get apply wind.
    bool ShouldApplyWind() const { return applyWind_; }

private:
    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene) override;

    /// Update referenced materials.
    void UpdateReferencedMaterials();
    /// Update referenced material.
    void UpdateReferencedMaterial(unsigned i);

private:
    /// Whether to receive wind updates.
    bool applyWind_ = false;
    /// Wind system.
    WeakPtr<WindSystem> windSystem_;
    /// Materials referenced by wind system.
    PODVector<Material*> referencedMaterials_;
};

}
