#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Core/StringUtils.h>
#include <FlexEngine/Math/MathDefs.h>

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

/// Load float range from XML attribute into variable if element is not empty.
inline void LoadFloatRange(const XMLElement& elem, const String& attributeName, FloatRange& variable)
{
    const Variant value = GetAttribute<Variant>(elem, attributeName, Variant::EMPTY);
    const VariantType type = value.GetType();
    if (type == VAR_FLOAT)
    {
        variable = value.GetFloat();
    }
    else if (type == VAR_VECTOR2)
    {
        const Vector2 vec = value.GetVector2();
        variable = FloatRange(vec.x_, vec.y_);
    }
}

/// Get float range from XML attribute or default if attribute does not exist.
inline FloatRange GetFloatRange(const XMLElement& elem, const String& attributeName, const FloatRange& defaultValue)
{
    FloatRange result = defaultValue;
    LoadFloatRange(elem, attributeName, result);
    return result;
}

}
