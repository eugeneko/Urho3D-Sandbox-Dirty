#include <FlexEngine/Math/MathFunction.h>

#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Container/Utility.h>
#include <FlexEngine/Core/StringUtils.h>
#include <FlexEngine/Math/MathDefs.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/Log.h>

namespace FlexEngine
{

namespace
{

/// Math function construct function.
using MathFunctionConstructor = MathFunctionSPtr(*) (const MathFunctionVector& inputs);

/// Math functions description map.
static const HashMap<String, MathFunctionConstructor> mathFunctions =
{
    { InputMathFunction::GetName(),      &InputMathFunction::Construct      },
    { TaylorMathFunction::GetName(),     &TaylorMathFunction::Construct     },
    { HarmonicalMathFunction::GetName(), &HarmonicalMathFunction::Construct },
    { ClampedMathFunction::GetName(),    &ClampedMathFunction::Construct    },
    { ScaledMathFunction::GetName(),     &ScaledMathFunction::Construct     },
};

/// Math functions aliases
static const HashMap<String, String> mathAliases =
{
    { "zero",    "0"                            },
    { "one",     "1"                            },
    { "linear",  "tailor (x, 0, 1)"             },
    { "square",  "tailor (x, 0, 0, 1)"          },
    { "smright", "sin (x,  90,  0,   1,   0)"   },
    { "smleft",  "sin (x, -90,  90, -1,   1)"   },
    { "smboth",  "sin (x, -180, 90, -0.5, 0.5)" },
    { "x",       "input(0)"                     },
};

/// Pre-process format of math function.
String SanitateMathFunctionCode(const String& str)
{
    String buf = str.Trimmed();

    // Replace aliases
    for (const auto& pair : mathAliases)
    {
        buf.Replace(pair.first_, pair.second_);
    }

    // Add spaces around tokens
    buf.Replace("(", " ( ");
    buf.Replace(")", " ) ");

    // Remove commas
    buf.Replace(",", " ");

    return buf;
}

/// Constructs math function with name, inputs and parameters.
MathFunctionSPtr ConstructMathFunction(const String& sourceString, const String& name, const MathFunctionVector& inputs)
{
    auto iter = mathFunctions.Find(name);
    if (iter == mathFunctions.End())
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': unknown function '%s'",
            sourceString.CString(), name.CString());
        return nullptr;
    }

    const MathFunctionConstructor construct = iter->second_;

    // Construct function
    return construct(inputs);
}

/// Constructs math function from token sequence
MathFunctionSPtr ConstructMathFunction(const String& sourceString, Vector<String>& tokens)
{
    // Empty token list is not allowed
    if (tokens.Empty())
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': it is empty", sourceString.CString());
        return nullptr;
    }

    // Read the first token
    const String firstToken = PopElement(tokens);

    // Parse constant
    if (!IsAlpha(firstToken[0]))
    {
        double value = 0.0;
        if (firstToken.Compare("inf", false) == 0 || firstToken.Compare("+inf", false) == 0)
        {
            value = M_INFINITY;
        }
        else if (firstToken.Compare("-inf", false) == 0)
        {
            value = -M_INFINITY;
        }
        else
        {
            value = ToDouble(firstToken);
        }
        return CreateConstFunction(value);
    }

    // Parse function
    const String functionName = firstToken;
    if (PopElement(tokens) != "(")
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': '(' is missing", sourceString.CString());
        return nullptr;
    }

    // Read inputs and parameters
    MathFunctionVector inputs;
    String token;
    while ((token = PopElement(tokens)) != ")")
    {
        // Last token must be ')'
        if (tokens.Empty())
        {
            URHO3D_LOGERRORF("Cannot parse input function '%s': ')' is missing", sourceString.CString());
            return nullptr;
        }
        
        // Load new input
        tokens.Push(token);
        const MathFunctionSPtr input = ConstructMathFunction(sourceString, tokens);
        if (!input)
        {
            URHO3D_LOGERRORF("Cannot parse input function '%s': failed to parse nested function", sourceString.CString());
            return nullptr;
        }

        inputs.Push(input);
    }

    // Construct function
    return ConstructMathFunction(sourceString, functionName, inputs);
}

}

//////////////////////////////////////////////////////////////////////////
MathFunction::~MathFunction()
{
}

