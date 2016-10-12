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
    /// Leaf stride distance.
    float leafStride_;
    /// Leaf noise.
    Vector2 leafNoise_;
    /// Fade out start.
    float fadeOutStart_;
    /// Fade out end.
    float fadeOutEnd_;
}

float VectorPower(float x, Vector4 power)
{
    return power.x + power.y * x ** 2 + power.z * x ** 4 + power.w * x ** 6;
}

float Fract(float value)
{
    return value - Floor(value);
}

float UnLerpClamped(float value, float lhs, float rhs)
{
    return Clamp((value - lhs) / (rhs - lhs), 0.0, 1.0);
}

float PseudoRandom(Vector2 uv)
{
    return Fract(Sin(uv.DotProduct(Vector2(12.9898, 78.233)) * M_RADTODEG) * 43758.5453);
}

Vector2 PseudoRandom2(Vector2 uv)
{
    return Vector2(PseudoRandom(uv), PseudoRandom(uv + Vector2(1, 3)));
}

void GenerateFoliage(ModelFactoryWrapper& model, FoliageDesc desc, float height)
{
    for (float x = -desc.branchSize_/2; x <= desc.branchSize_/2; x += desc.trailStride_)
    {
        for (float y = height; y >= 0; y -= desc.leafStride_)
        {
            Vector2 position = Vector2(x, y);
            Vector2 relativePosition = Vector2(Abs(position.x), Max(0.0, Abs(position.y - height / 2) - Max(0.0, height / 2 - 0.5)));
            float fadeOut = UnLerpClamped(relativePosition.length, desc.fadeOutStart_, desc.fadeOutEnd_);
            if (PseudoRandom(position) < fadeOut)
                continue;

            position += (PseudoRandom2(Vector2(x, y)) - Vector2(0.5, 0.5)) * desc.leafNoise_;
            model.AddRect2D(Vector3(position, PseudoRandom(position) * 0.1), 0.0, desc.leafSize_, Vector2(0, 0), Vector2(1, 1), Vector2(0, 1), Vector4());
        }
    }
}

Model@ MainFoliageMask(ProceduralContext@ context, float scale, float height)
{
    ModelFactory@ factory = context.CreateModelFactory();
    ModelFactoryWrapper model(factory);

    FoliageDesc desc;
    desc.leafSize_ = Vector2(1, 1) * 0.02 / scale;
    desc.branchHeight_ = Vector4(0.7, -0.2, 0, 0);
    desc.branchSize_ = 0.8;
    desc.trailStride_ = 0.05 / scale;
    desc.leafStride_ = 0.015 / scale;
    desc.leafNoise_ = Vector2(0.1, 0.05);
    desc.fadeOutStart_ = 0.3;
    desc.fadeOutEnd_ = 0.5;

    GenerateFoliage(model, desc, height);
    
    return context.CreateModel(factory);
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

Model@ MainLeafMask(ProceduralContext@ context)
{
    ModelFactory@ factory = context.CreateModelFactory();
    ModelFactoryWrapper model(factory);

    LeafDesc desc;
    desc.numSegments_ = 30;
    desc.segmentWidth_ = 2.5;
    desc.angleRange_ = Vector2(80.0, 20.0);
    desc.positionRange_ = Vector2(0.05, 0.90);
    desc.shapePower_ = Vector2(8.0, 1.3);
    desc.width_ = 0.8;
    desc.segmentMaskInfo_ = Vector4(1.0, 10.0, 0.1, 1.0);

    GenerateLeaf(model, desc);
    
    return context.CreateModel(factory);
}

void MainFoliage(ProceduralContext@ context, float scale, float height)
{
    context[0] = Variant(1);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));

    XMLFile@ opaqueRP = cache.GetResource("XMLFile", "RenderPaths/BakeOpaque.xml");
    XMLFile@ transparentRP = cache.GetResource("XMLFile", "RenderPaths/BakeTransparent.xml");
    Material@ maskAddMaterial = cache.GetResource("Material", "Materials/Procedural/MaskAdd.xml");
    Material@ superMaskMaterial = cache.GetResource("Material", "Materials/Procedural/SuperMask.xml");
    Material@ mixColorMaterial = cache.GetResource("Material", "Materials/Procedural/MixColor.xml");
    Model@ quadModel = context.CreateQuadModel();

    Model@ leafMaskModel = MainLeafMask(context);
    Texture2D@ leafMask = context.RenderTexture(
        64, 64, BLACK, opaqueRP, leafMaskModel, maskAddMaterial, Vector3(0.5, 0, 0.5), Vector2(1, 1));

    Model@ foliageMaskModel = MainFoliageMask(context, scale, height);
    Texture2D@ foliageMask = context.RenderTexture(
        512, 1024, BLACK, opaqueRP, foliageMaskModel, superMaskMaterial, Vector3(0.5, 0.5 - height/2, 0.5), Vector2(1, height), Array<Texture2D@> = {leafMask});

    Texture2D@ foliageDiffuse = context.RenderTexture(
        512, 1024, BLACK, transparentRP, quadModel, mixColorMaterial, Vector3(), Vector2(1, 1), Array<Texture2D@> =
        {
            foliageMask,
            context.RenderTexture(Color(0.204, 0.502, 0.09, 1)),
            context.RenderTexture(Color(0.298, 0.769, 0.09, 1)),
            context.RenderTexture(Color(0, 0, 0, 0))
        });

    Image@ foliageDiffuseImage = foliageDiffuse.GetImage();
    foliageDiffuseImage.FillGaps(2);
    foliageDiffuseImage.PrecalculateLevels();
    foliageDiffuseImage.AdjustAlpha(1.18);

    context[3] = Variant(foliageDiffuseImage);
}

