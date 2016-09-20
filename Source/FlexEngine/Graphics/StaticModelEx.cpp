#include <FlexEngine/Graphics/StaticModelEx.h>

#include <FlexEngine/Core/Attribute.h>

#include <Urho3D/Core/Context.h>
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
    // Dereference materials
    if (applyWind_)
        SetApplyWind(false);
}

void StaticModelEx::RegisterObject(Context* context)
{
    context->RegisterFactory<StaticModelEx>(FLEXENGINE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Apply Wind", ShouldApplyWind, SetApplyWind, bool, false, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(StaticModel);
}

void StaticModelEx::ApplyAttributes()
{
}

void StaticModelEx::SetModel(Model* model)
{
    StaticModel::SetModel(model);
}

void StaticModelEx::SetMaterial(Material* material)
{
    StaticModel::SetMaterial(material);
    UpdateReferencedMaterials();
}

bool StaticModelEx::SetMaterial(unsigned index, Material* material)
{
    if (StaticModel::SetMaterial(index, material))
    {
        UpdateReferencedMaterial(index);
        return true;
    }
    return false;
}

void StaticModelEx::SetApplyWind(bool applyWind)
{
    applyWind_ = applyWind;
    UpdateReferencedMaterials();
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
    if (!windSystem_)
        return;

    const unsigned oldNumMaterials = referencedMaterials_.Size();
    const unsigned newNumMaterials = applyWind_ ? geometries_.Size() : 0;

    // Dereference removed materials
    for (unsigned i = newNumMaterials; i < oldNumMaterials; ++i)
        windSystem_->DereferenceMaterial(referencedMaterials_[i]);

    // Update referenced materials
    referencedMaterials_.Resize(newNumMaterials);
    for (unsigned i = oldNumMaterials; i < newNumMaterials; ++i)
        referencedMaterials_[i] = nullptr;

    for (unsigned i = 0; i < newNumMaterials; ++i)
    {
        if (referencedMaterials_[i] != batches_[i].material_)
        {
            windSystem_->DereferenceMaterial(referencedMaterials_[i]);
            windSystem_->ReferenceMaterial(batches_[i].material_);
            referencedMaterials_[i] = batches_[i].material_;
        }
    }
}

void StaticModelEx::UpdateReferencedMaterial(unsigned i)
{
    if (!windSystem_ || !applyWind_)
        return;

    if (i >= referencedMaterials_.Size())
        referencedMaterials_.Resize(i + 1);

    if (referencedMaterials_[i] != batches_[i].material_)
    {
        windSystem_->DereferenceMaterial(referencedMaterials_[i]);
        windSystem_->ReferenceMaterial(batches_[i].material_);
        referencedMaterials_[i] = batches_[i].material_;
    }
}

}
