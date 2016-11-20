#include "FlexEnginePlayer.h"

// #TODO Fix me
#undef TRANSPARENT

#include <FlexEngine/AngelScript/ScriptAPI.h>
#include <FlexEngine/Animation/FootAnimation.h>
#include <FlexEngine/Factory/ProceduralComponent.h>
#include <FlexEngine/Factory/ScriptedResource.h>
#include <FlexEngine/Factory/TreeHost.h>
#include <FlexEngine/Graphics/Grass.h>
#include <FlexEngine/Graphics/StaticModelEx.h>
#include <FlexEngine/Graphics/Wind.h>
#include <FlexEngine/Scene/DynamicComponent.h>

#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(FlexEnginePlayer);

FlexEnginePlayer::FlexEnginePlayer(Context* context) :
    Urho3DPlayer(context)
{
}

void FlexEnginePlayer::Start()
{
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();
    GetSubsystem<Renderer>()->SetMinInstances(1);
    GetSubsystem<Renderer>()->SetNumExtraInstancingBufferElements(1);

    DynamicComponent::RegisterObject(context_);
    ProceduralSystem::RegisterObject(context_);
    ProceduralComponent::RegisterObject(context_);
    ProceduralComponentAgent::RegisterObject(context_);
    ScriptedResource::RegisterObject(context_);

    TreeHost::RegisterObject(context_);
    TreeElement::RegisterObject(context_);
    BranchGroup::RegisterObject(context_);
    LeafGroup::RegisterObject(context_);
    TreeLevelOfDetail::RegisterObject(context_);
    TreeProxy::RegisterObject(context_);

    FootAnimation::RegisterObject(context_);

    StaticModelEx::RegisterObject(context_);
    Grass::RegisterObject(context_);
    GrassPatch::RegisterObject(context_);
    WindSystem::RegisterObject(context_);
    WindZone::RegisterObject(context_);

    Urho3DPlayer::Start();

    Script* scriptSubsystem = GetSubsystem<Script>();
    asIScriptEngine* scriptEngine = scriptSubsystem->GetScriptEngine();
    RegisterAPI(scriptEngine);
}
