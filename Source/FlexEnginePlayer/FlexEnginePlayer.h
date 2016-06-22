#pragma once

#include <FlexEngine/Common.h>
#include <Urho3DPlayer/Urho3DPlayer.h>

using namespace FlexEngine;

/// FlexEnginePlayer application runs a script specified on the command line.
class FlexEnginePlayer : public Urho3DPlayer
{
    URHO3D_OBJECT(FlexEnginePlayer, Urho3DPlayer);

public:
    /// Construct.
    FlexEnginePlayer(Context* context);

    /// Setup after engine initialization. Load the script and execute its start function.
    virtual void Start();
};
