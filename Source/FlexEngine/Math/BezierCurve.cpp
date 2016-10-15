#include <FlexEngine/Math/BezierCurve.h>

#include <FlexEngine/Container/Utility.h>

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/Sort.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Math/MathDefs.h>

namespace FlexEngine
{

//////////////////////////////////////////////////////////////////////////
PODVector<Vector4> CreateBezierCurve(const PODVector<float>& values)
{
    PODVector<Vector4> result;
    if (values.Size() < 2)
    {
        URHO3D_LOGERROR("Curve can be created from at least 2 points");
        return result;
    }

    const unsigned n = values.Size() - 1;
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
    r[0] = values[0] + 2 * values[1];

    // internal segments
    for (unsigned i = 1; i < n - 1; i++)
    {
        a[i] = 1;
        b[i] = 4;
        c[i] = 1;
        r[i] = 4 * values[i] + 2 * values[i + 1];
    }

    // right segment
    a[n - 1] = 2;
    b[n - 1] = 7;
    c[n - 1] = 0;
    r[n - 1] = 8 * values[n - 1] + values[n];

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
        p2[i] = 2 * values[i + 1] - p1[i + 1];
    }

    p2[n - 1] = 0.5f*(values[n] + p1[n - 1]);

    // Merge result
    for (unsigned i = 0; i < n; ++i)
    {
        result.Push(Vector4(values[i], p1[i], p2[i], values[i + 1]));
    }
    return result;
}

float SampleBezierCurveAbs(const BezierCurve1D& curve, float location)
{
    if (curve.Empty())
    {
        URHO3D_LOGERROR("Cannot sample empty curve");
        return 0.0f;
    }

    const unsigned numKnots = curve.Size() + 1;
    const unsigned basePoint = Clamp(static_cast<unsigned>(location), 0u, numKnots - 2);

    const float t = Clamp(location - static_cast<float>(basePoint), 0.0f, 1.0f);
    const float q = 1.0f - t;
    const Vector4& p = curve[basePoint];
    return q*q*q*p.x_ + 3 * q*q*t*p.y_ + 3 * q*t*t*p.z_ + t*t*t*p.w_;
}

float SampleBezierCurveDerivativeAbs(const BezierCurve1D& curve, float location)
{
    if (curve.Empty())
    {
        return 0.0f;
    }

    const unsigned numKnots = curve.Size() + 1;
    const unsigned basePoint = Clamp(static_cast<unsigned>(location), 0u, numKnots - 2);

    const float t = Clamp(location - static_cast<float>(basePoint), 0.0f, 1.0f);
    const Vector4& p = curve[basePoint];
    return -3*(1 - t)*(1 - t)*p.x_ + 3 * (1 - 4*t + 3*t*t)*p.y_ + 3 * (2*t - 3*t*t)*p.z_ + 3*t*t*p.w_;
}

float SampleBezierCurve(const BezierCurve1D& curve, float location)
{
    return SampleBezierCurveAbs(curve, location * curve.Size());
}

float SampleBezierCurveDerivative(const BezierCurve1D& curve, float location)
{
    return SampleBezierCurveDerivativeAbs(curve, location * curve.Size());
}

//////////////////////////////////////////////////////////////////////////
BezierCurve2D CreateBezierCurve(const PODVector<Vector2>& values)
{
    BezierCurve2D result;
    result.xcoef_ = CreateBezierCurve(SpliceVectorArray(values, 0));
    result.ycoef_ = CreateBezierCurve(SpliceVectorArray(values, 1));
    return result;
}

Vector2 SampleBezierCurveAbs(const BezierCurve2D& curve, float location)
{
    Vector2 result;
    result.x_ = SampleBezierCurveAbs(curve.xcoef_, location);
    result.y_ = SampleBezierCurveAbs(curve.ycoef_, location);
    return result;
}

