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
using MathFunctionConstructor = MathFunctionSPtr(*) (const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters);

/// Math function description.
struct MathFunctionInfo
{
    int minInputs_;
    int maxInputs_;
    int minParams_;
    int maxParams_;
    MathFunctionConstructor construct_;
};

/// Math functions description map.
static HashMap<String, MathFunctionInfo> mathFunctions =
{
    {"x",     { 0, 0, 0, 0,   &MathFunction::Construct              }},
    {"const", { 0, 0, 1, 1,   &ConstantMathFunction::Construct      }},
    {"poly",  { 1, 1, 1,-1,   &PolynomialMathFunction::Construct    }},
    {"sin",   { 1, 1, 4, 4,   &HarmonicalMathFunction::Construct    }},
    {"clamp", { 1, 1, 2, 4,   &ClampedMathFunction::Construct       }},
    {"fit",   { 1, 1, 2, 4,   &ScaledMathFunction::Construct        }},
};

/// Math functions aliases
static HashMap<String, String> mathAliases =
{
    {"zero",    "const(0)"                          },
    {"one",     "const(1)"                          },
    {"linear",  "poly (x(), 0,1)"                   },
    {"square",  "poly (x(), 0,0,1)"                 },
    {"smright", "sin  (x(),  90,  0,   1,   0)"     },
    {"smleft",  "sin  (x(), -90,  90, -1,   1)"     },
    {"smboth",  "sin  (x(), -180, 90, -0.5, 0.5)"   },
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
MathFunctionSPtr ConstructMathFunction(const String& sourceString, const String& name,
    const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    auto iter = mathFunctions.Find(name);
    if (iter == mathFunctions.End())
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': unknown function '%s'",
            sourceString.CString(), name);
        return nullptr;
    }

    MathFunctionInfo& desc = iter->second_;

    // Check inputs and parameters
    if (desc.minInputs_ >= 0 && inputs.Size() < static_cast<unsigned>(desc.minInputs_))
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': function '%s' must have at least %d inputs",
            sourceString.CString(), name, desc.minInputs_);
        return nullptr;
    }

    if (desc.maxInputs_ >= 0 && inputs.Size() > static_cast<unsigned>(desc.maxInputs_))
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': function '%s' must have at most %d inputs",
            sourceString.CString(), name, desc.maxInputs_);
        return nullptr;
    }

    if (desc.minParams_ >= 0 && parameters.Size() < static_cast<unsigned>(desc.minParams_))
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': function '%s' must have at least %d parameters",
            sourceString.CString(), name, desc.minParams_);
        return nullptr;
    }

    if (desc.maxParams_ >= 0 && parameters.Size() > static_cast<unsigned>(desc.maxParams_))
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': function '%s' must have at most %d parameters",
            sourceString.CString(), name, desc.maxParams_);
        return nullptr;
    }

    // Construct function
    return desc.construct_(inputs, parameters);
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

    // Read function name
    const String functionName = PopElement(tokens);
    if (PopElement(tokens) != "(")
    {
        URHO3D_LOGERRORF("Cannot parse input function '%s': '(' is missing", sourceString.CString());
        return nullptr;
    }

    // Read inputs and parameters
    MathFunctionInputVector inputs;
    MathFunctionParameterVector parameters;
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
        if (IsAlpha(token[0]))
        {
            tokens.Push(token);
            const MathFunctionSPtr input = ConstructMathFunction(sourceString, tokens);
            if (!input)
            {
                URHO3D_LOGERRORF("Cannot parse input function '%s': failed to parse nested function", sourceString.CString());
                return nullptr;
            }
            inputs.Push(input);
        }
        // Load parameter
        else
        {
            if (token.Compare("inf", false) == 0 || token.Compare("+inf", false) == 0)
            {
                parameters.Push(M_INFINITY);
            }
            else if (token.Compare("-inf", false) == 0)
            {
                parameters.Push(-M_INFINITY);
            }
            else
            {
                parameters.Push(ToDouble(token));
            }
        }
    }

    // Construct function
    return ConstructMathFunction(sourceString, functionName, inputs, parameters);
}

}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr MathFunction::Construct(const MathFunctionInputVector& /*inputs*/, const MathFunctionParameterVector& /*parameters*/)
{
    return MakeShared<MathFunction>();
}

