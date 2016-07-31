#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace FlexEngine
{

class MathFunction;

/// Shared pointer on math function.
using MathFunctionSPtr = SharedPtr<MathFunction>;

/// Revert linear interpolation (scalars only).
template <class T>
T UnLerp(const T& first, const T& second, const T& value)
{
    return (value - first) / (second - first);
}

/// Revert linear interpolation (scalars only) and clamp result to [0, 1] range.
template <class T>
T UnLerpClamped(const T& first, const T& second, const T& value)
{
    return Clamp(UnLerp(first, second, value), T(0), T(1));
}

/// Construct orthogonal vector for passed one. XOY plane is preferred.
inline Vector3 ConstructOrthogonalVector(const Vector3& vec)
{
    return Abs(vec.y_) < 1.0f - M_LARGE_EPSILON
        ? Vector3(-vec.z_,   0.0f, vec.x_).Normalized()
        : Vector3(-vec.y_, vec.x_,   0.0f).Normalized();
}

/// Dot product of two vectors.
template <class T>
auto DotProduct(const T& left, const T& right) -> decltype(left.x_)
{
    return left.DotProduct(right);
}

/// Cross product of two 3D vectors.
inline Vector3 CrossProduct(const Vector3& left, const Vector3& right)
{
    return left.CrossProduct(right);
}

/// Per-component max for 2D vector.
inline Vector2 VectorMax(const Vector2& left, const Vector2& right)
{
    return Vector2(Max(left.x_, right.x_), Max(left.y_, right.y_));
}

/// Per-component max for 3D vector.
inline Vector3 VectorMax(const Vector3& left, const Vector3& right)
{
    return Vector3(Max(left.x_, right.x_), Max(left.y_, right.y_), Max(left.z_, right.z_));
}

/// Per-component min for 2D vector.
inline Vector2 VectorMin(const Vector2& left, const Vector2& right)
{
    return Vector2(Min(left.x_, right.x_), Min(left.y_, right.y_));
}

/// Per-component min for 3D vector.
inline Vector3 VectorMin(const Vector3& left, const Vector3& right)
{
    return Vector3(Min(left.x_, right.x_), Min(left.y_, right.y_), Min(left.z_, right.z_));
}

/// Project vector onto axis
template <class T>
auto ProjectOntoAxis(const T& axis, const T& vector) -> decltype(axis.x_)
{
    return DotProduct(vector, axis) / axis.Length();
}

/// Return X in power Y.
template <class T> T Pow(T x, T y) { return pow(x, y); }

/// Per-component power for 2D vector.
inline Vector2 Pow(const Vector2& left, const Vector2& right)
{
    return Vector2(Pow(left.x_, right.x_), Pow(left.y_, right.y_));
}

/// Per-component power for 3D vector.
inline Vector3 Pow(const Vector3& left, const Vector3& right)
{
    return Vector3(Pow(left.x_, right.x_), Pow(left.y_, right.y_), Pow(left.z_, right.z_));
}

/// Return fractional part of passed value.
template <class T> T Fract(T value) { T intpart; return modf(value, &intpart); }

/// Round value down.
template <class T> T Floor(T x) { return floor(x); }

/// Round value to nearest integer.
template <class T> T Round(T x) { return floor(x + 0.5f); }

/// Round value up.
template <class T> T Ceil(T x) { return ceil(x); }

/// Quad interpolation among four values.
/// @param factor1 Controls interpolation from v0 to v1 and from v2 to v3
/// @param factor2 Controls interpolation from v0 to v2 and from v1 to v3
template <class T, class U>
T QLerp(const T& v0, const T& v1, const T& v2, const T& v3, const U factor1, const U factor2)
{
    return Lerp(Lerp(v0, v1, factor1), Lerp(v2, v3, factor1), factor2);
}

}
