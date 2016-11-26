#include <FlexEngine/Animation/FootAnimation.h>

// #include <FlexEngine/Math/MathDefs.h>
// #include <FlexEngine/Math/PoissonRandom.h>
// #include <FlexEngine/Math/StandardRandom.h>
//
#include <Urho3D/Core/Context.h>
// #include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/Log.h>
// #include <Urho3D/Graphics/Geometry.h>
// #include <Urho3D/Graphics/IndexBuffer.h>
// #include <Urho3D/Graphics/Terrain.h>
// #include <Urho3D/Graphics/VertexBuffer.h>
// #include <Urho3D/Graphics/Material.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Sphere.h>
// #include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{

/// Non-recursively get children bones by parent name.
PODVector<Bone*> GetChildren(Skeleton& skeleton, const String& parentName)
{
    PODVector<Bone*> result;
    const Bone* parent = skeleton.GetBone(parentName);
    const unsigned parentIndex = parent - skeleton.GetBone((unsigned)0);
    for (unsigned i = 0; i < skeleton.GetNumBones(); ++i)
        if (Bone* bone = skeleton.GetBone(i))
            if (bone->parentIndex_ == parentIndex)
                result.Push(bone);
    return result;
}

/// Names of foot bones.
struct FootBoneNames
{
    String thigh_;
    String calf_;
    String heel_;
};

/// Get names of thigh, calf and heel bones.
FootBoneNames GetFootBones(Skeleton& skeleton, const String& thighName)
{
    FootBoneNames result;
    result.thigh_ = thighName;
    PODVector<Bone*> thighChildren = GetChildren(skeleton, result.thigh_);
    if (!thighChildren.Empty())
    {
        result.calf_ = thighChildren[0]->name_;
        PODVector<Bone*> calfChildren = GetChildren(skeleton, result.calf_);
        if (!calfChildren.Empty())
            result.heel_ = calfChildren[0]->name_;
    }
    return result;
}

/// Rotate node to make child match its position. Returns true if successfully matched.
bool MatchChildPosition(Node& parent, Node& child, const Vector3& newChildPosition)
{
    const Vector3 parentPosition = parent.GetWorldPosition();
    const Vector3 childPosition = child.GetWorldPosition();
    const Quaternion thighRotation(childPosition - parentPosition, newChildPosition - parentPosition);
    parent.SetWorldRotation(thighRotation * parent.GetWorldRotation());

    return Equals(0.0f, (newChildPosition - child.GetWorldPosition()).LengthSquared());
}

/// Animation state of single foot animation.
struct FootAnimationState
{
    /// Thigh position.
    Vector3 thighPosition_;
    /// Heel position.
    Vector3 heelPosition_;
    /// Knee rotation.
    float kneeRotation_;
};

/// Resolved animation state of single foot animation.
struct FootAnimationStateResolved
{
    /// Thigh position.
    Vector3 thighPosition_;
    /// Calf position.
    Vector3 calfPosition_;
    /// Heel position.
    Vector3 heelPosition_;
};

/// Foot animation key frame.
struct FootAnimationKeyFrame
{
    /// Key frame time.
    float time_ = 0.0f;
    /// Heel position.
    Vector3 heelPosition_;
    /// Knee rotation. Contains only roll component.
    Quaternion kneeRotation_;
    /// Fix for thigh rotation.
    Quaternion thighRotationFix_;
    /// Fix for calf rotation.
    Quaternion calfRotationFix_;
    /// Heel rotation in local space.
    Quaternion heelRotationLocal_;
    /// Heel rotation in world space.
    Quaternion heelRotationWorld_;
};

/// Foot animation track.
struct FootAnimationTrack
{
    /// Key frames.
    Vector<FootAnimationKeyFrame> keyFrames_;

