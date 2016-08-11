#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/MathDefs.h>

namespace FlexEngine
{

/// Coefficient array that represents 1D Bezier curve.
using BezierCurve1D = PODVector<Vector4>;

/// Compute coefficients of Bezier curve by knots.
BezierCurve1D CreateBezierCurve(const PODVector<float>& values);

/// Sample point on Bezier curve and return value. Location must be in range [0, numKnots].
float SampleBezierCurveAbs(const BezierCurve1D& curve, float location);

/// Sample derivative of point on Bezier curve and return value. Location must be in range [0, numKnots].
float SampleBezierCurveDerivativeAbs(const BezierCurve1D& curve, float location);

/// Sample point on Bezier curve and return value. Location must be in range [0, 1].
float SampleBezierCurve(const BezierCurve1D& curve, float location);

/// Sample derivative of point on Bezier curve and return value. Location must be in range [0, 1].
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

/// Compute coefficients of 2D Bezier curve by knots.
BezierCurve2D CreateBezierCurve(const PODVector<Vector2>& values);

/// Sample point on 2D Bezier curve and return value. Location must be in range [0, numKnots].
Vector2 SampleBezierCurveAbs(const BezierCurve2D& curve, float location);

/// Sample derivative of point on 2D Bezier curve and return value. Location must be in range [0, numKnots].
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

/// Compute coefficients of 3D Bezier curve by knots.
BezierCurve3D CreateBezierCurve(const PODVector<Vector3>& values);

/// Sample point on 3D Bezier curve and return value. Location must be in range [0, numKnots].
Vector3 SampleBezierCurveAbs(const BezierCurve3D& curve, float location);

/// Sample derivative of point on 3D Bezier curve and return value. Location must be in range [0, numKnots].
Vector3 SampleBezierCurveDerivativeAbs(const BezierCurve3D& curve, float location);

/// Sample point on 3D Bezier curve and return value. Location must be in range [0, 1].
Vector3 SampleBezierCurve(const BezierCurve3D& curve, float location);

/// Sample derivative of point on Bezier 3D curve and return value. Location must be in range [0, 1].
Vector3 SampleBezierCurveDerivative(const BezierCurve3D& curve, float location);

}

