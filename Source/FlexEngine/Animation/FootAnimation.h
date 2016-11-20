#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class Animation;
// class IndexBuffer;
// class Terrain;
// class VertexBuffer;
// struct WorkItem;
// class WorkQueue;

}

namespace FlexEngine
{

/// Grass patch data.
class FootAnimation : public LogicComponent
{
    URHO3D_OBJECT(FootAnimation, LogicComponent);

public:
    /// Construct.
    FootAnimation(Context* context);
    /// Destruct.
    virtual ~FootAnimation();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Called on scene update, variable timestep.
    virtual void PostUpdate(float timeStep) override;

    /// Set animation attribute.
    void SetAnimationAttr(const ResourceRef& value);
    /// Return animation attribute.
    ResourceRef GetAnimationAttr() const;

private:
    /// Source animation.
    SharedPtr<Animation> animation_;
    /// Foot bone name.
    String footBoneName_;
    /// Ground offset.
    Vector3 groundOffset_ = Vector3::ZERO;
    /// Ground normal.
    Vector3 groundNormal_ = Vector3::UP;
    /// Whether to adjust foot rotation. Set 0 to remain local transforms from animation as-is.
    float adjustFoot_ = 0.0f;
    /// Whether to adjust foot rotation to ground orientation. Set 0 to ignore ground normal.
    float adjustToGround_ = 0.0f;

    /// Animation time.
    float time_ = 0.0f;
};

}