    /// Get length.
    float GetLength() const
    {
        return keyFrames_.Back().time_ - keyFrames_.Front().time_;
    }
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
    FootAnimationKeyFrame SampleFrame(float time, unsigned& frame) const
    {
        GetKeyFrameIndex(time, frame);
        const unsigned nextFrame = frame + 1 < keyFrames_.Size() ? frame + 1 : 0;
        const FootAnimationKeyFrame* keyFrame = &keyFrames_[frame];
        const FootAnimationKeyFrame* nextKeyFrame = &keyFrames_[nextFrame];
        float timeInterval = nextKeyFrame->time_ - keyFrame->time_;
        if (timeInterval < 0.0f)
            timeInterval += GetLength();

        float t = timeInterval > 0.0f ? (time - keyFrame->time_) / timeInterval : 1.0f;

        FootAnimationKeyFrame result;
        result.time_ = time;
        result.heelPosition_ = Lerp(keyFrame->heelPosition_, nextKeyFrame->heelPosition_, t);
        result.kneeRotation_ = keyFrame->kneeRotation_.Slerp(nextKeyFrame->kneeRotation_, t);
        result.thighRotationFix_ = keyFrame->thighRotationFix_.Slerp(nextKeyFrame->thighRotationFix_, t);
        result.calfRotationFix_ = keyFrame->calfRotationFix_.Slerp(nextKeyFrame->calfRotationFix_, t);
        result.heelRotationLocal_ = keyFrame->heelRotationLocal_.Slerp(nextKeyFrame->heelRotationLocal_, t);
        result.heelRotationWorld_ = keyFrame->heelRotationWorld_.Slerp(nextKeyFrame->heelRotationWorld_, t);
        return result;
    }
};

/// Eliminates all but the first element from every consecutive group of equivalent elements from the range [first, last) and returns a past-the-end iterator for the new logical end of the range.
template <class TIterator>
TIterator Unique(TIterator first, TIterator last)
{
    if (first == last)
        return last;

    TIterator result = first;
    while (++first != last)
    {
        if (!(*result == *first))
        {
            *(++result) = *first;
        }
    }
    return ++result;
}

/// Append track times to result array.
void AppendTrackTimes(PODVector<float>& result, AnimationTrack& track)
{
    for (unsigned i = 0; i < track.GetNumKeyFrames(); ++i)
        result.Push(track.GetKeyFrame(i)->time_);
}

/// Get signed angle between two vectors.
float AngleSigned(const Vector3& lhs, const Vector3& rhs, const Vector3& base)
{
    return lhs.Angle(rhs) * (lhs.CrossProduct(rhs).DotProduct(base) < 0 ? 1 : -1);
}

/// Mix in quaternion to another with specified weight. Returns new weight of result quaternion.
Quaternion MixQuaternion(Quaternion& lhs, const Quaternion& rhs, float weight, float totalWeight)
{
    // Nothing to mix at right
    if (weight < M_EPSILON)
        return lhs;

    // Nothing to mix at left
    if (totalWeight < M_EPSILON)
        return rhs;

    return lhs.Slerp(rhs, weight / (weight + totalWeight));
}

/// Get angle of knee rotation.
float GetKneeRotationAngle(const Vector3& thighPosition, const Vector3& calfPosition, const Vector3& heelPosition,
    const Vector3& forwardDirection)
{
    const Vector3 footDirection = (heelPosition - thighPosition).Normalized();
    const float kneeDistance = (calfPosition - thighPosition).ProjectOntoAxis(footDirection);
    const Vector3 kneeDirection = calfPosition - (thighPosition + footDirection * kneeDistance);
    const Vector3 baseKneeDirection = footDirection.CrossProduct(forwardDirection).CrossProduct(footDirection).Normalized();
    return AngleSigned(kneeDirection, baseKneeDirection, footDirection);
}

