#include "Procedural/Scripts/ModelFactoryWrapper.as"

class FoliageDesc
{
    /// Leaf size.
    Vector2 leafSize_;
    /// Relative branch size.
    float branchSize_;
    /// Relative branch height.
    Vector4 branchHeight_;
    /// Leaf trails stride distance.
    float trailStride_;
    /// Trail length above branch.
    Vector4 trailLengthAbove_;
    /// Trail length below branch.
    Vector4 trailLengthBelow_;
    /// Trail length noise.
    Vector2 trailLengthNoise_;
    /// Leaf stride distance.
    float leafStride_;
    /// Leaf noise.
    Vector2 leafNoise_;
}

float VectorPower(float x, Vector4 power)
{
    return power.x + power.y * x ** 2 + power.z * x ** 4 + power.w * x ** 6;
}

float Fract(float value)
{
    return value - Floor(value);
}

float PseudoRandom(Vector2 uv)
{
    return Fract(Sin(uv.DotProduct(Vector2(12.9898, 78.233)) * M_RADTODEG) * 43758.5453);
}

Vector2 PseudoRandom2(Vector2 uv)
{
    return Vector2(PseudoRandom(uv), PseudoRandom(uv + Vector2(1, 3)));
}

void GenerateFoliage(ModelFactoryWrapper& model, FoliageDesc desc)
{
    for (float x = -desc.branchSize_/2; x <= desc.branchSize_/2; x += desc.trailStride_)
    {
        Vector2 trailBase = Vector2(x, VectorPower(2*x, desc.branchHeight_));
        Vector2 noise = PseudoRandom2(Vector2(x, 1)) * desc.trailLengthNoise_;
        float yFrom = VectorPower(2*x, desc.trailLengthAbove_) - noise.x;
        float yTo = VectorPower(2*x, desc.trailLengthBelow_) + noise.y;
        if (yFrom >= yTo)
        {
            for (float y = yFrom; y >= yTo; y -= desc.leafStride_)
            {
                Vector2 leafBase = Vector2(0, y);
                Vector2 position = trailBase + leafBase;
                
                position += (PseudoRandom2(trailBase + leafBase) - Vector2(0.5, 0.5)) * desc.leafNoise_;
                model.AddRect2D(Vector3(position, PseudoRandom(position) * 0.1), 0.0, desc.leafSize_, Vector2(0, 0), Vector2(1, 1), Vector2(0, 1), Vector4());
            }
        }
    }
}

void Main(ModelFactory@ dest)
{
    ModelFactoryWrapper model(dest);
    
    FoliageDesc desc;
    desc.leafSize_ = Vector2(1, 1) * 0.05;
    desc.branchHeight_ = Vector4(0.7, -0.2, 0, 0);
    desc.branchSize_ = 0.8;
    desc.trailStride_ = 0.1;
    desc.trailLengthAbove_ = Vector4(0.2, 0, -0.2, 0);
    desc.trailLengthBelow_ = Vector4(-0.7, 0.2, 0, 0);
    desc.trailLengthNoise_ = Vector2(0.1, 0.2);
    desc.leafStride_ = 0.03;
    desc.leafNoise_ = Vector2(0.1, 0.05);
    
    GenerateFoliage(model, desc);
}
