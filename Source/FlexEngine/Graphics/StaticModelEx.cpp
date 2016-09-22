#include <FlexEngine/Graphics/StaticModelEx.h>

#include <FlexEngine/Core/Attribute.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
// #include <Urho3D/Graphics/DebugRenderer.h>
// #include <Urho3D/IO/Log.h>
// #include <Urho3D/Scene/Node.h>
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

    URHO3D_ACCESSOR_ATTRIBUTE("Apply Wind", ShouldApplyWind, SetApplyWind, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Clone Materials", AreMaterialsCloned, SetCloneMaterials, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Unique Materials", AreMaterialsUnique, SetUniqueMaterials, bool, false, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(StaticModel);
}

void StaticModelEx::ApplyAttributes()
{
}

void StaticModelEx::UpdateBatches(const FrameInfo& frame)
{
    StaticModel::UpdateBatches(frame);
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

void StaticModelEx::SetModel(Model* model)
{
    StaticModel::SetModel(model);
    geometryDataEx_.Resize(GetNumGeometries());
}

void StaticModelEx::SetMaterial(Material* material)
{
    StaticModel::SetMaterial(material);
    UpdateReferencedMaterial(material);
    for (unsigned i = 0; i < GetNumGeometries(); ++i)
        SetMaterialImpl(i, material);
}

bool StaticModelEx::SetMaterial(unsigned index, Material* material)
{
    if (StaticModel::SetMaterial(index, material))
    {
        UpdateReferencedMaterial(material);
        SetMaterialImpl(index, material);
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
        for (unsigned i = 0; i < batches_.Size(); ++i)
        {
            windSystem_->ReferenceMaterial(batches_[i].material_);
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
        geometryDataEx_[index].clonedMaterial_ = material->Clone();
}

void StaticModelEx::SetBatchMaterial(unsigned index)
{
    assert(index < batches_.Size());
    batches_[index].material_ = cloneRequests_ ? geometryDataEx_[index].clonedMaterial_ : geometryDataEx_[index].originalMaterial_;
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

}
