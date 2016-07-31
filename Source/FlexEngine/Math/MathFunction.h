#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Math/MathDefs.h>

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Container/Ptr.h>

namespace FlexEngine
{

/// Vector of doubles.
using DoubleVector = PODVector<double>;

/// Math function vector.
using MathFunctionVector = Vector<MathFunctionSPtr>;

/// Interface of math function. Does nothing by default.
class MathFunction : public RefCounted
{
public:
    /// Destruct.
    virtual ~MathFunction();
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const = 0;
    /// Compute value.
    float Compute(float value) const;
};

/// Input math function.
class InputMathFunction : public MathFunction
{
public:
    /// Get name.
    static String GetName() { return "input"; }
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionVector& args);
    /// Construct by polynomial coefficients from x^0 to x^size.
    InputMathFunction(MathFunctionSPtr inputIndex, double defaultValue = 0.0);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const MathFunctionSPtr inputIndex_;
    const double defaultValue_;
};

/// Constant math function.
class ConstantMathFunction : public MathFunction
{
public:
    /// Construct by polynomial coefficients from x^0 to x^size.
    ConstantMathFunction(double value);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const double value_;
};

/// Taylor series math function.
class TaylorMathFunction : public MathFunction
{
public:
    /// Get name.
    static String GetName() { return "tailor"; }
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionVector& args);
    /// Construct by polynomial coefficients from x^0 to x^size.
    TaylorMathFunction(MathFunctionSPtr fun, const MathFunctionVector& poly);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const MathFunctionSPtr fun_;
    /// Polynomial coefficients.
    const MathFunctionVector poly_;
};

/// Harmonical math function.
class HarmonicalMathFunction : public MathFunction
{
public:
    /// Get name.
    static String GetName() { return "sin"; }
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionVector& args);
    /// Construct by coefficients <code>sin(period*value + phase) * scale + offset</code>.
    /// @note Degrees are used for sin computation.
    HarmonicalMathFunction(MathFunctionSPtr fun,
        MathFunctionSPtr period, MathFunctionSPtr phase, MathFunctionSPtr scale, MathFunctionSPtr offset);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const MathFunctionSPtr fun_;
    const MathFunctionSPtr period_;
    const MathFunctionSPtr phase_;
    const MathFunctionSPtr scale_;
    const MathFunctionSPtr offset_;
};

/// Clamped math function.
class ClampedMathFunction : public MathFunction
{
public:
    /// Get name.
    static String GetName() { return "clamp"; }
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionVector& args);
    /// Construct with base function and clamp edges for input and output.
    ClampedMathFunction(MathFunctionSPtr fun,
        MathFunctionSPtr minOutput, MathFunctionSPtr maxOutput, MathFunctionSPtr minInput, MathFunctionSPtr maxInput);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const MathFunctionSPtr fun_;
    const MathFunctionSPtr minOutput_;
    const MathFunctionSPtr maxOutput_;
    const MathFunctionSPtr minInput_;
    const MathFunctionSPtr maxInput_;
};

/// Scaled math function. Input is transformed from [min, max] to [0, 1]. Output is transformed from [0, 1] to [min, max].
class ScaledMathFunction : public MathFunction
{
public:
    /// Get name.
    static String GetName() { return "fit"; }
    /// Construct.
    static MathFunctionSPtr Construct(const MathFunctionVector& args);
    /// Construct with input and output ranges.
    ScaledMathFunction(MathFunctionSPtr fun,
        MathFunctionSPtr minOutput, MathFunctionSPtr maxOutput, MathFunctionSPtr minInput, MathFunctionSPtr maxInput);
    /// Compute value.
    virtual double Compute(const DoubleVector& inputs) const override;

private:
    const MathFunctionSPtr fun_;
    const MathFunctionSPtr minOutput_;
    const MathFunctionSPtr maxOutput_;
    const MathFunctionSPtr minInput_;
    const MathFunctionSPtr maxInput_;
};

/// Construct const math function.
MathFunctionSPtr CreateConstFunction(double value);

/// Construct function from string.
MathFunctionSPtr CreateMathFunction(const String& str);

/// Compute math function for each element.
PODVector<double> ComputeMathFunction(const MathFunction& fun, const PODVector<double>& values);

}
