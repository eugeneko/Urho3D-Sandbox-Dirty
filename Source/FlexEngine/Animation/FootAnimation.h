#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class Animation;
// class IndexBuffer;
// class Terrain;
// class VertexBuffer;
// struct WorkItem;
// class WorkQueue;
class Model;

}

namespace FlexEngine
{

/// Blend animations and return result.
SharedPtr<Animation> BlendAnimations(Model& model, const PODVector<Animation*>& animations,
    const PODVector<float>& weights, const PODVector<float>& offsets, const PODVector<float>& timestamps);

/// Description of character skeleton 2-segment.
struct CharacterSkeletonSegment2
{
    /// Name of the bone that is related to the root position of the segment.
    String rootBone_;
    /// Name of the bone that is related to the joint position of the segment. Must be a child of the root bone.
    String jointBone_;
    /// Name of the bone that is related to the target position of the segment. Must be a child of the joint bone.
    String targetBone_;
};

/// Character skeleton.
class CharacterSkeleton : public Resource
{
    URHO3D_OBJECT(CharacterSkeleton, Resource);

public:
    /// Container of 2-segment.
    using Segment2Map = HashMap<String, CharacterSkeletonSegment2>;

public:
    /// Construct.
    CharacterSkeleton(Context* context);
    /// Destruct.
    virtual ~CharacterSkeleton();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);

    /// Get 2-segments.
    const Segment2Map& GetSegments2() const { return segments2_; }

private:
    /// Container of 2-segments.
    Segment2Map segments2_;
};


/// Key frame of 2-segment animation track.
// #TODO Rename members!
struct CharacterAnimationSegment2KeyFrame
{
    /// Key frame time.
    float time_ = 0.0f;
    /// Heel position.
    Vector3 heelPosition_;
    /// Direction of knee.
    Vector3 kneeDirection_;
    /// Fix for thigh rotation.
    Quaternion thighRotationFix_;
    /// Fix for calf rotation.
    Quaternion calfRotationFix_;
    /// Heel rotation in local space.
    Quaternion heelRotationLocal_;
    /// Heel rotation in world space.
    Quaternion heelRotationWorld_;
};

/// Animation track of single 2-segment.
// #TODO Rename members!
struct CharacterAnimationSegment2Track
{
    /// Base direction is used for resolving joint angle.
    Vector3 initialDirection_;
    /// Key frames.
    Vector<CharacterAnimationSegment2KeyFrame> keyFrames_;

    /// Get length of track.
    float GetLength() const { return keyFrames_.Empty() ? 0 : keyFrames_.Back().time_ - keyFrames_.Front().time_; }
    /// Get index of key frame.
    void GetKeyFrameIndex(float time, unsigned& index) const
    {
        if (time < 0.0f)
            time = 0.0f;

        if (index >= keyFrames_.Size())
            index = keyFrames_.Size() - 1;

        // Check for being too far ahead
        while (index && time < keyFrames_[index].time_)
            --index;

        // Check for being too far behind
        while (index < keyFrames_.Size() - 1 && time >= keyFrames_[index + 1].time_)
            ++index;
    }
    /// Sample frame at specified time.
    CharacterAnimationSegment2KeyFrame SampleFrame(float time, unsigned& frame) const
    {
        GetKeyFrameIndex(time, frame);
        const unsigned nextFrame = frame + 1 < keyFrames_.Size() ? frame + 1 : 0;
        const CharacterAnimationSegment2KeyFrame* keyFrame = &keyFrames_[frame];
        const CharacterAnimationSegment2KeyFrame* nextKeyFrame = &keyFrames_[nextFrame];
        float timeInterval = nextKeyFrame->time_ - keyFrame->time_;
        if (timeInterval < 0.0f)
            timeInterval += GetLength();

        float t = timeInterval > 0.0f ? (time - keyFrame->time_) / timeInterval : 1.0f;

        CharacterAnimationSegment2KeyFrame result;
        result.time_ = time;
        result.heelPosition_ = Lerp(keyFrame->heelPosition_, nextKeyFrame->heelPosition_, t);
        result.kneeDirection_ = Lerp(keyFrame->kneeDirection_, nextKeyFrame->kneeDirection_, t);
        result.thighRotationFix_ = keyFrame->thighRotationFix_.Slerp(nextKeyFrame->thighRotationFix_, t);
        result.calfRotationFix_ = keyFrame->calfRotationFix_.Slerp(nextKeyFrame->calfRotationFix_, t);
        result.heelRotationLocal_ = keyFrame->heelRotationLocal_.Slerp(nextKeyFrame->heelRotationLocal_, t);
        result.heelRotationWorld_ = keyFrame->heelRotationWorld_.Slerp(nextKeyFrame->heelRotationWorld_, t);
        return result;
    }
};

/// Character Animation.
class CharacterAnimation : public Resource
{
    URHO3D_OBJECT(CharacterAnimation, Resource);

public:
    /// Map of tracks for 2-segments.
    using Segment2TrackMap = HashMap<String, CharacterAnimationSegment2Track>;
public:
    /// Construct.
    CharacterAnimation(Context* context);
    /// Destruct.
    virtual ~CharacterAnimation();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Save resource. Return true if successful.
    virtual bool Save(Serializer& dest) const;

    /// Load from an XML element. Return true if successful.
    bool Load(const XMLElement& source);
    /// Save to an XML element. Return true if successful.
    bool Save(XMLElement& dest) const;

    /// Import animation using model and skeleton.
    bool ImportAnimation(CharacterSkeleton& characterSkeleton, Model& model, Animation& animation);

private:
    /// Tracks for 2-segments.
    Segment2TrackMap segments2_;

};

/// .
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
