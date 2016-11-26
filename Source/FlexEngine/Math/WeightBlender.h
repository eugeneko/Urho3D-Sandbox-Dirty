#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Math/StringHash.h>

namespace Urho3D
{

class BoundingBox;

}

namespace FlexEngine
{

class WeightBlender
{
public:
    /// Set weight.
    void SetWeight(StringHash key, float weight, float fadeTime = 0);
    /// Update.
    void Update(float timeStep, bool removeZeroWeights = false);
    /// Get weight.
    float GetWeight(StringHash key) const;
    /// Get normalized weight.
    float GetNormalizedWeight(StringHash key) const;

private:
    /// State description.
    struct State
    {
        /// Current weight.
        float weight_ = 0.0f;
        /// Target weight that is going to be reached after some time.
        float targetWeight_ = 0.0f;
        /// Speed of weight changing.
        float speed_ = 0.0f;
    };

    /// Map of states.
    using StateMap = HashMap<StringHash, State>;
    /// States.
    StateMap states_;
    /// Summary weight of states.
    float sumWeight_ = 0.0f;
};

}

