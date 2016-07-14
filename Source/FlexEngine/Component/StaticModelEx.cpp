#include <FlexEngine/Component/StaticModelEx.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Model.h>

namespace FlexEngine
{

StaticModelEx::StaticModelEx(Context* context) :
    StaticModel(context),
    lodSwitchDuration_(0.0f),
    numSwitchAnimations_(0)
{

}

StaticModelEx::~StaticModelEx()
{

}

void StaticModelEx::RegisterObject(Context* context)
{
    context->RegisterFactory<StaticModelEx>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(StaticModel);
    URHO3D_ATTRIBUTE("Switch Duration", float, lodSwitchDuration_, false, AM_FILE);
}

void StaticModelEx::UpdateBatches(const FrameInfo& frame)
{
    /// @todo Add immediate switch if invisible

    // If disabled, use base implementation.
    if (!IsSmoothLodEnabled())
    {
        StaticModel::UpdateBatches(frame);
        return;
    }

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

    assert(numSwitchAnimations_ >= 0);
    if (newLodDistance != lodDistance_ || numSwitchAnimations_ > 0)
    {
        lodDistance_ = newLodDistance;
        CalculateLodLevels(frame.timeStep_);
    }
}

bool StaticModelEx::IsSmoothLodEnabled() const
{
    if (batches_.Size() != geometries_.Size())
    {
        assert(batches_.Size() == geometries_.Size() * 2);
        return true;
    }
    else
    {
        return false;
    }
}

void StaticModelEx::EnableSmoothLod(float switchOffsetFactor, float switchDuration)
{
    numSwitchAnimations_ = 0;
    lodSwitchDuration_ = switchDuration;

    const unsigned numGeometries = GetNumGeometries();

    // Remove extra batches added before
    batches_.Resize(numGeometries);

    // Add extra batches with empty geometry
    Vector<SourceBatch> emptyBatches = batches_;
    for (unsigned i = 0; i < emptyBatches.Size(); ++i)
    {
        emptyBatches[i].geometry_ = (Geometry*)0;
    }
    batches_ += emptyBatches;

    // Fill geometry data
    geometryDataEx_.Resize(numGeometries);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        Vector<SharedPtr<Geometry> >& batchGeometries = geometries_[i];
        StaticModelGeometryData& geometryData = geometryData_[i];
        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];

        geometryDataEx.center_ = geometryData.center_;
        for (unsigned j = 0; j < batchGeometries.Size(); ++j)
        {
            const float lodDistance = batchGeometries[j]->GetLodDistance();
            const float lodDistanceScaled = lodDistance * switchOffsetFactor;
            const float lodDistanceFadeIn = Min(lodDistance, lodDistanceScaled);
            const float lodDistanceFadeOut = Max(lodDistance, lodDistanceScaled);
            geometryDataEx.lodDistances_.Push(Vector2(lodDistanceFadeIn, lodDistanceFadeOut));
        }

        // Setup instancing data
        batches_[i].instancingData_ = &geometryDataEx.primaryInstanceData_;
        batches_[i + numGeometries].instancingData_ = &geometryDataEx.secondaryInstanceData_;
        geometryDataEx.primaryInstanceData_[0].x_ = 1.0;
        geometryDataEx.secondaryInstanceData_[0].x_ = 0.0;
    }

    // Reset LOD levels
    ResetLodLevels();
}

void StaticModelEx::ResetLodLevels()
{
    for (unsigned i = 0; i < geometryDataEx_.Size(); ++i)
    {
        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];
        geometryDataEx.primaryLodLevel_ = 0;
        geometryDataEx.secondaryLodLevel_ = 0;
        geometryDataEx.lodLevelMixFactor_ = 0.0f;
    }
}

void StaticModelEx::CalculateLodLevels(float timeStep)
{
    // If disabled, use base implementation.
    if (!IsSmoothLodEnabled())
    {
        StaticModel::CalculateLodLevels();
        return;
    }

    const unsigned numBatches = batches_.Size() / 2;
    for (unsigned i = 0; i < numBatches; ++i)
    {
        const Vector<SharedPtr<Geometry> >& batchGeometries = geometries_[i];
        // If only one LOD geometry, no reason to go through the LOD calculation
        if (batchGeometries.Size() <= 1)
            continue;

        StaticModelGeometryDataEx& geometryDataEx = geometryDataEx_[i];

        /// @todo Don't animate switch if first time or camera warp
        if (geometryDataEx.lodLevelMixFactor_ > 0.0f)
        {
            // Update animation
            geometryDataEx.lodLevelMixFactor_ -= timeStep / lodSwitchDuration_;

            if (geometryDataEx.lodLevelMixFactor_ <= 0.0)
            {
                // Hide second instance
                --numSwitchAnimations_;
                geometryDataEx.lodLevelMixFactor_ = 0.0f;
                batches_[i + numBatches].geometry_ = (Geometry*)0;
            }
        }
        else
        {
            // Re-compute LOD
            const unsigned newLod = ComputeBestLod(lodDistance_, geometryDataEx.primaryLodLevel_, geometryDataEx.lodDistances_);
            if (newLod != geometryDataEx.primaryLodLevel_)
            {
                // Start switch
                ++numSwitchAnimations_;
                geometryDataEx.secondaryLodLevel_ = geometryDataEx.primaryLodLevel_;
                geometryDataEx.primaryLodLevel_ = newLod;
                geometryDataEx.lodLevelMixFactor_ = 1.0;

                batches_[i].geometry_ = batchGeometries[geometryDataEx.primaryLodLevel_];
                batches_[i + numBatches].geometry_ = batchGeometries[geometryDataEx.secondaryLodLevel_];
            }
        }

        // Update factor
        geometryDataEx.primaryInstanceData_[0].x_ = 1.0f - geometryDataEx.lodLevelMixFactor_;
        geometryDataEx.secondaryInstanceData_[0].x_ = 2.0f - geometryDataEx.lodLevelMixFactor_;
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

}