float MathFunction::Compute(float value) const
{
    return static_cast<float>(Compute(DoubleVector{ static_cast<double>(value) }));
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr InputMathFunction::Construct(const MathFunctionVector& args)
{
    if (args.Size() > 1)
    {
        URHO3D_LOGERRORF(
            "Cannot create function with %d arguments, required signature: %s([inputIndex])",
            args.Size(), GetName().CString());
        return nullptr;
    }

    const MathFunctionSPtr inputIndex = args.Size() > 0 ? args[0] : CreateConstFunction(0);
    return MakeShared<InputMathFunction>(inputIndex);
}

InputMathFunction::InputMathFunction(MathFunctionSPtr inputIndex, double defaultValue /*= 0.0*/)
    : inputIndex_(inputIndex)
    , defaultValue_(defaultValue)
{
}

double InputMathFunction::Compute(const DoubleVector& inputs) const
{
    const unsigned inputIndex = static_cast<unsigned>(inputIndex_->Compute(inputs));
    return inputIndex < inputs.Size() ? inputs[inputIndex] : defaultValue_;
}

//////////////////////////////////////////////////////////////////////////

ConstantMathFunction::ConstantMathFunction(double value)
    : value_(value)
{
}

double ConstantMathFunction::Compute(const DoubleVector& /*inputs*/) const
{
    return value_;
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr TaylorMathFunction::Construct(const MathFunctionVector& args)
{
    if (args.Size() < 1)
    {
        URHO3D_LOGERRORF(
            "Cannot create function with %d arguments, required signature: %s(input, a0[, a1...])",
            args.Size(), GetName().CString());
        return nullptr;
    }

    MathFunctionVector parameters = args;
    parameters.Erase(0, 1);
    return MakeShared<TaylorMathFunction>(args[0], parameters);
}

TaylorMathFunction::TaylorMathFunction(MathFunctionSPtr fun, const MathFunctionVector& poly)
    : fun_(fun)
    , poly_(poly)
{
}

double TaylorMathFunction::Compute(const DoubleVector& inputs) const
{
    const double value = fun_->Compute(inputs);

    double result = 0.0;
    double valuePower = 1.0;
    for (const MathFunctionSPtr& coef : poly_)
    {
        result += valuePower * coef->Compute(inputs);
        valuePower *= value;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr HarmonicalMathFunction::Construct(const MathFunctionVector& args)
{
    if (args.Size() != 5)
    {
        URHO3D_LOGERRORF(
            "Cannot create function with %d arguments, required signature: %s(input, period, phase, scale, offset)",
            args.Size(), GetName().CString());
        return nullptr;
    }

    return MakeShared<HarmonicalMathFunction>(args[0], args[1], args[2], args[3], args[4]);
}

HarmonicalMathFunction::HarmonicalMathFunction(MathFunctionSPtr fun,
    MathFunctionSPtr period, MathFunctionSPtr phase, MathFunctionSPtr scale, MathFunctionSPtr offset)
    : fun_(fun)
    , period_(period)
    , phase_(phase)
    , scale_(scale)
    , offset_(offset)
{
}

double HarmonicalMathFunction::Compute(const DoubleVector& inputs) const
{
    const double angle = period_->Compute(inputs) * fun_->Compute(inputs) + phase_->Compute(inputs);
    return Sin(angle) * scale_->Compute(inputs) + offset_->Compute(inputs);
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr ClampedMathFunction::Construct(const MathFunctionVector& args)
{
    if (args.Size() < 3 || args.Size() > 5)
    {
        URHO3D_LOGERRORF(
            "Cannot create function with %d arguments, required signature: %s(input, minOutput, maxOutput[, minInput, maxInput])",
            args.Size(), GetName().CString());
        return nullptr;
    }

    const MathFunctionSPtr minInput = args.Size() > 3 ? args[3] : CreateConstFunction(0.0);
    const MathFunctionSPtr maxInput = args.Size() > 4 ? args[4] : CreateConstFunction(1.0);
    return MakeShared<ClampedMathFunction>(args[0], args[1], args[2], minInput, maxInput);
}

ClampedMathFunction::ClampedMathFunction(MathFunctionSPtr fun,
    MathFunctionSPtr minOutput, MathFunctionSPtr maxOutput, MathFunctionSPtr minInput, MathFunctionSPtr maxInput)
    : fun_(fun)
    , minOutput_(minOutput)
    , maxOutput_(maxOutput)
    , minInput_(minInput)
    , maxInput_(maxInput)
{
    assert(fun_);
}

double ClampedMathFunction::Compute(const DoubleVector& inputs) const
{
    const double value = Clamp(fun_->Compute(inputs), minInput_->Compute(inputs), maxInput_->Compute(inputs));
    return Clamp(value, minOutput_->Compute(inputs), maxOutput_->Compute(inputs));
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr ScaledMathFunction::Construct(const MathFunctionVector& args)
{
    if (args.Size() < 3 || args.Size() > 5)
    {
        URHO3D_LOGERRORF(
            "Cannot create function with %d arguments, required signature: %s(input, minOutput, maxOutput[, minInput, maxInput])",
            args.Size(), GetName().CString());
        return nullptr;
    }

    const MathFunctionSPtr minInput = args.Size() > 3 ? args[3] : CreateConstFunction(0.0);
    const MathFunctionSPtr maxInput = args.Size() > 4 ? args[4] : CreateConstFunction(1.0);
    return MakeShared<ScaledMathFunction>(args[0], args[1], args[2], minInput, maxInput);
}

ScaledMathFunction::ScaledMathFunction(MathFunctionSPtr fun,
    MathFunctionSPtr minOutput, MathFunctionSPtr maxOutput, MathFunctionSPtr minInput, MathFunctionSPtr maxInput)
    : fun_(fun)
    , minOutput_(minOutput)
    , maxOutput_(maxOutput)
    , minInput_(minInput)
    , maxInput_(maxInput)
{
}

double ScaledMathFunction::Compute(const DoubleVector& inputs) const
{
    const double value = UnLerp(minInput_->Compute(inputs), maxInput_->Compute(inputs), fun_->Compute(inputs));
    return Lerp(minOutput_->Compute(inputs), maxOutput_->Compute(inputs), value);
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr CreateConstFunction(double value)
{
    return MakeShared<ConstantMathFunction>(value);
}

MathFunctionSPtr CreateMathFunction(const String& str)
{
    Vector<String> tokens = SanitateMathFunctionCode(str).Split(' ');
    ReverseVector(tokens);
    return ConstructMathFunction(str, tokens);
}

PODVector<double> ComputeMathFunction(const MathFunction& fun, const PODVector<double>& values)
{
    PODVector<double> result;
    result.Reserve(values.Size());
    for (const double& value : values)
    {
        result.Push(fun.Compute(DoubleVector{ value }));
    }
    return result;
}

template <>
MathFunctionSPtr To<MathFunctionSPtr>(const String& str)
{
    return CreateMathFunction(str);
}

}
