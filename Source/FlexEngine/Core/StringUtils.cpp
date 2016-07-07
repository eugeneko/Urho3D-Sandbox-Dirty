#include <FlexEngine/Core/StringUtils.h>

namespace FlexEngine
{

template <>
String To<String>(const String& str)
{
    return str;
}

template <>
bool To<bool>(const String& str)
{
    return ToBool(str);
}

template <>
float To<float>(const String& str)
{
    return ToFloat(str);
}

template <>
double To<double>(const String& str)
{
    return ToDouble(str);
}

template <>
int To<int>(const String& str)
{
    return ToInt(str);
}

template <>
unsigned To<unsigned>(const String& str)
{
    return ToUInt(str);
}

template <>
Color To<Color>(const String& str)
{
    return ToColor(str);
}

template <>
IntRect To<IntRect>(const String& str)
{
    return ToIntRect(str);
}

template <>
IntVector2 To<IntVector2>(const String& str)
{
    return ToIntVector2(str);
}

template <>
Quaternion To<Quaternion>(const String& str)
{
    return ToQuaternion(str);
}

template <>
Rect To<Rect>(const String& str)
{
    return ToRect(str);
}

template <>
Vector2 To<Vector2>(const String& str)
{
    return ToVector2(str);
}

template <>
Vector3 To<Vector3>(const String& str)
{
    return ToVector3(str);
}

template <>
Vector4 To<Vector4>(const String& str)
{
    return ToVector4(str);
}

template <>
Variant To<Variant>(const String& str)
{
    return ToVectorVariant(str);
}

template <>
Matrix3 To<Matrix3>(const String& str)
{
    return ToMatrix3(str);
}

template <>
Matrix3x4 To<Matrix3x4>(const String& str)
{
    return ToMatrix3x4(str);
}

template <>
Matrix4 To<Matrix4>(const String& str)
{
    return ToMatrix4(str);
}

}
