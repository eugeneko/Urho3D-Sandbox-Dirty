#include <FlexEngine/Math/BezierCurve.h>

#include <FlexEngine/Container/Ptr.h>

#include <Urho3D/Math/MathDefs.h>

namespace FlexEngine
{


PODVector<Vector4> CreateBezierCurve(const PODVector<float>& knots)
{
    assert(!knots.Empty());

    const unsigned n = knots.Size() - 1;
    PODVector<float> p1(n);
    PODVector<float> p2(n);

    // rhs vector
    PODVector<float> a(n);
    PODVector<float> b(n);
    PODVector<float> c(n);
    PODVector<float> r(n);

    // left most segment
    a[0] = 0;
    b[0] = 2;
    c[0] = 1;
    r[0] = knots[0] + 2 * knots[1];

    // internal segments
    for (unsigned i = 1; i < n - 1; i++)
    {
        a[i] = 1;
        b[i] = 4;
        c[i] = 1;
        r[i] = 4 * knots[i] + 2 * knots[i + 1];
    }

    // right segment
    a[n - 1] = 2;
    b[n - 1] = 7;
    c[n - 1] = 0;
    r[n - 1] = 8 * knots[n - 1] + knots[n];

    // solves Ax=b with the Thomas algorithm (from Wikipedia)
    for (unsigned i = 1; i < n; i++)
    {
        const float m = a[i] / b[i - 1];
        b[i] = b[i] - m * c[i - 1];
        r[i] = r[i] - m * r[i - 1];
    }

    p1[n - 1] = r[n - 1] / b[n - 1];
    for (int i = n - 2; i >= 0; --i)
    {
        p1[i] = (r[i] - c[i] * p1[i + 1]) / b[i];
    }

    // we have p1, now compute p2
    for (unsigned i = 0; i < n - 1; i++)
    {
        p2[i] = 2 * knots[i + 1] - p1[i + 1];
    }

    p2[n - 1] = 0.5f*(knots[n] + p1[n - 1]);

    // Merge result
    PODVector<Vector4> result;
    for (unsigned i = 0; i < n; ++i)
    {
        result.Push(Vector4(knots[i], p1[i], p2[i], knots[i + 1]));
    }
    return result;
}

float SampleBezierCurve(const BezierCurve1D& curve, float location)
{
    assert(!curve.Empty());

    const unsigned numKnots = curve.Size() + 1;
    const unsigned basePoint = Clamp(static_cast<unsigned>(location), 0u, numKnots - 1);

    const float t = Clamp(location - static_cast<float>(basePoint), 0.0f, 1.0f);
    const float q = 1.0f - t;
    const Vector4& p = curve[basePoint];
    return q*q*q*p.x_ + 3 * q*q*t*p.y_ + 3 * q*t*t*p.z_ + t*t*t*p.w_;
}

float SampleBezierCurveDerivative(const BezierCurve1D& curve, float location)
{
    assert(!curve.Empty());

    const unsigned numKnots = curve.Size() + 1;
    const unsigned basePoint = Clamp(static_cast<unsigned>(location), 0u, numKnots - 1);

    const float t = Clamp(location - static_cast<float>(basePoint), 0.0f, 1.0f);
    const Vector4& p = curve[basePoint];
    return -3*(1 - t)*(1 - t)*p.x_ + 3 * (1 - 4*t + 3*t*t)*p.y_ + 3 * (2*t - 3*t*t)*p.z_ + 3*t*t*p.w_;
}

BezierCurve2D CreateBezierCurve(const PODVector<Vector2>& values)
{
    BezierCurve2D result;
    result.xcoef_ = CreateBezierCurve(SpliceVectorArray(values, 0));
    result.ycoef_ = CreateBezierCurve(SpliceVectorArray(values, 1));
    return result;
}

Vector2 SampleBezierCurve(const BezierCurve2D& curve, float location)
{
    Vector2 result;
    result.x_ = SampleBezierCurve(curve.xcoef_, location);
    result.y_ = SampleBezierCurve(curve.ycoef_, location);
    return result;
}

Vector2 SampleBezierCurveDerivative(const BezierCurve2D& curve, float location)
{
    Vector2 result;
    result.x_ = SampleBezierCurveDerivative(curve.xcoef_, location);
    result.y_ = SampleBezierCurveDerivative(curve.ycoef_, location);
    return result;
}

BezierCurve3D CreateBezierCurve(const PODVector<Vector3>& values)
{
    BezierCurve3D result;
    result.xcoef_ = CreateBezierCurve(SpliceVectorArray(values, 0));
    result.ycoef_ = CreateBezierCurve(SpliceVectorArray(values, 1));
    result.zcoef_ = CreateBezierCurve(SpliceVectorArray(values, 2));
    return result;
}

Vector3 SampleBezierCurve(const BezierCurve3D& curve, float location)
{
    Vector3 result;
    result.x_ = SampleBezierCurve(curve.xcoef_, location);
    result.y_ = SampleBezierCurve(curve.ycoef_, location);
    result.z_ = SampleBezierCurve(curve.zcoef_, location);
    return result;
}

Vector3 SampleBezierCurveDerivative(const BezierCurve3D& curve, float location)
{
    Vector3 result;
    result.x_ = SampleBezierCurveDerivative(curve.xcoef_, location);
    result.y_ = SampleBezierCurveDerivative(curve.ycoef_, location);
    result.z_ = SampleBezierCurveDerivative(curve.zcoef_, location);
    return result;
}

}