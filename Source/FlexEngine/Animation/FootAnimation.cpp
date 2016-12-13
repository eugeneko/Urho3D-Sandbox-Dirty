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
#include <Urho3D/Graphics/DebugRenderer.h>
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

SharedPtr<Animation> BlendAnimations(Model& model, const PODVector<Animation*>& animations,
    const PODVector<float>& weights, const PODVector<float>& offsets, const PODVector<float>& timestamps)
{
    // Create and setup node
    Node node(model.GetContext());
    AnimatedModel* animatedModel = node.CreateComponent<AnimatedModel>();
    animatedModel->SetModel(&model);
    for (unsigned i = 0; i < animations.Size(); ++i)
    {
        Animation* animation = animations[i];
        AnimationState* animationState = animatedModel->AddAnimationState(animation);
        animationState->SetWeight(i < weights.Size() ? weights[i] : 1.0f);
    }

    // Get all nodes
    SharedPtr<Animation> result = MakeShared<Animation>(model.GetContext());
    Node* rootNode = animatedModel->GetSkeleton().GetRootBone()->node_;
    if (!rootNode)
        return result;

    PODVector<Node*> nodes;
    rootNode->GetChildren(nodes, true);

    // Create tracks
    for (Node* node : nodes)
        if (!node->GetName().Empty())
        {
            AnimationTrack* track = result->CreateTrack(node->GetName());
            track->channelMask_ = CHANNEL_POSITION | CHANNEL_ROTATION | CHANNEL_SCALE;
        }

    // Play animation
    const Vector<SharedPtr<AnimationState>>& animationStates = animatedModel->GetAnimationStates();
    for (unsigned i = 0; i < timestamps.Size(); ++i)
    {
        const float time = timestamps[i];

        // Reset nodes
        for (Node* node : nodes)
            if (Bone* bone = model.GetSkeleton().GetBone(node->GetName()))
                node->SetTransform(bone->initialPosition_, bone->initialRotation_, bone->initialScale_);

        // Play animation
        for (unsigned j = 0; j < animationStates.Size(); ++j)
        {
            float timeOffset = j < offsets.Size() ? offsets[j] : 0.0f;
            animationStates[j]->SetTime(time + timeOffset);
            animationStates[j]->Apply();
        }

        node.MarkDirty();

        // Write tracks
        for (Node* node : nodes)
        {
            if (AnimationTrack* track = result->GetTrack(node->GetName()))
            {
                AnimationKeyFrame keyFrame;
                keyFrame.time_ = time;
                keyFrame.position_ = node->GetPosition();
                keyFrame.rotation_ = node->GetRotation();
                keyFrame.scale_ = node->GetScale();
                track->AddKeyFrame(keyFrame);
            }
        }
    }

    return result;
}

/// Merge times of animation tracks.
PODVector<float> MergeAnimationTrackTimes(const PODVector<AnimationTrack*>& tracks)
{
    PODVector<float> result;
    for (AnimationTrack* track : tracks)
    {
        if (track)
        {
            for (unsigned i = 0; i < track->GetNumKeyFrames(); ++i)
                result.Push(track->GetKeyFrame(i)->time_);
        }
    }
    Sort(result.Begin(), result.End());
    result.Erase(Unique(result.Begin(), result.End()), result.End());
    return result;
}

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

/// Foot animation track.
struct FootAnimationTrack
{
    Vector3 initialDirection_;
    /// Key frames.
    Vector<FootAnimationKeyFrame> keyFrames_;
    /// Static ranges.
    Vector<Pair<float, float>> staticRanges_;

    bool IsStatic(float time) const
    {
        for (const Pair<float, float>& range : staticRanges_)
            if (range.first_ <= time && time <= range.second_)
                return true;
        return false;
    }
    float GetMomementRange(float time) const
    {
        float result = 0.0f;
        for (const Pair<float, float>& range : staticRanges_)
            if (time < range.first_)
                return range.first_ - time;
        return staticRanges_.Empty() ? 1.0f : staticRanges_.Front().first_ - time + GetLength();
    }
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
        result.kneeDirection_ = Lerp(keyFrame->kneeDirection_, nextKeyFrame->kneeDirection_, t);
        result.thighRotationFix_ = keyFrame->thighRotationFix_.Slerp(nextKeyFrame->thighRotationFix_, t);
        result.calfRotationFix_ = keyFrame->calfRotationFix_.Slerp(nextKeyFrame->calfRotationFix_, t);
        result.heelRotationLocal_ = keyFrame->heelRotationLocal_.Slerp(nextKeyFrame->heelRotationLocal_, t);
        result.heelRotationWorld_ = keyFrame->heelRotationWorld_.Slerp(nextKeyFrame->heelRotationWorld_, t);
        return result;
    }
};

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

