#include <FlexEngine/Physics/BulletSystem.h>

#include <FlexEngine/Graphics/LineRenderer.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

BulletSystem::BulletSystem(Context* context)
    : LogicComponent(context)
{
}

BulletSystem::~BulletSystem()
{
}

void BulletSystem::RegisterObject(Context* context)
{
    context->RegisterFactory<BulletSystem>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(LogicComponent);
}

void BulletSystem::ApplyAttributes()
{
    if (GetScene())
    {
        if (!lineRenderer_)
            lineRenderer_ = GetScene()->GetComponent<LineRenderer>();
        if (!physics_)
            physics_ = GetScene()->GetComponent<PhysicsWorld>();
    }
}

void BulletSystem::Update(float timeStep)
{
    if (!lineRenderer_ || !physics_)
        return;

    bool wasInactive_ = true;
    unsigned numInactive_ = 0;
    for (BulletData& bullet : bullets_)
    {
        if (bullet.active_)
        {
            wasInactive_ = false;
            if (UpdateBullet(bullet, timeStep))
            {
                const Vector3 direction = (bullet.secondPosition_ - bullet.firstPosition_).Normalized();
                const Vector3 lineStart = bullet.position_ - direction * Min(bullet.desc_.traceLength_ / 2, bullet.distance_);
                const Vector3 lineEnd = bullet.position_ + direction * Min(bullet.desc_.traceLength_ / 2, bullet.desc_.maxDistance_ - bullet.distance_);
                const Color color = Lerp(bullet.desc_.initialColor_, bullet.desc_.finalColor_, bullet.distance_ / bullet.desc_.maxDistance_);
                lineRenderer_->AddLine(lineStart, lineEnd, color, bullet.desc_.traceThickness_);
            }
        }
        else if (wasInactive_)
            ++numInactive_;
    }

    if (numInactive_ > bullets_.Size() / 2)
    {
        bullets_.Erase(0, numInactive_);
    }
}

void BulletSystem::FireBullet(const BulletDesc& bullet)
{
    if (bullet.velocity_.LengthSquared() <= M_LARGE_EPSILON * M_LARGE_EPSILON)
        return;

    BulletData data;
    data.desc_ = bullet;
    data.active_ = true;
    data.time_ = 0.0f;
    data.distance_ = 0.0f;

    data.position_ = bullet.position_;
    data.velocity_ = bullet.velocity_;

    data.firstTime_ = 0.0f;
    data.secondTime_ = 0.0f;
    data.firstDistance_ = 0.0f;
    data.secondDistance_ = 0.0f;
    data.firstPosition_ = bullet.position_;
    data.secondPosition_ = bullet.position_;

    bullets_.Push(data);
}

bool BulletSystem::UpdateBullet(BulletData& bullet, float timeStep)
{
    bullet.time_ += timeStep;
    if (bullet.secondTime_ < bullet.time_)
    {
        bullet.firstTime_ = bullet.secondTime_;
        bullet.firstDistance_ = bullet.secondDistance_;
        bullet.firstPosition_ = bullet.secondPosition_;
        bullet.secondTime_ += updateInterval_;
        bullet.secondPosition_ += bullet.velocity_ * updateInterval_;
        bullet.secondDistance_ += updateInterval_ * bullet.velocity_.Length();
        bullet.velocity_ += physics_->GetGravity() * bullet.desc_.gravityFactor_ * updateInterval_;
        bullet.velocity_ -= bullet.velocity_ * updateInterval_ * bullet.desc_.airResistance_;

        PhysicsRaycastResult raycastResult;
        const Vector3 offset = bullet.secondPosition_ - bullet.firstPosition_;
        physics_->RaycastSingle(raycastResult, Ray(bullet.firstPosition_, offset), offset.Length());
        if (raycastResult.body_)
        {
            bullet.desc_.maxDistance_ = bullet.firstDistance_ + raycastResult.distance_;
        }
    }

    const float factor = InverseLerp(bullet.firstTime_, bullet.secondTime_, bullet.time_);
    bullet.position_ = Lerp(bullet.firstPosition_, bullet.secondPosition_, factor);
    bullet.distance_ = Lerp(bullet.firstDistance_, bullet.secondDistance_, factor);
    if (bullet.distance_ >= bullet.desc_.maxDistance_)
    {
        bullet.active_ = false;
    }

    return bullet.active_;
}

}
