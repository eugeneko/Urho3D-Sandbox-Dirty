#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

class FileSystem;
class ResourceCache;
class String;

}

namespace FlexEngine
{

/// Remove all resource cache directories of Resource Cache.
void RemoveAllResourceCacheDirs(ResourceCache& resourceCache);

/// Remove all resource packs of Resource Cache.
void RemoveAllPackageFiles(ResourceCache& resourceCache);

/// Scan directories upward hierarchy starting from specified directory to find file with specified name.
/// @return Absolute path of directory that contains file, empty line if not found.
String ScanDirectoriesUpward(FileSystem& fileSystem, const String& directory, const String& file, const int maxDepth = -1);

/// Add elements to Resource Cache
void AddResourceCacheElements(ResourceCache& resourceCache, const String& rootFolder, const String& elements);

}