void CreateFootAnimationTrack(FootAnimationTrack& track, Model* model, Animation* animation, const String& thighName)
{
    if (!model)
        return;

    // Setup temporary node for playback
    Node node(model->GetContext());
    AnimatedModel* animatedModel = node.CreateComponent<AnimatedModel>();
    animatedModel->SetModel(model);
    AnimationState* animationState = animatedModel->AddAnimationState(animation);
    animationState->SetWeight(1.0f);

    // Parse skeleton
    Skeleton& skeleton = animatedModel->GetSkeleton();
    const FootBoneNames boneNames = GetFootBones(skeleton, thighName);

    // Get related nodes and bones
    Node* thighNode = node.GetChild(boneNames.thigh_, true);
    Node* calfNode = thighNode ? thighNode->GetChild(boneNames.calf_) : nullptr;
    Node* heelNode = calfNode ? calfNode->GetChild(boneNames.heel_) : nullptr;
    Bone* thighBone = skeleton.GetBone(boneNames.thigh_);
    Bone* calfBone = skeleton.GetBone(boneNames.calf_);
    Bone* heelBone = skeleton.GetBone(boneNames.heel_);
    if (!thighNode || !calfNode || !heelNode || !thighBone || !calfBone || !heelBone)
        return;

    // Get tracks
    PODVector<float> times;
    AnimationTrack* thighTrack = animation->GetTrack(boneNames.thigh_);
    AnimationTrack* calfTrack = animation->GetTrack(boneNames.calf_);
    AnimationTrack* heelTrack = animation->GetTrack(boneNames.heel_);
    AppendTrackTimes(times, *thighTrack);
    AppendTrackTimes(times, *calfTrack);
    AppendTrackTimes(times, *heelTrack);

    Sort(times.Begin(), times.End());
    times.Erase(Unique(times.Begin(), times.End()), times.End());

    // Play animation and convert pose
    const unsigned numKeyFrames = times.Size();
    track.keyFrames_.Resize(numKeyFrames);
    for (unsigned i = 0; i < numKeyFrames; ++i)
    {
        // Play animation
        animationState->SetTime(times[i]);
        animationState->Apply();
        node.MarkDirty();

        // Receive animation data
        const Vector3 thighPosition = thighNode->GetWorldPosition();
        const Vector3 calfPosition = calfNode->GetWorldPosition();
        const Vector3 heelPosition = heelNode->GetWorldPosition();
        const Quaternion thighRotation = thighNode->GetRotation();
        const Quaternion calfRotation = calfNode->GetRotation();

        // Revert to bind pose
        thighNode->SetTransform(thighBone->initialPosition_, thighBone->initialRotation_, thighBone->initialScale_);
        calfNode->SetTransform(calfBone->initialPosition_, calfBone->initialRotation_, calfBone->initialScale_);

        // Try to resolve foot shape and generate fix angles
        MatchChildPosition(*thighNode, *calfNode, calfPosition);
        const Quaternion thighRotationFix = thighNode->GetRotation().Inverse() * thighRotation;
        thighNode->SetRotation(thighNode->GetRotation() * thighRotationFix);

        MatchChildPosition(*calfNode, *heelNode, heelPosition);
        const Quaternion calfRotationFix = calfNode->GetRotation().Inverse() * calfRotation;
        calfNode->SetRotation(calfNode->GetRotation() * calfRotationFix);

        // Make new frame
        track.keyFrames_[i].time_ = times[i];
        track.keyFrames_[i].heelPosition_ = heelPosition;
        track.keyFrames_[i].kneeRotation_ = Quaternion(GetKneeRotationAngle(thighPosition, calfPosition, heelPosition, Vector3::FORWARD));
        track.keyFrames_[i].thighRotationFix_ = thighRotationFix;
        track.keyFrames_[i].calfRotationFix_ = calfRotationFix;
        track.keyFrames_[i].heelRotationLocal_ = heelNode->GetRotation();
        track.keyFrames_[i].heelRotationWorld_ = heelNode->GetWorldRotation();
    }
}

/// Intersect sphere and sphere. If there is no intersection point, second sphere is moved toward first one.
void IntersectSphereSphereGuaranteed(const Sphere& first, const Sphere& second, float& distance, float& radius)
{
    // http://mathworld.wolfram.com/Sphere-SphereIntersection.html
    const float R = first.radius_;
    const float r = second.radius_;
    const float d = Min(R + r, (second.center_ - first.center_).Length());
    radius = Sqrt(Max(0.0f, (-d + r - R) * (-d - r + R) * (-d + r + R) * (d + r + R))) / (2 * d);
    distance = Sqrt(R * R - radius * radius);
}

/// Resolve knee position.
Vector3 ResolveKneePosition(const Vector3& thighPosition, const Vector3& targetHeelPosition, const Vector3& forwardDirection,
    float thighLength, float calfLength, float kneeAngle)
{
    float distance;
    float radius;
    IntersectSphereSphereGuaranteed(Sphere(thighPosition, thighLength), Sphere(targetHeelPosition, calfLength), distance, radius);
    const Vector3 direction = (targetHeelPosition - thighPosition).Normalized();
    const Vector3 kneeY = direction.CrossProduct(forwardDirection).Normalized();
    const Vector3 kneeX = kneeY.CrossProduct(direction).Normalized();
    return thighPosition + direction * distance + kneeX * Cos(kneeAngle) * radius  + kneeY * Sin(kneeAngle) * radius;
}

//////////////////////////////////////////////////////////////////////////
FootAnimation::FootAnimation(Context* context)
    : LogicComponent(context)
{
}

FootAnimation::~FootAnimation()
{

}

