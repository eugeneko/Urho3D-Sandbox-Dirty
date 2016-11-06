#include <FlexEngine/Math/StandardRandom.h>

#include <random>

namespace FlexEngine
{

struct StandardRandom::Core
{
    Core(unsigned seed) : generator_(seed) { }

    std::mt19937 generator_;
};

StandardRandom::StandardRandom(unsigned seed /*= 0*/)
{
    Reset(seed);
}

StandardRandom::~StandardRandom()
{
}

void StandardRandom::Reset(unsigned seed /*= 0*/)
{
    core_ = MakeUnique<Core>(seed);
}

unsigned StandardRandom::Random()
{
    return core_->generator_();
}

int StandardRandom::IntegerFromRange(int min, int max)
{
    if (min > max)
        std::swap(min, max);
    std::uniform_int_distribution<int> uniformInt(min, max);
    return uniformInt(core_->generator_);
}

float StandardRandom::FloatFromRange(float min, float max)
{
    if (min > max)
        std::swap(min, max);
    std::uniform_real_distribution<float> uniformReal(min, max);
    return uniformReal(core_->generator_);
}

float StandardRandom::FloatFrom01()
{
    return FloatFromRange(0.0, 1.0);
}

float StandardRandom::FloatFrom11()
{
    return FloatFromRange(-1.0, 1.0);
}

}