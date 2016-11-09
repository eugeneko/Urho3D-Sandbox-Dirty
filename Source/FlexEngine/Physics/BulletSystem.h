#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class PhysicsWorld;

}

namespace FlexEngine
{

class LineRenderer;

/// Bullet description.
struct BulletDesc
{
    /// Position.
    Vector3 position_;
    /// Velocity.
    Vector3 velocity_;
    /// Start bullet color.
    Color initialColor_;
    /// End bullet color.
    Color finalColor_;
    /// Gravity factor.
    float gravityFactor_;
    /// Air resistance factor.
    float airResistance_;
    /// Max distance.
    float maxDistance_;
    /// Thickness of trace.
    float traceThickness_;
    /// Length of trace.
    float traceLength_;
};

/// Bullet system.
class BulletSystem : public LogicComponent
{
    URHO3D_OBJECT(BulletSystem, LogicComponent);

public:
    /// Construct.
    BulletSystem(Context* context);
    /// Destruct.
    virtual ~BulletSystem();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Called on scene update, variable timestep.
    virtual void Update(float timeStep) override;

    /// Fire bullet.
    void FireBullet(const BulletDesc& bullet);

private:
    /// Bullet data.
    struct BulletData
    {
        /// Description.
        BulletDesc desc_;

        /// Is active?
        bool active_;

        /// Time of flight.
        float time_;
        /// Passed distance.
        float distance_;

        /// Current position.
        Vector3 position_;
        /// Current velocity.
        Vector3 velocity_;

        /// First time.
        float firstTime_;
        /// Second time.
        float secondTime_;
        /// First distance.
        float firstDistance_;
        /// Second distance.
        float secondDistance_;
        /// First position.
        Vector3 firstPosition_;
        /// Second position.
        Vector3 secondPosition_;
    };

    /// Update bullet.
    bool UpdateBullet(BulletData& bullet, float timeStep);

private:
    /// Physics engine.
    WeakPtr<PhysicsWorld> physics_;
    /// Line renderer.
    WeakPtr<LineRenderer> lineRenderer_;

    /// Integration time interval.
    float updateInterval_ = 0.05f;

    /// Array of bullets.
    Vector<BulletData> bullets_;
};

}