void FootAnimation::RegisterObject(Context* context)
{
    context->RegisterFactory<FootAnimation>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(LogicComponent);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animation", GetAnimationAttr, SetAnimationAttr, ResourceRef, ResourceRef(Animation::GetTypeNameStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Foot Root Bone", String, footBoneName_, "", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Ground Offset", Vector3, groundOffset_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Adjust Foot", float, adjustFoot_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Adjust to Ground", float, adjustToGround_, 0.0f, AM_DEFAULT);
}

void FootAnimation::ApplyAttributes()
{
    if (node_)
    {
    }
}

void FootAnimation::DelayedStart()
{
    prevPosition_ = node_->GetPosition();
    state_ = 0;
}

void FootAnimation::PostUpdate(float timeStep)
{
    if (!animation_ || !node_)
        return;


    if (Node* plane = node_->GetChild("Plane"))
    {
        groundOffset_ = plane->GetPosition();
        groundNormal_ = plane->GetRotation() * Vector3::UP;
    }

    const Vector3 velocity = (node_->GetPosition() - prevPosition_) / timeStep;
    prevPosition_ = node_->GetPosition();

    AnimatedModel* model = node_->GetComponent<AnimatedModel>();

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SharedPtr<Animation> animForward(cache->GetResource<Animation>("Swat_WalkFwd.ani"));
    SharedPtr<Animation> animBackward(cache->GetResource<Animation>("Swat_WalkBwd.ani"));
    SharedPtr<Animation> animLeft(cache->GetResource<Animation>("Swat_WalkLeft.ani"));
    SharedPtr<Animation> animRight(cache->GetResource<Animation>("Swat_WalkRight.ani"));
    SharedPtr<Animation> animIdle(cache->GetResource<Animation>("Swat_WalkRight.ani"));
    FootAnimationTrack trackForward, trackBackward, trackLeft, trackRight, trackIdle;
    CreateFootAnimationTrack(trackForward, model->GetModel(), animForward, footBoneName_);
    CreateFootAnimationTrack(trackBackward, model->GetModel(), animBackward, footBoneName_);
    CreateFootAnimationTrack(trackLeft, model->GetModel(), animLeft, footBoneName_);
    CreateFootAnimationTrack(trackRight, model->GetModel(), animRight, footBoneName_);
    CreateFootAnimationTrack(trackIdle, model->GetModel(), animIdle, footBoneName_);

    Skeleton& skeleton = model->GetSkeleton();
    Node* thighNode = node_->GetChild(footBoneName_, true);
    Node* calfNode = thighNode->GetChildren()[0];
    Node* heelNode = calfNode->GetChildren()[0];
    thighNode->SetRotationSilent(model->GetSkeleton().GetBone(thighNode->GetName())->initialRotation_);
    calfNode->SetRotationSilent(model->GetSkeleton().GetBone(calfNode->GetName())->initialRotation_);
    heelNode->SetRotationSilent(model->GetSkeleton().GetBone(heelNode->GetName())->initialRotation_);
    thighNode->MarkDirty();
    const float thighLength = (thighNode->GetWorldPosition() - calfNode->GetWorldPosition()).Length();
    const float calfLength = (calfNode->GetWorldPosition() - heelNode->GetWorldPosition()).Length();

    unsigned frameIndex = 0;
    const FootAnimationKeyFrame frameForward = trackForward.SampleFrame(time_, frameIndex);
    const FootAnimationKeyFrame frameBackward = trackBackward.SampleFrame(time_, frameIndex);
    const FootAnimationKeyFrame frameLeft = trackLeft.SampleFrame(time_, frameIndex);
    const FootAnimationKeyFrame frameRight = trackRight.SampleFrame(time_, frameIndex);
    const FootAnimationKeyFrame frameIdle = trackIdle.SampleFrame(time_, frameIndex);
//     const Vector3 lookDirection = node_->GetRotation() * Vector3::BACK;
//     Vector4 interpolateFactors;
//     interpolateFactors.x_ = Max(0.0f, -lookDirection.z_);
//     interpolateFactors.y_ = Max(0.0f, lookDirection.z_);
//     interpolateFactors.z_ = Max(0.0f, -lookDirection.x_);
//     interpolateFactors.w_ = Max(0.0f, lookDirection.x_);
    const Vector3 fixedVelocity = velocity / 1.7f;
//     float nonIdleFactor = fixedVelocity.Length();
    const Vector3 moveDirection = velocity.LengthSquared() > M_EPSILON ? velocity.Normalized() : Vector3::BACK;
    Vector4 interpolateFactors;
    interpolateFactors.x_ = Max(0.0f, -moveDirection.z_);
    interpolateFactors.y_ = Max(0.0f, moveDirection.z_);
    interpolateFactors.z_ = Max(0.0f, moveDirection.x_);
    interpolateFactors.w_ = Max(0.0f, -moveDirection.x_);
    interpolateFactors /= interpolateFactors.x_ + interpolateFactors.y_ + interpolateFactors.z_ + interpolateFactors.w_;
    FootAnimationKeyFrame frame;
    frame.time_ = frameForward.time_;
    float weight = 0.0f;
    auto makeMeHappy = [&frame, &weight](const FootAnimationKeyFrame& rhs, float factor)
    {
        frame.heelPosition_ += rhs.heelPosition_ * factor;
        frame.kneeRotation_ = MixQuaternion(frame.kneeRotation_, rhs.kneeRotation_, factor, weight);
        frame.thighRotationFix_ = MixQuaternion(frame.thighRotationFix_, rhs.thighRotationFix_, factor, weight);
        frame.calfRotationFix_ = MixQuaternion(frame.calfRotationFix_, rhs.calfRotationFix_, factor, weight);
        frame.heelRotationLocal_ = MixQuaternion(frame.heelRotationLocal_, rhs.heelRotationLocal_, factor, weight);
        frame.heelRotationWorld_ = MixQuaternion(frame.heelRotationWorld_, rhs.heelRotationWorld_, factor, weight);
        weight += factor;
    };
    makeMeHappy(frameForward, interpolateFactors.x_);
    makeMeHappy(frameBackward, interpolateFactors.y_);
    makeMeHappy(frameLeft, interpolateFactors.z_);
    makeMeHappy(frameRight, interpolateFactors.w_);

    AnimationController* controller = node_->GetComponent<AnimationController>();
    controller->SetWeight(animForward->GetName(), Max(0.00001f, interpolateFactors.x_));
    controller->SetWeight(animBackward->GetName(), Max(0.00001f, interpolateFactors.y_));
    controller->SetWeight(animLeft->GetName(), Max(0.00001f, interpolateFactors.z_));
    controller->SetWeight(animRight->GetName(), Max(0.00001f, interpolateFactors.w_));
    controller->SetTime(animForward->GetName(), time_);
    controller->SetTime(animBackward->GetName(), time_);
    controller->SetTime(animLeft->GetName(), time_);
    controller->SetTime(animRight->GetName(), time_);

    const Vector3 forwardDirection = node_->GetRotation() * Vector3::FORWARD;

    // Resolve foot shape
    const Vector3 newHeelPosition = node_->GetWorldTransform() * (Quaternion(Vector3::UP, groundNormal_) * frame.heelPosition_ + groundOffset_);
    const Vector3 newCalfPosition = ResolveKneePosition(thighNode->GetWorldPosition(), newHeelPosition, forwardDirection,
        thighLength, calfLength, frame.kneeRotation_.RollAngle());

    // Apply foot shape
    if (!MatchChildPosition(*thighNode, *calfNode, newCalfPosition))
        URHO3D_LOGWARNING("Failed to resolve thigh-calf segment of foot animation");
    thighNode->SetRotation(thighNode->GetRotation() * frame.thighRotationFix_);

    if (!MatchChildPosition(*calfNode, *heelNode, newHeelPosition))
        URHO3D_LOGWARNING("Failed to resolve calf-heel segment of foot animation");
    calfNode->SetRotation(calfNode->GetRotation() * frame.calfRotationFix_);

    // Resolve heel rotation
    const Quaternion origHeelRotation = calfNode->GetWorldRotation() * frame.heelRotationLocal_;
    const Quaternion fixedHeelRotation = node_->GetWorldRotation() * frame.heelRotationWorld_;
    const Quaternion adjustToGoundRotation = Quaternion::IDENTITY.Slerp(Quaternion(Vector3::UP, groundNormal_), adjustToGround_);
    heelNode->SetWorldRotation(adjustToGoundRotation * origHeelRotation.Slerp(fixedHeelRotation, adjustFoot_));

    thighNode->MarkDirty();
    time_ = Mod(time_ + timeStep, trackForward.GetLength());
}

void FootAnimation::SetAnimationAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    animation_ = cache->GetResource<Animation>(value.name_);
}

ResourceRef FootAnimation::GetAnimationAttr() const
{
    return GetResourceRef(animation_, Animation::GetTypeStatic());
}

}
