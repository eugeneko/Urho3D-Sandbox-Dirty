#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{

String GetOutputResourceCacheDir(ResourceCache& resourceCache)
{
    const StringVector& dirs = resourceCache.GetResourceDirs();
    return dirs.Empty() ? String::EMPTY : dirs.Back();
}

String GetFilePath(const String& fileName)
{
    const unsigned to = fileName.FindLast('/');
    return to != String::NPOS ? fileName.Substring(0, to) : String::EMPTY;
}

void CreateDirectoriesToFile(FileSystem& fileSystem, const String& fileName)
{
    const String filePath = GetFilePath(fileName);
    if (!filePath.Empty())
    {
        fileSystem.CreateDir(filePath);
    }
}

void CreateDirectoriesToFile(ResourceCache& resourceCache, const String& fileName)
{
    FileSystem* fileSystem = resourceCache.GetSubsystem<FileSystem>();
    if (fileSystem)
    {
        CreateDirectoriesToFile(*fileSystem, fileName);
    }
}

}
