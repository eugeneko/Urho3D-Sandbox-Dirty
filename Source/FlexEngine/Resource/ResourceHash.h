#pragma once

#include <FlexEngine/Common.h>

namespace Urho3D
{

class Resource;

}

namespace FlexEngine
{

class Hash;

/// Number of geometries of default model.
static const unsigned NUM_DEFAULT_MODEL_GEOMETRIES = 32;

/// Compute hash of resource.
Hash HashResource(Resource* resource);

/// Initialize stub resource.
void InitializeStubResource(Resource* resource);

}
