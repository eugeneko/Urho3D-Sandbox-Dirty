#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Scene/Serializable.h>

namespace Urho3D
{

class Material;

}

namespace FlexEngine
{

/// Helper for Serializable that provides triggers in the editor.
class EnableTriggers
{
protected:
    /// Always returns false.
    bool GetFalse() const { return false; }

};

/// Define a trigger attribute that call function on each invocation.
#define URHO3D_TRIGGER_ATTRIBUTE(name, callback) URHO3D_ACCESSOR_ATTRIBUTE(name, GetFalse, callback, bool, false, AM_EDIT)

}
