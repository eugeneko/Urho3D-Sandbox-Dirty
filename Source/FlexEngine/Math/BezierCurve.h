#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/MathDefs.h>

namespace FlexEngine
{

/// Coefficient array that represents 1D Bezier curve.
using BezierCurve1D = PODVector<Vector4>;

/// Compute coefficients of 1D Bezier curve by knots. At least two values are required.
BezierCurve1D CreateBezierCurve(const PODVector<float>& values);

/// Sample point on 1D Bezier curve and return value. Location must be in range [0, number_of_segments].
float SampleBezierCurveAbs(const BezierCurve1D& curve, float location);

/// Sample derivative of point on 1D Bezier curve and return value. Location must be in range [0, number_of_segments].
float SampleBezierCurveDerivativeAbs(const BezierCurve1D& curve, float location);

/// Sample point on 1D Bezier curve and return value. Location must be in range [0, 1].
float SampleBezierCurve(const BezierCurve1D& curve, float location);

/// Sample derivative of point on 1D Bezier curve and return value. Location must be in range [0, 1].
float SampleBezierCurveDerivative(const BezierCurve1D& curve, float location);

/// Splice vector array and return array of specified component.
template <class T>
PODVector<float> SpliceVectorArray(const PODVector<T>& vectorArray, unsigned component)
{
    static const unsigned numComponents = sizeof(T) / sizeof(float);
    assert(component < numComponents);

    PODVector<float> result;
    for (const T& elem : vectorArray)
    {
        result.Push(elem.Data()[component]);
    }
    return result;
}

/// 2D Bezier curve.
struct BezierCurve2D
{
    BezierCurve1D xcoef_;
    BezierCurve1D ycoef_;
};

/// Compute coefficients of 2D Bezier curve by knots. At least two values are required.
BezierCurve2D CreateBezierCurve(const PODVector<Vector2>& values);

/// Sample point on 2D Bezier curve and return value. Location must be in range [0, number_of_segments].
Vector2 SampleBezierCurveAbs(const BezierCurve2D& curve, float location);

/// Sample derivative of point on 2D Bezier curve and return value. Location must be in range [0, number_of_segments].
Vector2 SampleBezierCurveDerivativeAbs(const BezierCurve2D& curve, float location);

/// Sample point on 2D Bezier curve and return value. Location must be in range [0, 1].
Vector2 SampleBezierCurve(const BezierCurve2D& curve, float location);

/// Sample derivative of point on 2D Bezier curve and return value. Location must be in range [0, 1].
Vector2 SampleBezierCurveDerivative(const BezierCurve2D& curve, float location);

/// 3D Bezier curve.
struct BezierCurve3D
{
    BezierCurve1D xcoef_;
    BezierCurve1D ycoef_;
    BezierCurve1D zcoef_;
};

/// Compute coefficients of 3D Bezier curve by knots. At least two values are required.
BezierCurve3D CreateBezierCurve(const PODVector<Vector3>& values);

/// Sample point on 3D Bezier curve and return value. Location must be in range [0, number_of_segments].
Vector3 SampleBezierCurveAbs(const BezierCurve3D& curve, float location);

/// Sample derivative of point on 3D Bezier curve and return value. Location must be in range [0, number_of_segments].
Vector3 SampleBezierCurveDerivativeAbs(const BezierCurve3D& curve, float location);

/// Sample point on 3D Bezier curve and return value. Location must be in range [0, 1].
Vector3 SampleBezierCurve(const BezierCurve3D& curve, float location);

/// Sample derivative of point on Bezier 3D curve and return value. Location must be in range [0, 1].
Vector3 SampleBezierCurveDerivative(const BezierCurve3D& curve, float location);

/// Cubic curve represents 1D function.
struct CubicCurve
{
    /// Segments of curve.
    PODVector<Vector4> segments_;
    /// Knot locations.
    PODVector<float> locations_;
};

/// Cubic curve node.
struct CubicCurvePoint
{
    /// Construct default.
    CubicCurvePoint() = default;
    /// Construct.
    CubicCurvePoint(float t, float x, float dx) : t_(t), x_(x), dxl_(dx), dxr_(dx) {}
    /// Construct.
    CubicCurvePoint(float t, float x, float dxl, float dxr) : t_(t), x_(x), dxl_(dxl), dxr_(dxr) {}
    /// Compare.
    bool operator < (const CubicCurvePoint& another) const { return t_ < another.t_; }
    /// Location.
    float t_;
    /// Value.
    float x_;
    /// Left derivative.
    float dxl_;
    /// Right derivative.
    float dxr_;
};

/// Compute coefficients of cubic curve x(t). Points are tuples of (t, x, dx-, dx+). At least two values are required.
CubicCurve CreateCubicCurve(PODVector<CubicCurvePoint> points, bool silent = false);

/// Sample cubic curve. If location is out of curve, it is clamped.
float SampleCubicCurve(const CubicCurve& curve, float location);

/// Parse text string as cubic curve. All possible formats are listed below.
///
/// @code
/// t1 x1 dx1  ... tn xn dxn
/// ex t1 x1 dx1- dx1+ ... tn xn dx1- dx1+
///   where n >= 2, ti and xi are abscissa and ordinate of i-th point
///   and dxi is its derivative (dxi- is left derivative, dxi+ is right derivative)
/// @endcode
PODVector<CubicCurvePoint> ReadCubicCurve(const String& str, bool silent = false);

/// Parse text string as cubic curve. Some predefined curves have aliases.
///
/// Alias       | value in format (x1, dx1) - (x2, dx2), t1 is 0, t2 is 1.
/// ------------|----------------------------------------------------------
/// zero        | (0, 0) - (0, 0)
/// one         | (1, 0) - (1, 0)
/// linear      | (0, 1) - (1, 1)
/// 1-linear    | (1, -1) - (0, -1)
/// cos         | (0, 0) - (1, pi/2)
/// 1-cos       | (1, 0) - (0, -pi/2)
/// sin         | (0, pi/2) - (1, 0)
/// 1-sin       | (1, -pi/2) - (0, 0)
PODVector<CubicCurvePoint> ReadCubicCurveAliased(const String& str, bool silent = false);

/// Cubic curve wrapper.
class CubicCurveWrapper
{
public:
    /// Construct default.
    CubicCurveWrapper() = default;
    /// Construct from string.
    CubicCurveWrapper(const String& str);
    /// Set curve.
    void SetCurveString(const String& str, bool silent = true);
    /// Get curve.
    const String& GetCurveString() const;
    /// Set result range.
    void SetResultRange(const Vector2& range);
    /// Get result range.
    const FloatRange& GetResultRange() const;
    /// Compute value.
    float ComputeValue(float location) const;
private:
    /// String representation of curve.
    String str_;
    /// Curve.
    CubicCurve curve_;
    /// Result is interpolated within this range.
    FloatRange range_ = FloatRange(0.0f, 1.0f);
};

}

