#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Container/Pair.h>

namespace Urho3D
{

}

namespace FlexEngine
{

// Import all Urho3D entities into FlexEngine namespace.
using namespace Urho3D;

/// Category for all FlexEngine components.
extern const char* FLEXENGINE_CATEGORY;

/// @name Enable range-based for loop for iterator ranges
/// @{
template <class T> T begin(Urho3D::Pair<T, T>& range) { return range.first_; }
template <class T> T end(Urho3D::Pair<T, T>& range) { return range.second_; }
template <class T> T begin(const Urho3D::Pair<T, T>& range) { return range.first_; }
template <class T> T end(const Urho3D::Pair<T, T>& range) { return range.second_; }
/// @}

}