MathFunction::~MathFunction()
{
}

double MathFunction::Compute(double value) const
{
    return value;
}

float MathFunction::Compute(float value) const
{
    return static_cast<float>(Compute(static_cast<double>(value)));
}

//////////////////////////////////////////////////////////////////////////

MathFunctionSPtr ConstantMathFunction::Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    return MakeShared<ConstantMathFunction>(parameters[0]);
}

ConstantMathFunction::ConstantMathFunction(double value)
    : value_(value)
{
}

double ConstantMathFunction::Compute(double /*value*/) const
{
    return value_;
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr PolynomialMathFunction::Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    return MakeShared<PolynomialMathFunction>(inputs[0], parameters);
}

PolynomialMathFunction::PolynomialMathFunction(MathFunctionSPtr fun, const Vector<double>& poly)
    : fun_(fun)
    , poly_(poly)
{
}

double PolynomialMathFunction::Compute(double value) const
{
    value = fun_->Compute(value);

    double result = 0.0;
    double valuePower = 1.0;
    for (const double coef : poly_)
    {
        result += valuePower * coef;
        valuePower *= value;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr HarmonicalMathFunction::Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    return MakeShared<HarmonicalMathFunction>(inputs[0], parameters[0], parameters[1], parameters[2], parameters[3]);
}

HarmonicalMathFunction::HarmonicalMathFunction(MathFunctionSPtr fun, double period, double phase, double scale, double offset)
    : fun_(fun)
    , period_(period)
    , phase_(phase)
    , scale_(scale)
    , offset_(offset)
{
}

double HarmonicalMathFunction::Compute(double value) const
{
    return Sin(period_ * fun_->Compute(value) + phase_) * scale_ + offset_;
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr ClampedMathFunction::Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    const double minInput = parameters.Size() > 2 ? parameters[2] : 0.0;
    const double maxInput = parameters.Size() > 3 ? parameters[3] : 1.0;
    return MakeShared<ClampedMathFunction>(inputs[0], parameters[0], parameters[1], minInput, maxInput);
}

ClampedMathFunction::ClampedMathFunction(MathFunctionSPtr fun, double minOutput, double maxOutput, double minInput, double maxInput)
    : fun_(fun)
    , minOutput_(minOutput)
    , maxOutput_(maxOutput)
    , minInput_(minInput)
    , maxInput_(maxInput)
{
    assert(fun_);
}

double ClampedMathFunction::Compute(double value) const
{
    value = fun_ ? fun_->Compute(value) : value;
    return Clamp(fun_->Compute(Clamp(value, minInput_, maxInput_)), minOutput_, maxOutput_);
}

//////////////////////////////////////////////////////////////////////////
MathFunctionSPtr ScaledMathFunction::Construct(const MathFunctionInputVector& inputs, const MathFunctionParameterVector& parameters)
{
    const double minInput = parameters.Size() > 2 ? parameters[2] : 0.0;
    const double maxInput = parameters.Size() > 3 ? parameters[3] : 1.0;
    return MakeShared<ScaledMathFunction>(inputs[0], parameters[0], parameters[1], minInput, maxInput);
}

ScaledMathFunction::ScaledMathFunction(MathFunctionSPtr fun, double minOutput, double maxOutput, double minInput, double maxInput)
    : fun_(fun)
    , minOutput_(minOutput)
    , maxOutput_(maxOutput)
    , minInput_(minInput)
    , maxInput_(maxInput)
{
}

double ScaledMathFunction::Compute(double value) const
{
    value = fun_ ? fun_->Compute(value) : value;
    return Lerp(minOutput_, maxOutput_, fun_->Compute(UnLerp(minInput_, maxInput_, value)));
}

//////////////////////////////////////////////////////////////////////////
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
        result.Push(fun.Compute(value));
    }
    return result;
}

template <>
MathFunctionSPtr To<MathFunctionSPtr>(const String& str)
{
    return CreateMathFunction(str);
}

}
