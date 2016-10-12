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

/// Per-component lerp for 2D vector.
inline Vector2 VectorLerp(const Vector2& first, const Vector2& second, const Vector2& value)
{
    return Vector2(Lerp(first.x_, second.x_, value.x_), Lerp(first.y_, second.y_, value.y_));
}

/// Per-component lerp for 3D vector.
inline Vector3 VectorLerp(const Vector3& first, const Vector3& second, const Vector3& value)
{
    return Vector3(Lerp(first.x_, second.x_, value.x_), Lerp(first.y_, second.y_, value.y_), Lerp(first.z_, second.z_, value.z_));
}

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

/// Generalization of Hermite interpolation with variable degree of smoothing.
inline float SmoothStepEx(float t, float k)
{
    float q = 1 - t;
    return Clamp(q*q*t*(1 - k) + q*t*t*(2 + k) + t*t*t, 0.0f, 1.0f);
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

/// Angle between two vectors.
template <class T>
auto AngleBetween(const T& left, const T& right) -> decltype(left.x_)
{
    return left.Angle(right);
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
inline Vector2 VectorPow(const Vector2& left, const Vector2& right)
{
    return Vector2(Pow(left.x_, right.x_), Pow(left.y_, right.y_));
}

/// Per-component power for 3D vector.
inline Vector3 VectorPow(const Vector3& left, const Vector3& right)
{
    return Vector3(Pow(left.x_, right.x_), Pow(left.y_, right.y_), Pow(left.z_, right.z_));
}

/// Return square root of X.
template <class T> T Sqrt(T x) { return sqrt(x); }

/// Per-component square root of 2D vector.
inline Vector2 VectorSqrt(const Vector2& vec)
{
    return Vector2(Sqrt(vec.x_), Sqrt(vec.y_));
}

/// Per-component square root of 3D vector.
inline Vector3 VectorSqrt(const Vector3& vec)
{
    return Vector3(Sqrt(vec.x_), Sqrt(vec.y_), Sqrt(vec.z_));
}

/// Return fractional part of passed value.
template <class T> T Fract(T value) { T intpart; return modf(value, &intpart); }

/// Round value down.
template <class T> T Floor(T x) { return floor(x); }

/// Per-component floor for 2D vector.
inline Vector2 VectorFloor(const Vector2& vec) { return Vector2(Floor(vec.x_), Floor(vec.y_)); }

/// Per-component floor for 3D vector.
inline Vector3 VectorFloor(const Vector3& vec) { return Vector3(Floor(vec.x_), Floor(vec.y_), Floor(vec.z_)); }

/// Round value to nearest integer.
template <class T> T Round(T x) { return floor(x + 0.5f); }

/// Per-component round for 2D vector.
inline Vector2 VectorRound(const Vector2& vec) { return Vector2(Round(vec.x_), Round(vec.y_)); }

/// Per-component round for 3D vector.
inline Vector3 VectorRound(const Vector3& vec) { return Vector3(Round(vec.x_), Round(vec.y_), Round(vec.z_)); }

/// Round value up.
template <class T> T Ceil(T x) { return ceil(x); }

/// Per-component ceil for 2D vector.
inline Vector2 VectorCeil(const Vector2& vec) { return Vector2(Ceil(vec.x_), Ceil(vec.y_)); }

/// Per-component ceil for 3D vector.
inline Vector3 VectorCeil(const Vector3& vec) { return Vector3(Ceil(vec.x_), Ceil(vec.y_), Ceil(vec.z_)); }

/// Return hash of 2D vector.
inline unsigned MakeHash(const Vector2& v)
{
    const unsigned& x = *reinterpret_cast<const unsigned*>(&v.x_);
    const unsigned& y = *reinterpret_cast<const unsigned*>(&v.y_);
    return x ^ (y * 3);
}

/// Return hash of 3D vector.
inline unsigned MakeHash(const Vector3& v)
{
    const unsigned& x = *reinterpret_cast<const unsigned*>(&v.x_);
    const unsigned& y = *reinterpret_cast<const unsigned*>(&v.y_);
    const unsigned& z = *reinterpret_cast<const unsigned*>(&v.z_);
    return x ^ (y * 3) ^ (z * 11);
}

/// Return hash of 4D vector.
inline unsigned MakeHash(const Vector4& v)
{
    const unsigned& x = *reinterpret_cast<const unsigned*>(&v.x_);
    const unsigned& y = *reinterpret_cast<const unsigned*>(&v.y_);
    const unsigned& z = *reinterpret_cast<const unsigned*>(&v.z_);
    const unsigned& w = *reinterpret_cast<const unsigned*>(&v.w_);
    return x ^ (y * 3) ^ (z * 11) * (w * 29);
}

/// Get X axis of basis from rotation matrix.
inline Vector3 GetBasisX(const Matrix3& mat)
{
    return Vector3(mat.m00_, mat.m10_, mat.m20_);
}

/// Get Y axis of basis from rotation matrix.
inline Vector3 GetBasisY(const Matrix3& mat)
{
    return Vector3(mat.m01_, mat.m11_, mat.m21_);
}

/// Get Z axis of basis from rotation matrix.
inline Vector3 GetBasisZ(const Matrix3& mat)
{
    return Vector3(mat.m02_, mat.m12_, mat.m22_);
}

/// Quad interpolation among four values.
/// @param factor1 Controls interpolation from v0 to v1 and from v2 to v3
/// @param factor2 Controls interpolation from v0 to v2 and from v1 to v3
template <class T, class U>
T QLerp(const T& v0, const T& v1, const T& v2, const T& v3, const U factor1, const U factor2)
{
    return Lerp(Lerp(v0, v1, factor1), Lerp(v2, v3, factor1), factor2);
}

/// Pseudo-random generator.
inline float PseudoRandom(const Vector2& vec)
{
    return Fract(Sin(vec.DotProduct(Vector2(12.9898f, 78.233f)) * M_RADTODEG) * 43758.5453f);
}

/// Pseudo-random generator.
inline float PseudoRandom(float value)
{
    return PseudoRandom(Vector2(value, value));
}

/// Pseudo-random generator.
inline float PseudoRandom(const Vector3& vec)
{
    return PseudoRandom(Vector2(PseudoRandom(Vector2(vec.x_, vec.y_)), vec.z_));
}

/// Float range.
class FloatRange : public Vector2
{
public:
    /// Construct default.
    FloatRange() : Vector2() { }

    /// Construct with value.
    FloatRange(float value) : Vector2(value, value) { }

    /// Construct with values.
    FloatRange(float first, float second) : Vector2(first, second) { }

    /// Construct with values.
    explicit FloatRange(const Vector2& values) : Vector2(values) { }

    /// Get interpolated value.
    float Get(float factor) const { return Urho3D::Lerp(x_, y_, factor); }

    /// Get vector.
    const Vector2& GetVector() const { return *this; }

    /// Set vector.
    void SetVector(const Vector2& vec)
    {
        x_ = vec.x_;
        y_ = vec.y_;
    }
};

}
