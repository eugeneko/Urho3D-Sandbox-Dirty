#include <FlexEngine/Graphics/StaticModelEx.h>

#include <FlexEngine/Core/Attribute.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

StaticModelEx::StaticModelEx(Context* context)
    : StaticModel(context)
{

}

StaticModelEx::~StaticModelEx()
{
}

void StaticModelEx::RegisterObject(Context* context)
{
    context->RegisterFactory<StaticModelEx>(FLEXENGINE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()),
        AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Apply Wind", ShouldApplyWind, SetApplyWind, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Clone Materials", AreMaterialsCloned, SetCloneMaterials, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Unique Materials", AreMaterialsUnique, SetUniqueMaterials, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("LOD Switch Bias", GetLodSwitchBias, SetLodSwitchBias, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("LOD Switch Duration", GetLodSwitchDuration, SetLodSwitchDuration, float, 1.0f, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Is Occluder", bool, occluder_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Cast Shadows", bool, castShadows_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Shadow Distance", GetShadowDistance, SetShadowDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("LOD Bias", GetLodBias, SetLodBias, float, 1.0f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable);
    URHO3D_ATTRIBUTE("Occlusion LOD Level", int, occlusionLodLevel_, M_MAX_UNSIGNED, AM_DEFAULT);

}

void StaticModelEx::ApplyAttributes()
{
}

void StaticModelEx::UpdateBatches(const FrameInfo& frame)
{
    UpdateLodLevels(frame);
    UpdateWind();
}

void StaticModelEx::SetModel(Model* model)
{
    StaticModel::SetModel(model);

    // Setup extra batches
    const unsigned numGeometries = GetNumGeometries();
    geometryDataEx_.Resize(numGeometries);
    batches_.Resize(numGeometries * 2);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        batches_[i + numGeometries] = batches_[i];

        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];
        batches_[i].instancingData_ = &geometryDataEx.primaryInstanceData_;
        batches_[i + numGeometries].instancingData_ = &geometryDataEx.secondaryInstanceData_;
        geometryDataEx.primaryInstanceData_.x_ = 1.0;
        geometryDataEx.secondaryInstanceData_.x_ = 0.0;
    }

    SetupLodDistances();
    ResetLodLevels();
}

void StaticModelEx::SetMaterial(Material* material)
{
    StaticModel::SetMaterial(material);
    UpdateReferencedMaterial(material);
    for (unsigned i = 0; i < GetNumGeometries(); ++i)
    {
        SetMaterialImpl(i, material);
        SetBatchMaterial(i);
    }
}

bool StaticModelEx::SetMaterial(unsigned index, Material* material)
{
    if (index < GetNumGeometries())
    {
        StaticModel::SetMaterial(index, material);
        SetMaterialImpl(index, material);
        UpdateReferencedMaterial(material);
        SetBatchMaterial(index);
        return true;
    }
    return false;
}

Material* StaticModelEx::GetMaterial(unsigned index /*= 0*/) const
{
    return index < geometryDataEx_.Size() ? geometryDataEx_[index].originalMaterial_ : nullptr;
}

void StaticModelEx::SetApplyWind(bool applyWind)
{
    applyWind_ = applyWind;
    UpdateReferencedMaterials();
}

void StaticModelEx::SetCloneMaterials(bool cloneMaterials)
{
    cloneMaterials_ = cloneMaterials;
    if (cloneMaterials_)
    {
        for (unsigned i = 0; i < GetNumGeometries(); ++i)
            SetMaterialImpl(i, geometryDataEx_[i].originalMaterial_);
    }
    else
    {
        cloneRequests_ = 0;
    }
}

const ResourceRefList& StaticModelEx::GetMaterialsAttr() const
{
    materialsAttr_.names_.Resize(geometryDataEx_.Size());
    for (unsigned i = 0; i < geometryDataEx_.Size(); ++i)
        materialsAttr_.names_[i] = GetResourceName(geometryDataEx_[i].originalMaterial_);

    return materialsAttr_;
}