void CreateFootAnimationTrack(FootAnimationTrack& track, Model* model, Animation* animation, const String& thighName,
    const Vector3& velocity, float threshold)
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

    // Get initial pose
    track.initialDirection_ = heelNode->GetWorldPosition() - thighNode->GetWorldPosition();

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
    Vector<Vector3> globalPositions;
    float minHeight = M_INFINITY;
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

        // Get knee direction
        const Vector3 direction = (heelPosition - thighPosition).Normalized();
        const Vector3 jointProjection = direction * (calfPosition - thighPosition).ProjectOntoAxis(direction) + thighPosition;
        const Vector3 jointDirection = Quaternion(direction, track.initialDirection_) * (calfPosition - jointProjection);

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
        track.keyFrames_[i].kneeDirection_ = jointDirection.LengthSquared() > M_EPSILON ? jointDirection.Normalized() : Vector3::FORWARD;
        track.keyFrames_[i].thighRotationFix_ = thighRotationFix;
        track.keyFrames_[i].calfRotationFix_ = calfRotationFix;
        track.keyFrames_[i].heelRotationLocal_ = heelNode->GetRotation();
        track.keyFrames_[i].heelRotationWorld_ = heelNode->GetWorldRotation();
        globalPositions.Push(heelPosition + velocity * times[i]);
        minHeight = Min(minHeight, globalPositions.Back().y_);
    }

    float rangeBegin = -1;
    bool wasStatic = false;
    for (unsigned i = 0; i < numKeyFrames; ++i)
    {
        const bool isStatic = globalPositions[i].y_ < minHeight + threshold;
        if (isStatic && !wasStatic)
            rangeBegin = times[i];
        else if (wasStatic && !isStatic)
            track.staticRanges_.Push(MakePair(rangeBegin, times[i]));
        wasStatic = isStatic;
    }
    if (wasStatic)
        track.staticRanges_.Push(MakePair(rangeBegin, times.Back()));
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
Vector3 ResolveKneePosition(const Vector3& thighPosition, const Vector3& targetHeelPosition, const Vector3& jointDirection,
    float thighLength, float calfLength)
{
    float distance;
    float radius;
    IntersectSphereSphereGuaranteed(Sphere(thighPosition, thighLength), Sphere(targetHeelPosition, calfLength), distance, radius);
    const Vector3 direction = (targetHeelPosition - thighPosition).Normalized();
    return thighPosition + direction * distance + jointDirection.Normalized() * radius;
}

//////////////////////////////////////////////////////////////////////////
CharacterSkeleton::CharacterSkeleton(Context* context)
    : Resource(context)
{
}


CharacterSkeleton::~CharacterSkeleton()
{
}

void CharacterSkeleton::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterSkeleton>();
}

bool CharacterSkeleton::BeginLoad(Deserializer& source)
{
    SharedPtr<XMLFile> xmlFile = MakeShared<XMLFile>(context_);
    if (xmlFile->Load(source))
        return Load(xmlFile->GetRoot());
    return false;
}

