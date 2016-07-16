#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Core/StringUtils.h>

#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

/// Load XML element value into variable if element is not empty.
template <class T>
void LoadValue(const XMLElement& elem, T& variable)
{
    if (elem)
    {
        variable = To<T>(elem.GetValue());
    }
}

}
