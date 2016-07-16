#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

class FileSystem;
class ResourceCache;

}

namespace FlexEngine
{

/// Get last resource directory from Resource Cache.
String GetOutputResourceCacheDir(ResourceCache& resourceCache);

/// Get file path.
String GetFilePath(const String& fileName);

/// Create all non-existing directories from file name.
void CreateDirectoriesToFile(FileSystem& fileSystem, const String& fileName);

/// Create all non-existing directories from file name.
void CreateDirectoriesToFile(ResourceCache& resourceCache, const String& fileName);

}
