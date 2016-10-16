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

/// Cross product of two 3D vectors.
inline Vector3 CrossProduct(const Vector3& left, const Vector3& right)
{
    return left.CrossProduct(right);
}

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
