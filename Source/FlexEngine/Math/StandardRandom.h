#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Container/Ptr.h>

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
    /// Get random integer from range.
    int IntegerFromRange(int min, int max);
    /// Get random float from range.
    float FloatFromRange(float min, float max);
    /// Get random real [0, 1].
    float FloatFrom01();
    /// Get random real [-1, 1].
    float FloatFrom11();

private:
    struct Core;
    /// Generator core.
    UniquePtr<Core> core_;

};

}

