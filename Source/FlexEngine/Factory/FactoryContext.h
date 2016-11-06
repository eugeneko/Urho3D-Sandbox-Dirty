#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

namespace FlexEngine
{

/// Context of procedural factories.
struct FactoryContext
{
    /// Output directory.
    String outputDirectory_;
    /// Current directory that is used for substituting '@' placeholder in procedural resources names.
    String currentDirectory_;
    /// Seed of random generators.
    unsigned seed_;
    /// Shows whether force re-generation of existing resources is performed.
    bool forceGeneration_;

    /// Replace all '@' placeholders with current directory.
    String ExpandName(const String& fileName) const
    {
        return fileName.Replaced("@", currentDirectory_).Trimmed();
    }
};

}

}
