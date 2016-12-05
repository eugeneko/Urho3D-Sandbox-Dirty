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

    /// Called before the first update. At this point all other components of the node should exist. Will also be called if update events are not wanted; in that case the event is immediately unsubscribed afterward.
    virtual void DelayedStart() override;
    /// Called on scene update, variable timestep.
    virtual void PostUpdate(float timeStep) override;
    /// Visualize the component as debug geometry.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

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

    Vector3 prevPosition_;

    bool wasFootstep_ = false;
    Vector3 footstepPosition_;
    Vector3 expectedPosition_;

    float fadeRemaining_ = 0;
    float movementRange_ = 0;
    Vector3 fadeDelta_ = Vector3::ZERO;
};

}
