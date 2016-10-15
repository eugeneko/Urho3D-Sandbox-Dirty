#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/MathDefs.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

/// Load XML element value into variable if element is not empty.
template <class T>
bool LoadValue(const XMLElement& elem, T& variable)
{
    if (elem)
    {
        variable = FromString<T>(elem.GetValue().CString());
        return true;
    }
    return false;
}

/// Get XML value from element or default if element is empty.
template <class T>
T GetValue(const XMLElement& elem, const T& defaultValue)
{
    return elem ? FromString<T>(elem.GetValue().CString()) : defaultValue;
}

/// Load XML attribute value into variable if element is not empty.
template <class T>
bool LoadAttribute(const XMLElement& elem, const String& attributeName, T& variable)
{
    const String& value = elem.GetAttribute(attributeName);
    if (!value.Empty())
    {
        variable = FromString<T>(value.CString());
        return true;
    }
    return false;
}

/// Get XML attribute value from element or default if attribute does not exist.
template <class T>
T GetAttribute(const XMLElement& elem, const String& attributeName, const T& defaultValue)
{
    T result = defaultValue;
    LoadAttribute(elem, attributeName, result);
    return result;
}

/// Load XML attribute or child node value into variable if not empty.
template <class T>
bool LoadAttributeOrChild(const XMLElement& elem, const String& name, T& variable)
{
    return LoadAttribute(elem, name, variable) || LoadValue(elem.GetChild(name), variable);
}

/// Get XML attribute or child node value from element or default if not exist.
template <class T>
T GetAttributeOrChild(const XMLElement& elem, const String& name, const T& defaultValue)
{
    T result = defaultValue;
    LoadAttributeOrChild(elem, name, result);
    return result;
}

}
