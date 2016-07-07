#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Core/StringUtils.h>

namespace FlexEngine
{

/// Convert string to object
template <class T>
T To(const String& str);

}
