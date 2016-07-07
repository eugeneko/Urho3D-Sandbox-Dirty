#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/MathDefs.h>

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Container/Ptr.h>

namespace FlexEngine
{

/// Math function input vector.
using MathFunctionInputVector = Vector<MathFunctionSPtr>;

/// Math function parameter vector.
using MathFunctionParameterVector = Vector<double>;

/// Interface of math function. Does nothing by default.
class MathFunction : public RefCounted
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Destruct.
    virtual ~MathFunction();
    /// Compute value.
    virtual double Compute(double value) const;
    /// Compute value.
    float Compute(float value) const;
};

/// Constant math function.
class ConstantMathFunction : public MathFunction
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Construct by polynomial coefficients from x^0 to x^size.
    ConstantMathFunction(double value);
    /// Compute value.
    virtual double Compute(double value) const override;

private:
    const double value_;
};

/// Polynomial math function.
class PolynomialMathFunction : public MathFunction
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Construct by polynomial coefficients from x^0 to x^size.
    PolynomialMathFunction(MathFunctionSPtr fun, const Vector<double>& poly);
    /// Compute value.
    virtual double Compute(double value) const override;

private:
    const MathFunctionSPtr fun_;
    /// Polynomial coefficients.
    const Vector<double> poly_;
};

/// Harmonical math function.
class HarmonicalMathFunction : public MathFunction
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Construct by coefficients <code>sin(period*value + phase) * scale + offset</code>.
    /// @note Degrees are used for sin computation.
    HarmonicalMathFunction(MathFunctionSPtr fun, double period, double phase, double scale, double offset);
    /// Compute value.
    virtual double Compute(double value) const override;

private:
    const MathFunctionSPtr fun_;
    const double period_;
    const double phase_;
    const double scale_;
    const double offset_;
};

/// Clamped math function.
class ClampedMathFunction : public MathFunction
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Construct with base function and clamp edges for input and output.
    ClampedMathFunction(MathFunctionSPtr fun, double minOutput, double maxOutput, double minInput, double maxInput);
    /// Compute value.
    virtual double Compute(double value) const override;

private:
    const MathFunctionSPtr fun_;
    const double minOutput_;
    const double maxOutput_;
    const double minInput_;
    const double maxInput_;
};

/// Scaled math function. Input is transformed from [min, max] to [0, 1]. Output is transformed from [0, 1] to [min, max].
class ScaledMathFunction : public MathFunction
{
public:
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);
    /// Construct with input and output ranges.
    ScaledMathFunction(MathFunctionSPtr fun, double minOutput, double maxOutput, double minInput, double maxInput);
    /// Compute value.
    virtual double Compute(double value) const override;

private:
    MathFunctionSPtr fun_;
    const double minOutput_;
    const double maxOutput_;
    const double minInput_;
    const double maxInput_;
};

/// Construct function from string.
MathFunctionSPtr CreateMathFunction(const String& str);

/// Compute math function for each element.
PODVector<double> ComputeMathFunction(const MathFunction& fun, const PODVector<double>& values);

}
