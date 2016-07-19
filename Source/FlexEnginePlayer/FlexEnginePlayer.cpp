#include "FlexEnginePlayer.h"

#include <FlexEngine/Component/Procedural.h>
#include <FlexEngine/Factory/ProceduralFactory.h>

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
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();

    Procedural::RegisterObject(context_);

    SharedPtr<XMLFile> xmlFile(resourceCache->GetResource<XMLFile>("Procedural.xml"));
    GenerateResourcesFromXML(*xmlFile, false, 0);

    Urho3DPlayer::Start();
}
