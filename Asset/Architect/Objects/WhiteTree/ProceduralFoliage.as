#include "Scripts/ModelFactoryWrapper.as"

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

void MainFoliageMask(ModelFactory@ dest)
{
    ModelFactoryWrapper model(dest);
    
    FoliageDesc desc;
    desc.leafSize_ = Vector2(1, 1) * 0.02;
    desc.branchHeight_ = Vector4(0.7, -0.2, 0, 0);
    desc.branchSize_ = 0.8;
    desc.trailStride_ = 0.05;
    desc.trailLengthAbove_ = Vector4(0.2, 0, -0.2, 0);
    desc.trailLengthBelow_ = Vector4(-0.7, 0.2, 0, 0);
    desc.trailLengthNoise_ = Vector2(0.1, 0.2);
    desc.leafStride_ = 0.015;
    desc.leafNoise_ = Vector2(0.1, 0.05);
    
    GenerateFoliage(model, desc);
}

class LeafDesc
{
    /// Number of leaf segments.
    uint numSegments_;
    /// Relative width of each segment in % of initial value.
    float segmentWidth_;
    /// Angles of segments [fromAngle, toAngle]
    Vector2 angleRange_;
    /// Specifies trimming of segment position.
    Vector2 positionRange_;
    /// Describes length of segments as ||(1-x)^p - (1-x)^n||, where x is position, p is shapePower.x, n is shapePower.y
    /// and || is normalization to unit width.
    Vector2 shapePower_;
    /// Length of segments.
    float width_;
    /// Info passed to mask shader.
    Vector4 segmentMaskInfo_;
}

void GenerateLeaf(ModelFactoryWrapper& model, LeafDesc desc)
{
    float shapePowerPos = desc.shapePower_.x;
    float shapePowerNeg = desc.shapePower_.y;
    float lengthScaleY = Pow(shapePowerPos / shapePowerNeg, 1 / (shapePowerNeg - shapePowerPos));
    float lengthScale = Pow(lengthScaleY, shapePowerPos) - Pow(lengthScaleY, shapePowerNeg);
    
    for (uint i = 0; i < desc.numSegments_; ++i)
    {
        float factor = (i + 0.5) / desc.numSegments_;
        Vector2 position = Vector2(0.0, Lerp(desc.positionRange_.x, desc.positionRange_.y, factor));
        float angle = Lerp(desc.angleRange_.x, desc.angleRange_.y, factor);
        float len = (Pow(1.0 - position.y, shapePowerPos) - Pow(1.0 - position.y, shapePowerNeg)) / lengthScale * desc.width_;
        Vector2 scale = Vector2(desc.segmentWidth_ / desc.numSegments_, 0.5 * len);

        model.AddRect2D(Vector3(position), +angle, scale, Vector2(-0.5, 1.0 - len), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);
        model.AddRect2D(Vector3(position), -angle, scale, Vector2(-0.5, 1.0 - len), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);
    }
    
    Vector2 trunkScale = Vector2(desc.segmentWidth_ / desc.numSegments_, Lerp(1.0, desc.positionRange_.y, 0.5) - desc.positionRange_.x);
    model.AddRect2D(Vector3(0.0, desc.positionRange_.x, 0.0), 0.0, trunkScale, Vector2(-0.5, 0.0), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);
}

void MainLeafMask(ModelFactory@ dest)
{
    ModelFactoryWrapper model(dest);

    LeafDesc desc;
    desc.numSegments_ = 30;
    desc.segmentWidth_ = 2.5;
    desc.angleRange_ = Vector2(80.0, 20.0);
    desc.positionRange_ = Vector2(0.05, 0.90);
    desc.shapePower_ = Vector2(8.0, 1.3);
    desc.width_ = 0.8;
    desc.segmentMaskInfo_ =  Vector4(1.0, 10.0, 0.1, 1.0);
    
    GenerateLeaf(model, desc);
}
