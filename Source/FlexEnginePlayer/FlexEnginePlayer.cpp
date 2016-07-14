#include "FlexEnginePlayer.h"

#include <FlexEngine/Component/Procedural.h>

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

URHO3D_DEFINE_APPLICATION_MAIN(FlexEnginePlayer);

FlexEnginePlayer::FlexEnginePlayer(Context* context) :
    Urho3DPlayer(context)
{
}

void FlexEnginePlayer::Start()
{
    Procedural::RegisterObject(context_);

    Urho3DPlayer::Start();
}
