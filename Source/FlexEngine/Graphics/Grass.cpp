#include <FlexEngine/Graphics/Grass.h>

#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/PoissonRandom.h>
#include <FlexEngine/Math/StandardRandom.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/TerrainPatch.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{

namespace
{

/// Points density in sample.
static const float samplePointsDensity = 20.0f;
/// Number of points in sample (upper bound).
static const unsigned samplePointsLimit = 10000;
/// Max number of iterations for sample generation.
static const unsigned samplePointsMaxIterations = 30;
/// Factor of extra buffer allocation.
static const float allocationFactor = 1.1f;

}

Grass::Grass(Context* context)
    : Drawable(context, DRAWABLE_GEOMETRY)
    , workQueue_(context->GetSubsystem<WorkQueue>())
    , instanceData_(1, 1, 0, 0)
    // #TODO Unhardcode
    , denisty_(1.5f)
    , drawDistance_(100.0f)
    , updateThreshold_(20.0f)
{
    SubscribeToEvent(E_WORKITEMCOMPLETED, URHO3D_HANDLER(Grass, HandleUpdatePatchFinished));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Grass, HandleUpdate));
}

Grass::~Grass()
{
}

void Grass::RegisterObject(Context* context)
{
    context->RegisterFactory<Grass>(FLEXENGINE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
//     URHO3D_ATTRIBUTE("Is Occluder", bool, occluder_, false, AM_DEFAULT);
//     URHO3D_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, bool, true, AM_DEFAULT);
//     URHO3D_ATTRIBUTE("Cast Shadows", bool, castShadows_, false, AM_DEFAULT);
//     URHO3D_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, float, 0.0f, AM_DEFAULT);
//     URHO3D_ACCESSOR_ATTRIBUTE("Shadow Distance", GetShadowDistance, SetShadowDistance, float, 0.0f, AM_DEFAULT);
//     URHO3D_ACCESSOR_ATTRIBUTE("LOD Bias", GetLodBias, SetLodBias, float, 1.0f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable);

}

void Grass::ApplyAttributes()
{
    UpdateBuffersData();
}

void Grass::UpdateBatches(const FrameInfo& frame)
{
    UpdatePatchesThreshold(frame.camera_->GetNode()->GetPosition());
}

void Grass::UpdateGeometry(const FrameInfo& frame)
{

}

void Grass::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef Grass::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void Grass::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void Grass::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (patchesDirty_)
    {
        UpdatePatches(origin_);
    }
}

String Grass::MakeChildName(const IntVector2& index)
{
    return "Grass_" + String(index.x_) + "_" + String(index.y_);
}

unsigned Grass::GetNumBillboardVertices() const
{
    return 4;
}

int Grass::ComputeNumPatches(float terrainSize, float distance, unsigned numVertices)
{
    static const unsigned maxNumVertices = std::numeric_limits<unsigned short>::max() / 4;
    const unsigned maxNumBillboards = maxNumVertices / numVertices;
    const float maxPatchSize = Sqrt(static_cast<float>(maxNumBillboards)) * distance;
    return CeilToInt(terrainSize / maxPatchSize);
}

Vector2 Grass::ComputeLocalPosition(const BoundingBox& worldBoundingBox, const Vector3& position, int numPatches)
{
    Vector2 localPosition(position.x_ - worldBoundingBox.min_.x_, position.z_ - worldBoundingBox.min_.z_);
    localPosition /= Min(worldBoundingBox.Size().x_, worldBoundingBox.Size().z_);
    localPosition *= static_cast<float>(numPatches);
    return localPosition;
}

IntRect Grass::ComputePatchRegion(const BoundingBox& worldBoundingBox, const Vector3& position, float distance, int numPatches)
{
    const Vector3 offset = Vector3(distance, 0.0f, distance);
    const Vector2 begin = ComputeLocalPosition(worldBoundingBox, position - offset, numPatches);
    const Vector2 end = ComputeLocalPosition(worldBoundingBox, position + offset, numPatches);

    IntRect rect;
    rect.left_  = FloorToInt(begin.x_);
    rect.top_   = FloorToInt(begin.y_);
    rect.right_  = CeilToInt(end.x_);
    rect.bottom_ = CeilToInt(end.y_);
    return rect;
}

void Grass::UpdateBoundingBox()
{
    boundingBox_.Clear();
    if (terrain_)
    {
        const IntVector2 numPatches = terrain_->GetNumPatches();
        for (unsigned i = 0; i < static_cast<unsigned>(numPatches.x_ * numPatches.y_); ++i)
        {
            TerrainPatch* patch = terrain_->GetPatch(i);
            boundingBox_.Merge(patch->GetBoundingBox().Transformed(patch->GetNode()->GetTransform()));
        }
    }
    OnMarkedDirty(node_);
}

void Grass::UpdatePattern()
{
    const Vector3 boundingBoxSize = boundingBox_.Size();
    const float terrainSize = Min(boundingBoxSize.x_, boundingBoxSize.z_);
    const int downscalePattern = CeilToInt(terrainSize * denisty_ / samplePointsDensity);
    const float patternStep = downscalePattern / (terrainSize * denisty_);
    PoissonRandom poisson(0);
    pattern_ = poisson.generate(patternStep, samplePointsMaxIterations, samplePointsLimit);
    patternScale_ = 1 / (denisty_ * patternStep);
}

void Grass::UpdatePatchAsync(const WorkItem* workItem, unsigned threadIndex)
{
    Grass& self = *reinterpret_cast<Grass*>(workItem->aux_);
    GrassPatch& patch = *reinterpret_cast<GrassPatch*>(workItem->start_);
    patch.UpdatePatch(*self.terrain_);
}