Vector2 SampleBezierCurveDerivativeAbs(const BezierCurve2D& curve, float location)
{
    Vector2 result;
    result.x_ = SampleBezierCurveDerivativeAbs(curve.xcoef_, location);
    result.y_ = SampleBezierCurveDerivativeAbs(curve.ycoef_, location);
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

//////////////////////////////////////////////////////////////////////////
BezierCurve3D CreateBezierCurve(const PODVector<Vector3>& values)
{
    BezierCurve3D result;
    result.xcoef_ = CreateBezierCurve(SpliceVectorArray(values, 0));
    result.ycoef_ = CreateBezierCurve(SpliceVectorArray(values, 1));
    result.zcoef_ = CreateBezierCurve(SpliceVectorArray(values, 2));
    return result;
}

Vector3 SampleBezierCurveAbs(const BezierCurve3D& curve, float location)
{
    Vector3 result;
    result.x_ = SampleBezierCurveAbs(curve.xcoef_, location);
    result.y_ = SampleBezierCurveAbs(curve.ycoef_, location);
    result.z_ = SampleBezierCurveAbs(curve.zcoef_, location);
    return result;
}

Vector3 SampleBezierCurveDerivativeAbs(const BezierCurve3D& curve, float location)
{
    Vector3 result;
    result.x_ = SampleBezierCurveDerivativeAbs(curve.xcoef_, location);
    result.y_ = SampleBezierCurveDerivativeAbs(curve.ycoef_, location);
    result.z_ = SampleBezierCurveDerivativeAbs(curve.zcoef_, location);
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

//////////////////////////////////////////////////////////////////////////
CubicCurve CreateCubicCurve(PODVector<CubicCurvePoint> points, bool silent /*= false*/)
{
    Sort(points.Begin(), points.End());

    CubicCurve result;
    if (points.Size() < 2)
    {
        if (!silent)
        {
            URHO3D_LOGERROR("Curve can be created from at least 2 points");
        }
        return result;
    }

    for (unsigned i = 0; i < points.Size(); ++i)
    {
        result.locations_.Push(points[i].t_);
    }

    for (unsigned i = 0; i < points.Size() - 1; ++i)
    {
        const float x0 = points[i].x_;
        const float x1 = points[i + 1].x_;
        const float dx0 = points[i].dxr_;
        const float dx1 = points[i + 1].dxl_;
        result.segments_.Push(Vector4(x0, x0 + dx0 / 3.0f, x1 - dx1 / 3.0f, x1));
    }
    return result;
}

float SampleCubicCurve(const CubicCurve& curve, float location)
{
    if (curve.segments_.Empty())
    {
        URHO3D_LOGERROR("Cannot sample empty curve");
        return 0.0f;
    }

    const unsigned base = LowerBound(curve.locations_.Begin(), curve.locations_.End(), location) - curve.locations_.Begin();
    if (base == 0)
    {
        return curve.segments_.Front().x_;
    }
    if (base == curve.locations_.Size())
    {
        return curve.segments_.Back().w_;
    }

    assert(curve.locations_.Size() == curve.segments_.Size() + 1);
    const Vector4& p = curve.segments_[base - 1];
    const float t0 = curve.locations_[base - 1];
    const float t1 = curve.locations_[base];
    const float t = Clamp(InverseLerp(t0, t1, location), 0.0f, 1.0f);
    const float q = 1.0f - t;

    return q*q*q*p.x_ + 3 * q*q*t*p.y_ + 3 * q*t*t*p.z_ + t*t*t*p.w_;
}

PODVector<CubicCurvePoint> ReadCubicCurve(const String& str, bool silent /*= false*/)
{
    StringVector tokens = str.Replaced('|', ' ') .Split(' ');
    ReverseVector(tokens);

    PODVector<CubicCurvePoint> result;
    if (tokens.Empty())
    {
        if (!silent)
        {
            URHO3D_LOGERROR("String mustn't be empty");
        }
        return result;
    }

    PODVector<CubicCurvePoint> points;
    if (tokens.Back().Compare("ex", false) == 0)
    {
        tokens.Pop();
        if (tokens.Size() % 4 != 0)
        {
            if (!silent)
            {
                URHO3D_LOGERROR("Extended cubic curve description must have 4n tokens (except 'ex')");
            }
            return result;
        }
        while (!tokens.Empty())
        {
            CubicCurvePoint point;
            point.t_ = ToFloat(PopElement(tokens));
            point.x_ = ToFloat(PopElement(tokens));
            point.dxl_ = ToFloat(PopElement(tokens));
            point.dxr_ = ToFloat(PopElement(tokens));
            points.Push(point);
        }
    }
    else
    {
        if (tokens.Size() % 3 != 0)
        {
            if (!silent)
            {
                URHO3D_LOGERROR("Extended cubic curve description must have 3n tokens");
            }
            return result;
        }
        while (!tokens.Empty())
        {
            CubicCurvePoint point;
            point.t_ = ToFloat(PopElement(tokens));
            point.x_ = ToFloat(PopElement(tokens));
            point.dxl_ = point.dxr_ = ToFloat(PopElement(tokens));
            points.Push(point);
        }
    }
    return points;
}

PODVector<CubicCurvePoint> ReadCubicCurveAliased(const String& str, bool silent /*= false*/)
{
    static const HashMap<String, String> aliases =
    {
        MakePair<String, String>("zero",        "0 0  0 | 1 0  0"),
        MakePair<String, String>("one",         "0 1  0 | 1 1  0"),
        MakePair<String, String>("linear",      "0 0  1 | 1 1  1"),
        MakePair<String, String>("1-linear",    "0 1 -1 | 1 0 -1"),
        MakePair<String, String>("cos",         ToString("%f %f %f | %f %f %f", 0.0f, 0.0f,  0.0f,        1.0f, 1.0f,  M_PI / 2)),
        MakePair<String, String>("1-cos",       ToString("%f %f %f | %f %f %f", 0.0f, 1.0f,  0.0f,        1.0f, 0.0f, -M_PI / 2)),
        MakePair<String, String>("sin",         ToString("%f %f %f | %f %f %f", 0.0f, 0.0f,  M_PI / 2,    1.0f, 1.0f,  0.0f)),
        MakePair<String, String>("1-sin",       ToString("%f %f %f | %f %f %f", 0.0f, 1.0f, -M_PI / 2,    1.0f, 0.0f,  0.0f)),
        MakePair<String, String>("hermite",     "0 0 0 | 1 1 0"),
        MakePair<String, String>("1-hermite",   "0 1 0 | 1 0 0"),
    };
    const String* resolvedString = aliases[str.Trimmed()];
    if (resolvedString)
    {
        return ReadCubicCurve(*resolvedString, silent);
    }
    else
    {
        return ReadCubicCurve(str, silent);
    }
}

//////////////////////////////////////////////////////////////////////////
CubicCurveWrapper::CubicCurveWrapper(const String& str)
{
    SetCurveString(str);
}

void CubicCurveWrapper::SetCurveString(const String& str, bool silent)
{
    str_ = str;
    const CubicCurve newCurve = CreateCubicCurve(ReadCubicCurveAliased(str, silent), silent);
    if (!newCurve.segments_.Empty())
    {
        curve_ = newCurve;
    }
}

const String& CubicCurveWrapper::GetCurveString() const
{
    return str_;
}

void CubicCurveWrapper::SetResultRange(const Vector2& range)
{
    range_.SetVector(range);
}

const FloatRange& CubicCurveWrapper::GetResultRange() const
{
    return range_;
}

float CubicCurveWrapper::ComputeValue(float location) const
{
    return range_.Get(SampleCubicCurve(curve_, location));
}

}