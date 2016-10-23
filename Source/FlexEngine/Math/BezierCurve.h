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

/// Bezier curve accessor template interface.
template <class T>
struct BezierCurveAccessor
{
    /// Number of interpolated components.
    static const unsigned NumComponents = 0;
    /// Copy all components to array.
    void GetToArray(const T& /*object*/, float /*array*/[NumComponents]) {}
    /// Copy all components from array to object.
    void SetFromArray(T& /*object*/, const float /*array*/[NumComponents]) {}
};

template <>
struct BezierCurveAccessor<float>
{
    static const unsigned NumComponents = 1;
    static void GetToArray(const float& object, float array[NumComponents]) { array[0] = object; }
    static void SetFromArray(float& object, const float array[NumComponents]) { object = array[0]; }
};

template <>
struct BezierCurveAccessor<Vector2>
{
    static const unsigned NumComponents = 2;
    static void GetToArray(const Vector2& object, float array[NumComponents]) { array[0] = object.x_; array[1] = object.y_; }
    static void SetFromArray(Vector2& object, const float array[NumComponents]) { object = Vector2(array); }
};

template <>
struct BezierCurveAccessor<Vector3>
{
    static const unsigned NumComponents = 3;
    static void GetToArray(const Vector3& object, float array[NumComponents]) { array[0] = object.x_; array[1] = object.y_; array[2] = object.z_; }
    static void SetFromArray(Vector3& object, const float array[NumComponents]) { object = Vector3(array); }
};

template <>
struct BezierCurveAccessor<Matrix3>
{
    static const unsigned NumComponents = 9;
    static void GetToArray(const Matrix3& object, float array[NumComponents]) { memcpy(array, object.Data(), NumComponents * sizeof(float)); }
    static void SetFromArray(Matrix3& object, const float array[NumComponents]) { object = Matrix3(array); }
};

/// Bezier curve.
template <class T>
class BezierCurve
{
public:
    /// Number of components
    static const unsigned NumComponents = BezierCurveAccessor<T>::NumComponents;
    static_assert(NumComponents > 0, "Invalid Bezier curve element type");
    /// Construct empty.
    BezierCurve() {}
    /// Add point.
    void AddPoint(const T& point)
    {
        dirty_ = true;
        float array[NumComponents];
        BezierCurveAccessor<T>::GetToArray(point, array);
        for (unsigned i = 0; i < NumComponents; ++i)
            points_[i].Push(array[i]);
    }
    /// Clear all points.
    void Clear()
    {
        dirty_ = true;
        points_.Clear();
        for (unsigned i = 0; i < NumComponents; ++i)
            curves_[i].Clear();
    }
    /// Get number of points.
    unsigned GetNumPoints() const { return points_[0].Size(); }
    /// Get point.
    T GetPoint(unsigned index) const
    {
        float array[NumComponents];
        for (unsigned i = 0; i < NumComponents; ++i)
            array[i] = points_[i][index];
        return CreatePoint(array);
    }
    /// Sample point on curve by location from [0, 1]
    T SamplePoint(float t) const
    {
        Build();
        float array[NumComponents];
        for (unsigned i = 0; i < NumComponents; ++i)
            array[i] = SampleBezierCurve(curves_[i], t);
        return CreatePoint(array);
    }
    /// Sample point on curve by location from [0, N-1], where N is a number of points.
    T SamplePointAbs(float t) const
    {
        Build();
        float array[NumComponents];
        for (unsigned i = 0; i < NumComponents; ++i)
            array[i] = SampleBezierCurveAbs(curves_[i], t);
        return CreatePoint(array);
    }
    /// Sample point derivative on curve by location from [0, 1]
    T SampleDerivative(float t) const
    {
        Build();
        float array[NumComponents];
        for (unsigned i = 0; i < NumComponents; ++i)
            array[i] = SampleBezierCurveDerivative(curves_[i], t);
        return CreatePoint(array);
    }
    /// Sample point derivative on curve by location from [0, N-1], where N is a number of points.
    T SampleDerivativeAbs(float t) const
    {
        Build();
        float array[NumComponents];
        for (unsigned i = 0; i < NumComponents; ++i)
            array[i] = SampleBezierCurveDerivativeAbs(curves_[i], t);
        return CreatePoint(array);
    }

private:
    /// Build curve if dirty.
    void Build() const
    {
        if (dirty_)
        {
            dirty_ = false;
            for (unsigned i = 0; i < NumComponents; ++i)
                curves_[i] = CreateBezierCurve(points_[i]);
        }
    }
    /// Create point from array.
    static T CreatePoint(float array[NumComponents])
    {
        T point;
        BezierCurveAccessor<T>::SetFromArray(point, array);
        return point;
    }

private:
    /// Array of curve points.
    PODVector<float> points_[NumComponents];
    /// Is curve dirty?
    mutable bool dirty_ = false;
    /// Curves for each component.
    mutable BezierCurve1D curves_[NumComponents];
};

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