void MainLargeFoliage(ProceduralContext@ context)
{
    MainFoliage(context, 1.0, 2);
}

void MainSmallFoliage(ProceduralContext@ context)
{
    MainFoliage(context, 1.0, 1);
}

void MainBark(ProceduralContext@ context)
{
    context[0] = Variant(1);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));

    XMLFile@ transparentRP = cache.GetResource("XMLFile", "RenderPaths/BakeTransparent.xml");
    Material@ perlinNoiseMaterial = cache.GetResource("Material", "Materials/Procedural/PerlinNoise.xml");
    Material@ mixColorMaterial = cache.GetResource("Material", "Materials/Procedural/MixColor.xml");
    Material@ superMixColorMaterial = cache.GetResource("Material", "Materials/Procedural/SuperMixColor.xml");
    Model@ quadModel = context.CreateQuadModel();

    Texture2D@ crackMap = context.GeneratePerlinNoise(256, 1024, BLACK, WHITE, Vector2(4, 8),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0), Vector4(4, 4, 0.25, 0), Vector4(8, 8, 0.125, 0) }, 0, -1, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial).GetTexture2D();

    Texture2D@ tickMap = context.GeneratePerlinNoise(256, 1024, BLACK, WHITE, Vector2(8, 64),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0) }, 0, 2, Vector2(-0.5, 1),
        transparentRP, quadModel, perlinNoiseMaterial).GetTexture2D();
    Texture2D@ whiteColor = context.GeneratePerlinNoise(256, 1024, Color(0.941, 0.941, 0.941), Color(0.816, 0.816, 0.816), Vector2(4, 4),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0) }, 0, 0, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial).GetTexture2D();
    Texture2D@ tickColor = context.RenderTexture(Color(0.518, 0.518, 0.51));
    Texture2D@ white = context.RenderTexture(
        256, 1024, BLACK, transparentRP, quadModel, mixColorMaterial, Vector3(), Vector2(1, 1),
        Array<Texture2D@> = { tickMap, whiteColor, tickColor, whiteColor });

    Texture2D@ dark = context.GeneratePerlinNoise(256, 1024, BLACK, Color(0.126, 0.106, 0.09), Vector2(32, 8),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0), Vector4(4, 4, 0.25, 0) }, 0, -1, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial).GetTexture2D();

    Texture2D@ noise = context.GeneratePerlinNoise(256, 1024, BLACK, WHITE, Vector2(32, 16),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0), Vector4(4, 4, 0.25, 0), Vector4(8, 8, 0.125, 0) }, 0, 0, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial).GetTexture2D();

    Image@ barkDiffuseImage = context.RenderTexture(
        256, 1024, BLACK, transparentRP, quadModel, superMixColorMaterial, Vector3(), Vector2(1, 1),
        Array<Texture2D@> = { crackMap, white, dark, noise }).GetImage();

    barkDiffuseImage.PrecalculateLevels();

    context[3] = Variant(barkDiffuseImage);
}