void Grass::HandleUpdatePatchFinished(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_WORKITEMCOMPLETED)
    {
        if (WorkItem* workItem = dynamic_cast<WorkItem*>(eventData[WorkItemCompleted::P_ITEM].GetPtr()))
        {
            GrassPatch& patch = *reinterpret_cast<GrassPatch*>(workItem->start_);
            patch.SetWorkItem(nullptr);
            patch.FinishUpdatePatch();
        }
    }
}

void Grass::SchedulePatchUpdate(GrassPatch& patch)
{
    if (patch.GetWorkItem())
        return;

    SharedPtr<WorkItem> item = workQueue_->GetFreeItem();
    item->sendEvent_ = true;
    item->start_ = &patch;
    item->aux_ = this;
    item->workFunction_ = &UpdatePatchAsync;
    workQueue_->AddWorkItem(item);
    patch.SetWorkItem(item);
}

void Grass::CancelPatchUpdate(GrassPatch& patch)
{
    if (!patch.GetWorkItem())
        return;

//     workQueue_->DiscardWorkItem(patch.GetWorkItem());
    patch.SetWorkItem(nullptr);
}

GrassPatch* Grass::AddPatch(const IntVector2& index)
{
    PatchMap::Iterator patchIter = patches_.Find(index);
    if (patchIter != patches_.End())
        return patchIter->second_;

    Node* patchNode = node_->CreateTemporaryChild(MakeChildName(index), LOCAL);
    GrassPatch* patch = patchNode->CreateComponent<GrassPatch>();
//     patch->SetPattern(patternScale_, pattern_);
//     SchedulePatchUpdate(*patch);
//     patch->UpdatePatch(*terrain_);
//     patch->FinishUpdatePatch();
//     SharedPtr<GrassPatch> patch;
//     if (patchesPool_.Empty())
//         patch = MakeShared<GrassPatch>(context_);
//     else
//     {
//         patch = patchesPool_.Back();
//         patchesPool_.Pop();
//     }

    patches_[index] = patch;
    return patch;
}

Grass::PatchMap::Iterator Grass::RemovePatch(const IntVector2& index)
{
    PatchMap::Iterator patchIter = patches_.Find(index);
    if (patchIter != patches_.End())
    {
        Node* patchNode = node_->GetChild(MakeChildName(index));
        GrassPatch* patch = patchNode->GetComponent<GrassPatch>();
        CancelPatchUpdate(*patch);
        node_->RemoveChild(patchNode);
//         patchesPool_.Push(patchIter->second_);
        patchIter = patches_.Erase(patchIter);
    }
    return patchIter;
}

void Grass::UpdatePatches(const Vector3& origin)
{
    patchesDirty_ = false;
    const BoundingBox worldBoundingBox = boundingBox_.Transformed(node_->GetWorldTransform());
    const Vector3 worldBoundingBoxSize = worldBoundingBox.Size();

    const float terrainSize = Min(worldBoundingBoxSize.x_, worldBoundingBoxSize.z_);
    const int numPatches = ComputeNumPatches(terrainSize, 1 / denisty_, GetNumBillboardVertices());
    const float maxDistance = drawDistance_ + updateThreshold_ * 2;
    const IntRect region = ComputePatchRegion(worldBoundingBox, origin, maxDistance, numPatches);
    const float patchSize = terrainSize / numPatches;

    // Add new patches
    IntVector2 index;
    for (index.x_ = Max(0, region.left_); index.x_ < Min(numPatches, region.right_); ++index.x_)
        for (index.y_ = Max(0, region.top_); index.y_ < Min(numPatches, region.bottom_); ++index.y_)
        {
            if (patches_.Find(index) == patches_.End())
            {
                GrassPatch* patch = AddPatch(index);
                const Vector3 index3 = Vector3(static_cast<float>(index.x_), 0.0f, static_cast<float>(index.y_));
                patch->GetNode()->SetPosition(index3 * patchSize + (worldBoundingBox.min_ - node_->GetWorldPosition()) * Vector3(1, 0, 1));
                patch->GetNode()->MarkDirty();
                patch->SetPattern(patternScale_, pattern_);
                patch->SetMaterial(material_);
                patch->SetRange(worldBoundingBox.min_,
                    Rect(index.x_ * patchSize, index.y_ * patchSize, (index.x_ + 1) * patchSize, (index.y_ + 1) * patchSize));
                patch->UpdatePatch(*terrain_);
                patch->FinishUpdatePatch();
                patches_[index] = patch;
            }
        }

    // Remove old patches
    for (auto i = patches_.Begin(); i != patches_.End();)
    {
        if (region.IsInside(i->first_) == OUTSIDE)
            i = RemovePatch(i->first_);
        else
            ++i;
    }
}

void Grass::UpdatePatchesThreshold(const Vector3& origin)
{
    if ((origin - origin_).Length() > updateThreshold_)
    {
        origin_ = origin;
        patchesDirty_ = true;
    }
}

bool Grass::SetupSource()
{
    if (node_)
    {
        // Load terrain
        if (!terrain_)
        {
            terrain_ = node_->GetComponent<Terrain>();
            UpdateBoundingBox();
            UpdatePattern();
        }
    }
    return !!terrain_;
}

void Grass::UpdateBuffersData()
{
    if (!SetupSource())
        return;

    UpdatePatches(Vector3::ZERO);
}

}