void StaticModelEx::OnSceneSet(Scene* scene)
{
    StaticModel::OnSceneSet(scene);
    if (scene)
    {
        windSystem_ = scene->GetOrCreateComponent<WindSystem>();
        UpdateReferencedMaterials();
    }
}

void StaticModelEx::UpdateReferencedMaterials()
{
    if (windSystem_ && applyWind_)
    {
        for (unsigned i = 0; i < geometryDataEx_.Size(); ++i)
        {
            windSystem_->ReferenceMaterial(geometryDataEx_[i].originalMaterial_);
        }
    }
}

void StaticModelEx::UpdateReferencedMaterial(Material* material)
{
    if (windSystem_ && applyWind_)
        windSystem_->ReferenceMaterial(material);
}

void StaticModelEx::SetMaterialImpl(unsigned index, Material* material)
{
    assert(index < GetNumGeometries());
    geometryDataEx_[index].originalMaterial_ = material;
    geometryDataEx_[index].clonedMaterial_.Reset();
    if (cloneMaterials_)
        geometryDataEx_[index].clonedMaterial_ = material ? material->Clone() : nullptr;
}

void StaticModelEx::SetBatchMaterial(unsigned index)
{
    assert(index < geometryDataEx_.Size());
    batches_[index].material_ = cloneRequests_ ? geometryDataEx_[index].clonedMaterial_ : geometryDataEx_[index].originalMaterial_;
    batches_[index + geometryDataEx_.Size()].material_ = batches_[index].material_;
}

void StaticModelEx::SetCloneRequestSet(unsigned flagSet)
{
    if (cloneMaterials_)
    {
        if (!!cloneRequests_ != !!flagSet)
        {
            cloneRequests_ = flagSet;
            for (unsigned i = 0; i < GetNumGeometries(); ++i)
                SetBatchMaterial(i);
        }
    }
}

void StaticModelEx::SetCloneRequest(unsigned flag, bool enable)
{
    if (enable)
        SetCloneRequestSet(cloneRequests_ | flag);
    else
        SetCloneRequestSet(cloneRequests_ & ~flag);
}

//////////////////////////////////////////////////////////////////////////
void StaticModelEx::SetupLodDistances()
{
    const unsigned numGeometries = GetNumGeometries();
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        const Vector<SharedPtr<Geometry> >& batchGeometries = geometries_[i];
        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];

        geometryDataEx.lodDistances_.Resize(batchGeometries.Size());
        for (unsigned j = 0; j < batchGeometries.Size(); ++j)
        {
            const float distance = batchGeometries[j]->GetLodDistance();
            const float fadeIn = Min(distance, distance * lodSwitchBias_);
            const float fadeOut = Max(distance, distance * lodSwitchBias_);
            geometryDataEx.lodDistances_[j] = Vector2(fadeIn, fadeOut);
        }
    }
}

void StaticModelEx::ResetLodLevels()
{
    numLodSwitchAnimations_ = 0;
    for (unsigned i = 0; i < geometryDataEx_.Size(); ++i)
    {
        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];
        geometryDataEx.primaryLodLevel_ = 0;
        geometryDataEx.secondaryLodLevel_ = 0;
        geometryDataEx.lodLevelMix_ = 0.0f;
    }
}