bool CharacterSkeleton::Load(const XMLElement& source)
{
    for (XMLElement segmentNode = source.GetChild("segment2"); !segmentNode.IsNull(); segmentNode = segmentNode.GetNext("segment2"))
    {
        CharacterSkeletonSegment2 segment;
        segment.rootBone_ = segmentNode.GetAttribute("root");
        segment.jointBone_ = segmentNode.GetAttribute("joint");
        segment.targetBone_ = segmentNode.GetAttribute("target");
        const String name = segmentNode.GetAttribute("name");
        segments2_.Populate(name, segment);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
CharacterAnimation::CharacterAnimation(Context* context)
    : Resource(context)
{
}

CharacterAnimation::~CharacterAnimation()
{
}

void CharacterAnimation::RegisterObject(Context* context)
{
    context->RegisterFactory<CharacterAnimation>();
}

bool CharacterAnimation::BeginLoad(Deserializer& source)
{
    SharedPtr<XMLFile> xmlFile = MakeShared<XMLFile>(context_);
    if (xmlFile->Load(source))
        return Load(xmlFile->GetRoot());
    return false;
}

bool CharacterAnimation::Load(const XMLElement& source)
{
    for (XMLElement segmentNode = source.GetChild("segment2"); !segmentNode.IsNull(); segmentNode = segmentNode.GetNext("segment2"))
    {
        const String name = segmentNode.GetAttribute("name");
        CharacterAnimationSegment2Track track;
        track.initialDirection_ = segmentNode.GetVector3("baseDirection");

        for (XMLElement keyFrameNode = segmentNode.GetChild("segment2");
            !keyFrameNode.IsNull();
            keyFrameNode = keyFrameNode.GetNext("segment2"))
        {
            CharacterAnimationSegment2KeyFrame keyFrame;
            keyFrame.time_ = keyFrameNode.GetFloat("time");
            keyFrame.heelPosition_ = keyFrameNode.GetVector3("targetPosition");
            keyFrame.kneeDirection_ = keyFrameNode.GetVector3("jointOrientation");
            keyFrame.thighRotationFix_ = keyFrameNode.GetQuaternion("rootRotation");
            keyFrame.calfRotationFix_ = keyFrameNode.GetQuaternion("jointRotation");
            keyFrame.heelRotationLocal_ = keyFrameNode.GetQuaternion("targetRotation");
            keyFrame.heelRotationWorld_ = keyFrameNode.GetQuaternion("targetRotationWorld");
            track.keyFrames_.Push(keyFrame);
        }
        segments2_.Populate(name, track);
    }
    return true;
}

bool CharacterAnimation::Save(XMLElement& dest) const
{
    for (const Segment2TrackMap::KeyValue& elem : segments2_)
    {
        XMLElement segment2Node = dest.CreateChild("segment2");
        segment2Node.SetAttribute("name", elem.first_);
        const CharacterAnimationSegment2Track& track = elem.second_;
        segment2Node.SetVector3("baseDirection", track.initialDirection_);
        for (const CharacterAnimationSegment2KeyFrame& keyFrame : track.keyFrames_)
        {
            XMLElement keyFrameNode = segment2Node.CreateChild("keyFrame");
            keyFrameNode.SetFloat("time", keyFrame.time_);
            keyFrameNode.SetVector3("targetPosition", keyFrame.heelPosition_);
            keyFrameNode.SetVector3("jointOrientation", keyFrame.kneeDirection_);
            keyFrameNode.SetQuaternion("rootRotation", keyFrame.thighRotationFix_);
            keyFrameNode.SetQuaternion("jointRotation", keyFrame.calfRotationFix_);
            keyFrameNode.SetQuaternion("targetRotation", keyFrame.heelRotationLocal_);
            keyFrameNode.SetQuaternion("targetRotationWorld", keyFrame.heelRotationWorld_);
        }
    }
    return true;
}

bool CharacterAnimation::Save(Serializer& dest) const
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement animationNode = xml->CreateRoot("animation");

    Save(animationNode);
    return xml->Save(dest);
}

bool CharacterAnimation::ImportAnimation(CharacterSkeleton& characterSkeleton, Model& model, Animation& animation)
{
    // Setup temporary node for playback
    Node node(context_);
    AnimatedModel* animatedModel = node.CreateComponent<AnimatedModel>();
    animatedModel->SetModel(&model);
    AnimationState* animationState = animatedModel->AddAnimationState(&animation);
    animationState->SetWeight(1.0f);
    Skeleton& skeleton = animatedModel->GetSkeleton();

    // Create tracks of 2-segments
    Segment2TrackMap segments2;
    for (const CharacterSkeleton::Segment2Map::KeyValue& elem : characterSkeleton.GetSegments2())
    {
        const String& name = elem.first_;
        const CharacterSkeletonSegment2& joint = elem.second_;

        // Get nodes and bones of segment
        Node* segmentRootNode = node.GetChild(joint.rootBone_, true);
        Node* segmentJointNode = segmentRootNode ? segmentRootNode->GetChild(joint.jointBone_) : nullptr;
        Node* segmentTargetNode = segmentJointNode ? segmentJointNode->GetChild(joint.targetBone_) : nullptr;
        Bone* thighBone = skeleton.GetBone(segmentRootNode->GetName());
        Bone* calfBone = skeleton.GetBone(segmentJointNode->GetName());
        Bone* heelBone = skeleton.GetBone(segmentTargetNode->GetName());
        if (!segmentRootNode || !segmentJointNode || !segmentTargetNode || !thighBone || !calfBone || !heelBone)
        {
            URHO3D_LOGERRORF("Failed to load 2-segment '%s' of character skeleton: root='%s', joint='%s', target='%s'",
                name.CString(), joint.rootBone_.CString(), joint.jointBone_.CString(), joint.targetBone_.CString());
            return false;
        }

        // Get initial pose
        CharacterAnimationSegment2Track track;
        track.initialDirection_ = segmentTargetNode->GetWorldPosition() - segmentRootNode->GetWorldPosition();

        // Get sample times
        const PODVector<float> sampleTimes = MergeAnimationTrackTimes(
        {
            animation.GetTrack(joint.rootBone_),
            animation.GetTrack(joint.jointBone_),
            animation.GetTrack(joint.targetBone_)
        });

        // Play animation and convert pose
        const unsigned numKeyFrames = sampleTimes.Size();
        track.keyFrames_.Resize(numKeyFrames);
        for (unsigned i = 0; i < numKeyFrames; ++i)
        {
            // Play animation
            animationState->SetTime(sampleTimes[i]);
            animationState->Apply();
            node.MarkDirty();

            // Receive animation data
            const Vector3 thighPosition = segmentRootNode->GetWorldPosition();
            const Vector3 calfPosition = segmentJointNode->GetWorldPosition();
            const Vector3 heelPosition = segmentTargetNode->GetWorldPosition();
            const Quaternion thighRotation = segmentRootNode->GetRotation();
            const Quaternion calfRotation = segmentJointNode->GetRotation();

            // Get knee direction
            const Vector3 direction = (heelPosition - thighPosition).Normalized();
            const Vector3 jointProjection = direction * (calfPosition - thighPosition).ProjectOntoAxis(direction) + thighPosition;
            const Vector3 jointDirection = Quaternion(direction, track.initialDirection_) * (calfPosition - jointProjection);

            // Revert to bind pose
            segmentRootNode->SetTransform(thighBone->initialPosition_, thighBone->initialRotation_, thighBone->initialScale_);
            segmentJointNode->SetTransform(calfBone->initialPosition_, calfBone->initialRotation_, calfBone->initialScale_);

            // Try to resolve foot shape and generate fix angles
            MatchChildPosition(*segmentRootNode, *segmentJointNode, calfPosition);
            const Quaternion thighRotationFix = segmentRootNode->GetRotation().Inverse() * thighRotation;
            segmentRootNode->SetRotation(segmentRootNode->GetRotation() * thighRotationFix);

            MatchChildPosition(*segmentJointNode, *segmentTargetNode, heelPosition);
            const Quaternion calfRotationFix = segmentJointNode->GetRotation().Inverse() * calfRotation;
            segmentJointNode->SetRotation(segmentJointNode->GetRotation() * calfRotationFix);

            // Make new frame
            track.keyFrames_[i].time_ = sampleTimes[i];
            track.keyFrames_[i].heelPosition_ = heelPosition;
            track.keyFrames_[i].kneeDirection_ = jointDirection.LengthSquared() > M_EPSILON ? jointDirection.Normalized() : Vector3::FORWARD;
            track.keyFrames_[i].thighRotationFix_ = thighRotationFix;
            track.keyFrames_[i].calfRotationFix_ = calfRotationFix;
            track.keyFrames_[i].heelRotationLocal_ = segmentTargetNode->GetRotation();
            track.keyFrames_[i].heelRotationWorld_ = segmentTargetNode->GetWorldRotation();
        }
        segments2.Populate(name, track);
    }
    segments2_.Insert(segments2);
    return true;
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

    const float referenceVelocity = 1.8f;

    AnimatedModel* model = node_->GetComponent<AnimatedModel>();
    AnimationController* controller = node_->GetComponent<AnimationController>();

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SharedPtr<Animation> animForward(cache->GetResource<Animation>("Swat_WalkFwd.ani"));
    SharedPtr<Animation> animBackward(cache->GetResource<Animation>("Swat_WalkBwd.ani"));
    SharedPtr<Animation> animLeft(cache->GetResource<Animation>("Swat_WalkLeft.ani"));
    SharedPtr<Animation> animRight(cache->GetResource<Animation>("Swat_WalkRight.ani"));
    SharedPtr<Animation> animIdle(cache->GetResource<Animation>("Swat_WalkZero.ani"));
    FootAnimationTrack trackForward, trackBackward, trackLeft, trackRight, trackIdle;
    const float threshold = 0.01f;
    CreateFootAnimationTrack(trackForward, model->GetModel(), animForward, footBoneName_, Vector3::BACK * referenceVelocity, threshold);
    CreateFootAnimationTrack(trackBackward, model->GetModel(), animBackward, footBoneName_, Vector3::FORWARD * referenceVelocity, threshold);
    CreateFootAnimationTrack(trackLeft, model->GetModel(), animLeft, footBoneName_, Vector3::RIGHT * referenceVelocity, threshold);
    CreateFootAnimationTrack(trackRight, model->GetModel(), animRight, footBoneName_, Vector3::LEFT * referenceVelocity, threshold);
    CreateFootAnimationTrack(trackIdle, model->GetModel(), animIdle, footBoneName_, Vector3::ZERO, threshold);
    trackIdle.staticRanges_.Clear();

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
    const FootAnimationKeyFrame frameForward = trackForward.SampleFrame(controller->GetTime(animForward->GetName()), frameIndex);
    const FootAnimationKeyFrame frameBackward = trackBackward.SampleFrame(controller->GetTime(animBackward->GetName()), frameIndex);
    const FootAnimationKeyFrame frameLeft = trackLeft.SampleFrame(controller->GetTime(animLeft->GetName()), frameIndex);
    const FootAnimationKeyFrame frameRight = trackRight.SampleFrame(controller->GetTime(animRight->GetName()), frameIndex);
    const FootAnimationKeyFrame frameIdle = trackIdle.SampleFrame(controller->GetTime(animIdle->GetName()), frameIndex);
    FootAnimationKeyFrame frame;
    frame.time_ = frameForward.time_;
    float weight = 0.0f;
    float maxWeight = 0.0f;
    bool isFootstep = false;
    float movementRange = 0;
    Vector3 initialDirection;
    auto makeMeHappy = [&](const FootAnimationTrack& track, const FootAnimationKeyFrame& rhs, const String& anim)
    {
        float factor = controller->GetWeight(anim);
        float time = controller->GetTime(anim);
        frame.heelPosition_ += rhs.heelPosition_ * factor;
        frame.kneeDirection_ += rhs.kneeDirection_ * factor;
        frame.thighRotationFix_ = MixQuaternion(frame.thighRotationFix_, rhs.thighRotationFix_, factor, weight);
        frame.calfRotationFix_ = MixQuaternion(frame.calfRotationFix_, rhs.calfRotationFix_, factor, weight);
        frame.heelRotationLocal_ = MixQuaternion(frame.heelRotationLocal_, rhs.heelRotationLocal_, factor, weight);
        frame.heelRotationWorld_ = MixQuaternion(frame.heelRotationWorld_, rhs.heelRotationWorld_, factor, weight);
        initialDirection += track.initialDirection_ * factor;
        weight += factor;
        maxWeight = Max(maxWeight, factor);
        if (maxWeight == factor && factor >= 0.5)
        {
            //isFootstep = track.IsStatic(time);
            movementRange = track.GetMomementRange(time);
        }
    };
    makeMeHappy(trackForward, frameForward, animForward->GetName());
    makeMeHappy(trackBackward, frameBackward, animBackward->GetName());
    makeMeHappy(trackLeft, frameLeft, animLeft->GetName());
    makeMeHappy(trackRight, frameRight, animRight->GetName());
    makeMeHappy(trackIdle, frameIdle, animIdle->GetName());

    // Resolve foot shape
    Vector3 newHeelPosition = node_->GetWorldTransform() * (Quaternion(Vector3::UP, groundNormal_) * frame.heelPosition_ + groundOffset_);
    const Vector3 jointDirection = Quaternion(initialDirection, newHeelPosition - thighNode->GetWorldPosition()) * frame.kneeDirection_;
    const Vector3 newCalfPosition = ResolveKneePosition(thighNode->GetWorldPosition(), newHeelPosition, jointDirection,
        thighLength, calfLength);

    // Resolve footsteps
    if (isFootstep)
    {
        if (movementRange_ > 0)
            newHeelPosition -= fadeDelta_ * (fadeRemaining_ / movementRange_);
        expectedPosition_ = newHeelPosition;
        if (!wasFootstep_)
            footstepPosition_ = newHeelPosition;
        else
            newHeelPosition = footstepPosition_;
        fadeRemaining_ = 0;
        movementRange_ = 0;
        fadeDelta_ = Vector3::ZERO;
    }
    else if (wasFootstep_)
    {
        movementRange_ = movementRange;
        fadeRemaining_ = movementRange;
        fadeDelta_ = expectedPosition_ - footstepPosition_;
    }

    if (movementRange_ > 0 && !isFootstep)
        newHeelPosition -= fadeDelta_ * (fadeRemaining_ / movementRange_);
    wasFootstep_ = isFootstep;

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
    fadeRemaining_ = Max(0.0f, fadeRemaining_ - timeStep);
}

void FootAnimation::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        if (wasFootstep_)
        {
            debug->AddSphere(Sphere(footstepPosition_, 0.2f), Color::RED, depthTest);
        }
    }
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
