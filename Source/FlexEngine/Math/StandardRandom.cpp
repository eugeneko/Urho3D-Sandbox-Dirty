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

Vector4 StandardRandom::Vector4From01()
{
    Vector4 result;
    result.x_ = FloatFrom01();
    result.y_ = FloatFrom01();
    result.z_ = FloatFrom01();
    result.w_ = FloatFrom01();
    return result;
}

Matrix4 StandardRandom::Matrix4From01()
{
    Matrix4 result;
    result.m00_ = FloatFrom01();
    result.m01_ = FloatFrom01();
    result.m02_ = FloatFrom01();
    result.m03_ = FloatFrom01();
    result.m10_ = FloatFrom01();
    result.m11_ = FloatFrom01();
    result.m12_ = FloatFrom01();
    result.m13_ = FloatFrom01();
    result.m20_ = FloatFrom01();
    result.m21_ = FloatFrom01();
    result.m22_ = FloatFrom01();
    result.m23_ = FloatFrom01();
    result.m30_ = FloatFrom01();
    result.m31_ = FloatFrom01();
    result.m32_ = FloatFrom01();
    result.m33_ = FloatFrom01();
    return result;
}

}