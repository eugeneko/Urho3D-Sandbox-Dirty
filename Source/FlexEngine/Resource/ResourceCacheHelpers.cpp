#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <FlexEngine/Factory/TextureFactory.h>

#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/Resource.h>
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

bool SaveResource(Resource& resource, bool reloadAfter)
{
    // Check name
    const String& resourceName = resource.GetName();
    if (resourceName.Empty())
    {
        URHO3D_LOGERROR("Resource name mustn't be empty");
        return false;
    }

    // Check cache
    ResourceCache* cache = resource.GetSubsystem<ResourceCache>();
    if (!cache)
    {
        URHO3D_LOGERROR("Resource cache must be initialized");
        return false;
    }

    // Create directories
    const String& outputFileName = GetOutputResourceCacheDir(*cache) + resourceName;
    CreateDirectoriesToFile(*cache, outputFileName);

    // Save file
    bool success = false;
    if (resource.IsInstanceOf<Image>())
    {
        // #TODO Remove this hack
        return SaveImage(cache, static_cast<Image&>(resource));
    }
    else
    {
        File file(resource.GetContext(), outputFileName, FILE_WRITE);
        if (file.IsOpen())
        {
            if (resource.Save(file))
            {
                // Reload resource
                if (reloadAfter)
                {
                    file.Close();
                    cache->ReloadResourceWithDependencies(resourceName);
                }

                return true;
            }
        }
    }
    return false;
}

}
