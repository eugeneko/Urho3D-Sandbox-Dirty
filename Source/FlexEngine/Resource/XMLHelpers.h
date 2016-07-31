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

/// Get XML value from element or default if element is empty.
template <class T>
T GetValue(const XMLElement& elem, const T& defaultValue)
{
    return elem ? To<T>(elem.GetValue()) : defaultValue;
}

/// Load XML attribute value into variable if element is not empty.
template <class T>
void LoadAttribute(const XMLElement& elem, const String& attributeName, T& variable)
{
    const String& value = elem.GetAttribute(attributeName);
    if (!value.Empty())
    {
        variable = To<T>(value);
    }
}

/// Get XML attribute value from element or default if attribute does not exist.
template <class T>
T GetAttribute(const XMLElement& elem, const String& attributeName, const T& defaultValue)
{
    T result = defaultValue;
    LoadAttribute(elem, attributeName, result);
    return result;
}

}