void StaticModelEx::CalculateLodLevels(float timeStep)
{
    const unsigned numBatches = batches_.Size() / 2;
    for (unsigned i = 0; i < numBatches; ++i)
    {
        const Vector<SharedPtr<Geometry> >& batchGeometries = geometries_[i];
        // If only one LOD geometry, no reason to go through the LOD calculation
        if (batchGeometries.Size() <= 1)
            continue;

        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];

        // #TODO Don't animate switch if first time or camera warp
        if (geometryDataEx.lodLevelMix_ > 0.0f)
        {
            // Update animation
            geometryDataEx.lodLevelMix_ -= timeStep / lodSwitchDuration_;

            if (geometryDataEx.lodLevelMix_ <= 0.0)
            {
                // Hide second instance
                --numLodSwitchAnimations_;
                geometryDataEx.lodLevelMix_ = 0.0f;
                batches_[i + numBatches].geometry_ = nullptr;
            }
        }
        else
        {
            // Re-compute LOD
            const unsigned newLod = ComputeBestLod(lodDistance_, geometryDataEx.primaryLodLevel_, geometryDataEx.lodDistances_);
            if (newLod != geometryDataEx.primaryLodLevel_)
            {
                // Start switch
                ++numLodSwitchAnimations_;
                geometryDataEx.secondaryLodLevel_ = geometryDataEx.primaryLodLevel_;
                geometryDataEx.primaryLodLevel_ = newLod;
                geometryDataEx.lodLevelMix_ = 1.0;

                batches_[i].geometry_ = batchGeometries[geometryDataEx.primaryLodLevel_];
                batches_[i + numBatches].geometry_ = batchGeometries[geometryDataEx.secondaryLodLevel_];
            }
        }

        // Update factor
        geometryDataEx.primaryInstanceData_.x_ = 1.0f - geometryDataEx.lodLevelMix_;
        geometryDataEx.secondaryInstanceData_.x_ = 2.0f - geometryDataEx.lodLevelMix_;
    }
}

unsigned StaticModelEx::ComputeBestLod(float distance, unsigned currentLod, const PODVector<Vector2>& distances)
{
    const unsigned numLods = distances.Size();

    // Compute best LOD
    unsigned bestLod = 0xffffffff;
    for (unsigned i = 0; i < numLods - 1; ++i)
    {
        // Inner and outer distances for i-th LOD
        const float innerDistance = distances[i + 1].x_;
        const float outerDistance = distances[i + 1].y_;

        if (distance < innerDistance)
        {
            // Nearer than inner distance of i-th LOD, so use it
            bestLod = i;
            break;
        }
        else if (distance < outerDistance)
        {
            // Nearer that outer distance of i-th LOD, so it must be i-th or at least i+1-th level
            bestLod = Clamp(currentLod, i, i + 1);
            break;
        }
    }

    return Min(bestLod, distances.Size() - 1);
}

void StaticModelEx::UpdateLodLevels(const FrameInfo& frame)
{
    /// #TODO Add immediate switch if invisible
    // Update distances
    const BoundingBox& worldBoundingBox = GetWorldBoundingBox();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());

    const unsigned numBatches = batches_.Size() / 2;
    if (numBatches == 1)
    {
        batches_[0].distance_ = distance_;
    }
    else
    {
        const Matrix3x4& worldTransform = node_->GetWorldTransform();
        for (unsigned i = 0; i < numBatches; ++i)
        {
            batches_[i].distance_ = frame.camera_->GetDistance(worldTransform * geometryData_[i].center_);
            batches_[i + numBatches].distance_ = batches_[i].distance_;
        }
    }

    // Update LODs
    float scale = worldBoundingBox.Size().DotProduct(DOT_SCALE);
    float newLodDistance = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

    assert(numLodSwitchAnimations_ >= 0);
    if (newLodDistance != lodDistance_ || numLodSwitchAnimations_ > 0)
    {
        lodDistance_ = newLodDistance;
        CalculateLodLevels(frame.timeStep_);
    }
}

void StaticModelEx::UpdateWind()
{
    if (windSystem_ && applyWind_)
    {
        if (cloneMaterials_ && windSystem_->HasLocalWindZones())
        {
            const Pair<WindSample, bool> sample = windSystem_->GetWindSample(node_->GetWorldPosition());
            if (sample.second_)
            {
                SetCloneRequest(CR_WIND, true);
                for (unsigned i = 0; i < batches_.Size(); ++i)
                {
                    if (batches_[i].material_)
                        WindSystem::SetMaterialWind(*batches_[i].material_, sample.first_);
                }
            }
            else
            {
                SetCloneRequest(CR_WIND, false);
            }
        }
        else
        {
            SetCloneRequest(CR_WIND, false);
        }
    }
}

}
