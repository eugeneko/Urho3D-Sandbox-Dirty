#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Container/Str.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace FlexEngine
{

void RemoveAllResourceCacheDirs(ResourceCache& resourceCache)
{
    const StringVector& resourceDirs = resourceCache.GetResourceDirs();
    for (const String& resourceDir : resourceDirs)
    {
        resourceCache.RemoveResourceDir(resourceDir);
    }
}

void RemoveAllPackageFiles(ResourceCache& resourceCache)
{
    const Vector<SharedPtr<PackageFile>>& packageFiles = resourceCache.GetPackageFiles();
    for (const SharedPtr<PackageFile>& packageFile : packageFiles)
    {
        resourceCache.RemovePackageFile(packageFile);
    }
}

String ScanDirectoriesUpward(FileSystem& fileSystem, const String& directory, const String& file, const int maxDepth /*= -1*/)
{
    // Input folder shouldn't be empty
    String result = directory.Trimmed();
    if (result.Empty())
    {
        return String::EMPTY;
    }

    // Remove ending slash
    static const char SLASH = '/';
    if (result.Back() == SLASH)
    {
        result.Erase(result.End() - 1);
    }

    // Scan folders
    int currentDepth = 0;
    while (!fileSystem.FileExists(result + SLASH + file))
    {
        // Return invalid if max depth is reached
        if (maxDepth >= 0 && currentDepth > maxDepth)
        {
            return String::EMPTY;
        }

        // Return invalid if no directories upward
        const unsigned idx = result.FindLast(SLASH);
        if (idx == String::NPOS)
        {
            return String::EMPTY;
        }

        // Erase last entry in path and try again
        result.Erase(result.Begin() + idx, result.End());
    }
    
    return result + SLASH;
}

void AddResourceCacheElements(ResourceCache& resourceCache, const String& rootFolder, const String& elements)
{
    const StringVector elementsNames = elements.Split(';');
    for (const String& element : elementsNames)
    {
        resourceCache.AddResourceDir(rootFolder + element);
    }
}

}

