#include <FlexEngine/Math/WeightBlender.h>

#include <Urho3D/Math/MathDefs.h>

namespace FlexEngine
{

void WeightBlender::SetWeight(StringHash key, float weight, float fadeTime)
{
    State& state = states_[key];
    if (fadeTime == 0.0f)
        state.weight_ = state.targetWeight_ = weight;
    else
    {
        state.targetWeight_ = weight;
        state.speed_ = Abs(state.targetWeight_ - state.weight_) / fadeTime;
    }
}

void WeightBlender::Update(float timeStep, bool removeZeroWeights /*= false*/)
{
    sumWeight_ = 0.0f;
    for (StateMap::Iterator i = states_.Begin(); i != states_.End(); ++i)
    {
        State& state = i->second_;
        const float delta = state.targetWeight_ - state.weight_;
        if (delta != 0)
            state.weight_ += Sign(delta) * Min(Abs(delta), timeStep * state.speed_);
        sumWeight_ += state.weight_;
    }

    if (removeZeroWeights)
    {
        for (StateMap::Iterator i = states_.Begin(); i != states_.End();)
        {
            State& state = i->second_;
            if (Equals(state.weight_, state.targetWeight_) && Equals(state.weight_, 0.0f))
                i = states_.Erase(i);
            else
                ++i;
        }
    }
}

float WeightBlender::GetWeight(StringHash key) const
{
    const State* state = states_[key];
    return state ? state->weight_ : 0.0f;
}

float WeightBlender::GetNormalizedWeight(StringHash key) const
{
    const float weight = GetWeight(key);
    return sumWeight_ > 0 ? weight / sumWeight_ : weight;
}

}
