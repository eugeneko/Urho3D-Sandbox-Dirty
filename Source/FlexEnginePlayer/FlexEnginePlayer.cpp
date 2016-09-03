#include "FlexEnginePlayer.h"

#include <FlexEngine/AngelScript/ScriptAPI.h>
#include <FlexEngine/Component/Procedural.h>
#include <FlexEngine/Factory/ProceduralComponent.h>
#include <FlexEngine/Factory/ProceduralFactory.h>
#include <FlexEngine/Factory/TextureHost.h>
#include <FlexEngine/Factory/TreeHost.h>
#include <FlexEngine/Scene/DynamicComponent.h>

#include <Urho3D/DebugNew.h>
#include <Urho3D/AngelScript/Script.h>
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

    DynamicComponent::RegisterObject(context_);
    ProceduralComponent::RegisterObject(context_);
    ProceduralComponentAgent::RegisterObject(context_);
    TreeHost::RegisterObject(context_);
    TreeElement::RegisterObject(context_);
    BranchGroup::RegisterObject(context_);
    LeafGroup::RegisterObject(context_);
    TextureHost::RegisterObject(context_);
    TextureElement::RegisterObject(context_);
    InputTexture::RegisterObject(context_);
    RenderedModelTexture::RegisterObject(context_);
    PerlinNoiseTexture::RegisterObject(context_);
    FillGapFilter::RegisterObject(context_);

    Urho3DPlayer::Start();

    Script* scriptSubsystem = GetSubsystem<Script>();
    asIScriptEngine* scriptEngine = scriptSubsystem->GetScriptEngine();
    RegisterAPI(scriptEngine);

    SharedPtr<XMLFile> xmlFile(resourceCache->GetResource<XMLFile>("Procedural.xml"));
    GenerateResourcesFromXML(*xmlFile, false, 0);
}
