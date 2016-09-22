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
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame) override;

    /// Set model.
    virtual void SetModel(Model* model) override;
    /// Set material on all geometries.
    virtual void SetMaterial(Material* material) override;
    /// Set material on one geometry. Return true if successful.
    virtual bool SetMaterial(unsigned index, Material* material) override;

    /// Return material by geometry index.
    virtual Material* GetMaterial(unsigned index = 0) const override;

    /// Set whether to apply wind.
    void SetApplyWind(bool applyWind);
    /// Return whether to apply wind.
    bool ShouldApplyWind() const { return applyWind_; }
    /// Set whether to clone materials.
    void SetCloneMaterials(bool cloneMaterials);
    /// Return whether to clone materials.
    bool AreMaterialsCloned() const { return cloneMaterials_; }
    /// Set whether to use unique materials. This call is ignored if materials are not cloned.
    void SetUniqueMaterials(bool uniqueMaterials) { SetCloneRequest(CR_FORCE_UNIQUE, uniqueMaterials); }
    /// Return whether to use unique materials.
    bool AreMaterialsUnique() const { return cloneRequests_ & CR_FORCE_UNIQUE; }

private:
    /// Extended geometry data.
    struct GeometryDataEx
    {
        SharedPtr<Material> originalMaterial_;
        SharedPtr<Material> clonedMaterial_;
    };

    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene) override;

    /// Update referenced materials.
    void UpdateReferencedMaterials();
    /// Update referenced material.
    void UpdateReferencedMaterial(Material* material);
    /// Set original material and clone if needed.
    void SetMaterialImpl(unsigned index, Material* material);
    /// Set original or cloned batch material.
    void SetBatchMaterial(unsigned index);
    /// Set clone request flag set. This call is ignored if materials are not cloned.
    void SetCloneRequestSet(unsigned flagSet);
    /// Set clone request flag. This call is ignored if materials are not cloned.
    void SetCloneRequest(unsigned flag, bool enable);

private:
    /// Clone request from user.
    static const unsigned CR_FORCE_UNIQUE = 1 << 0;
    /// Clone request from wind system.
    static const unsigned CR_WIND = 1 << 1;

    /// Whether to receive wind updates.
    bool applyWind_ = false;
    /// Wind system.
    WeakPtr<WindSystem> windSystem_;
    /// Whether to clone materials.
    bool cloneMaterials_ = false;

    /// Flag set of clone material requests.
    unsigned cloneRequests_ = 0;
    /// Extended per-geometry data.
    Vector<GeometryDataEx> geometryDataEx_;
};

}
