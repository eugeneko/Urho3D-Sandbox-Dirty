#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Ptr.h>

namespace FlexEngine
{

/// Standard random generator.
class StandardRandom
{
public:
    /// Construct.
    StandardRandom(unsigned seed = 0);
    /// Destruct.
    ~StandardRandom();
    /// Reset.
    void Reset(unsigned seed = 0);

    /// Get random unsigned integer.
    unsigned Random();
    /// Get random integer from range [min, max].
    int IntegerFromRange(int min, int max);
    /// Get random float from range [min, max].
    float FloatFromRange(float min, float max);
    /// Get random float from range [0, 1].
    float FloatFrom01();
    /// Get random float from range [-1, 1].
    float FloatFrom11();
    /// Get random float vector from range [0, 1].
    Vector4 Vector4From01();
    /// Get random float matrix from range [0, 1].
    Matrix4 Matrix4From01();

private:
    struct Core;
    /// Generator core.
    UniquePtr<Core> core_;

};

}

