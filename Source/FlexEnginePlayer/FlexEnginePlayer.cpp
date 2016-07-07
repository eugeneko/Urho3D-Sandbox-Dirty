#include "FlexEnginePlayer.h"

#include <FlexEngine/Resource/ResourceCacheHelpers.h>

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
    FileSystem* fileSystem = GetSubsystem<FileSystem>();
    ResourceCache* resourceCache = GetSubsystem<ResourceCache>();

    RemoveAllResourceCacheDirs(*resourceCache);
    RemoveAllPackageFiles(*resourceCache);
    const String rootDir = ScanDirectoriesUpward(*fileSystem, fileSystem->GetProgramDir(), "RootFolderTag");
    fileSystem->SetCurrentDir(rootDir);
    AddResourceCacheElements(*resourceCache, rootDir, "Asset/Data;Asset/CoreData;Asset/Architect");

    Urho3DPlayer::Start();
}
