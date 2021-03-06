#include "Scripts/ModelFactoryWrapper.as"

class ProceduralToolbox
{
    XMLFile@ renderpathOpaque;
    XMLFile@ renderpathTransparent;
    Material@ maskAdd;
    Material@ superMaskRaw;
    Material@ mixColor;
    Material@ paintMaskRaw;
    Model@ quad;
    
    ProceduralToolbox(ProceduralContext@ context)
    {
        renderpathOpaque = cache.GetResource("XMLFile", "RenderPaths/BakeOpaque.xml");
        renderpathTransparent = cache.GetResource("XMLFile", "RenderPaths/BakeTransparent.xml");
        
        maskAdd = cache.GetResource("Material", "Materials/Procedural/MaskAdd.xml");
        superMaskRaw = cache.GetResource("Material", "Materials/Procedural/SuperMaskRaw.xml");
        mixColor = cache.GetResource("Material", "Materials/Procedural/MixColor.xml");
        paintMaskRaw = cache.GetResource("Material", "Materials/Procedural/PaintMaskRaw.xml");
        
        quad = context.CreateQuadModel();
    }
}

QuadList@ MainFoliageMask(ProceduralContext@ context, float scale, float height)
{
    Vector2 leafSize = Vector2(1, 1) * 0.03 / scale;
    Vector2 stripRange(0.9, height);
    Vector2 stripStride = Vector2(0.02, 0.03) / scale;
    Vector2 leafNoise = Vector2(0.05, 0.05);
    Vector2 filterRange(1, height);
    float filterRadius = 0.5;
    Vector2 filterFade(0.1, 0.5);

    QuadList@ rawStrips = GenerateQuadStrips(leafSize, stripRange, stripStride, leafNoise);
    QuadList@ roundStrips = FilterRoundQuadArea(rawStrips, filterRange, filterRadius, filterFade);
    QuadList@ noiseStrips = height > 1 ? FilterNoiseGrad(roundStrips, Vector2(0, 0), Vector2(0, -height)) : roundStrips;
    return noiseStrips;
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

QuadList@ GenerateLeaf(LeafDesc desc)
{
    QuadList@ dest = QuadList();

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

        dest.AddQuad(Vector3(position - Vector2(0, 0.5), 0.5), +angle, scale, Vector2(-0.5, 1.0 - len), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);
        dest.AddQuad(Vector3(position - Vector2(0, 0.5), 0.5), -angle, scale, Vector2(-0.5, 1.0 - len), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);
    }

    Vector2 trunkScale = Vector2(desc.segmentWidth_ / desc.numSegments_, Lerp(1.0, desc.positionRange_.y, 0.5) - desc.positionRange_.x);
    dest.AddQuad(Vector3(0.0, desc.positionRange_.x - 0.5, 0.5), 0.0, trunkScale, Vector2(-0.5, 0.0), Vector2(0.5, 1.0), Vector2(0, 1), desc.segmentMaskInfo_);

    return dest;
}

void MainLeaf(ProceduralContext@ context)
{
    context[0] = Variant(1);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));
    
    ProceduralToolbox toolbox(context);
    
    LeafDesc desc;
    desc.numSegments_ = 30;
    desc.segmentWidth_ = 2.5;
    desc.angleRange_ = Vector2(80.0, 20.0);
    desc.positionRange_ = Vector2(0.05, 0.90);
    desc.shapePower_ = Vector2(8.0, 1.3);
    desc.width_ = 0.8;
    desc.segmentMaskInfo_ = Vector4(1.0, 10.0, 0.1, 1.0);

    Model@ maskModel = CreateModel(context, FlipVertical(GenerateLeaf(desc)));
    Image@ leafMask = context.RenderTexture(
        64, 64, TRANSPARENT, toolbox.renderpathTransparent, maskModel, toolbox.maskAdd, Vector3(0.5, 0.5, 0.5), Vector2(1, 1)).GetImage();

    context[3] = Variant(leafMask);
}

void MainFoliage3m(ProceduralContext@ context)
{
    context[0] = Variant(2);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));
}

Model@ MainLeafMask(ProceduralContext@ context)
{
    LeafDesc desc;
    desc.numSegments_ = 30;
    desc.segmentWidth_ = 2.5;
    desc.angleRange_ = Vector2(80.0, 20.0);
    desc.positionRange_ = Vector2(0.05, 0.90);
    desc.shapePower_ = Vector2(8.0, 1.3);
    desc.width_ = 0.8;
    desc.segmentMaskInfo_ = Vector4(1.0, 10.0, 0.1, 1.0);

    QuadList@ quadList = GenerateLeaf(desc);
    ModelFactory@ factory = context.CreateModelFactory();
    ModelFactoryWrapper model(factory);
    quadList.Deploy(model);

    return context.CreateModel(factory);
}

void MainFoliage(ProceduralContext@ context, float scale, float height)
{
    Vector4 color1 = context[0].GetVector4();
    Vector4 color2 = context[1].GetVector4();

    context[0] = Variant(2);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));

    XMLFile@ opaqueRP = cache.GetResource("XMLFile", "RenderPaths/BakeOpaque.xml");
    XMLFile@ transparentRP = cache.GetResource("XMLFile", "RenderPaths/BakeTransparent.xml");
    Material@ maskAddMaterial = cache.GetResource("Material", "Materials/Procedural/MaskAdd.xml");
    Material@ superMaskRawMaterial = cache.GetResource("Material", "Materials/Procedural/SuperMaskRaw.xml");
    Material@ mixColorMaterial = cache.GetResource("Material", "Materials/Procedural/MixColor.xml");
    Material@ paintMaskRawMaterial = cache.GetResource("Material", "Materials/Procedural/PaintMaskRaw.xml");
    Model@ quadModel = context.CreateQuadModel();

    Model@ leafMaskModel = MainLeafMask(context);
    Texture2D@ leafMask = context.RenderTexture(
        64, 64, BLACK, opaqueRP, leafMaskModel, maskAddMaterial, Vector3(0.5, 0, 0.5), Vector2(1, 1));
        
    QuadList@ foliageData = MainFoliageMask(context, scale, height);

    Texture2D@ foliageMask = context.RenderTexture(
        512, 1024, BLACK, opaqueRP,
        CreateModel(context, FillRandomFactor(foliageData, 0.0)),
        superMaskRawMaterial,
        Vector3(0.5, 0.5, 0.5), Vector2(1, height),
        Array<Texture2D@> = {leafMask});

    Image@ foliageNormal = context.RenderTexture(
        512, 1024, BLACK, opaqueRP,
        CreateModel(context, FillRandomNormal(foliageData, 0.0, Vector2(1, 1) * 0.5)),
        paintMaskRawMaterial,
        Vector3(0.5, 0.5, 0.5), Vector2(1, height),
        Array<Texture2D@> = {leafMask}).GetImage();

    Image@ foliageDiffuse = context.RenderTexture(
        512, 1024, BLACK, transparentRP,
        quadModel,
        mixColorMaterial,
        Vector3(), Vector2(1, 1),
        Array<Texture2D@> =
        {
            foliageMask,
            context.RenderTexture(Color(color1.x, color1.y, color1.z, color1.w)),
            context.RenderTexture(Color(color2.x, color2.y, color2.z, color2.w)),
            context.RenderTexture(Color(0, 0, 0, 0))
        }).GetImage();

    foliageDiffuse.FillGaps(2);
    foliageDiffuse.PrecalculateLevels();
    foliageDiffuse.AdjustAlpha(1.07);
    
    foliageNormal.BuildNormalMapAlpha();
    foliageNormal.FillGaps(2);
    foliageNormal.PrecalculateLevels();

    context[3] = Variant(foliageDiffuse);
    context[4] = Variant(foliageNormal);
    
    Material@ perlinNoiseMaterial = cache.GetResource("Material", "Materials/Procedural/PerlinNoise.xml");
    Image@ noise = context.GeneratePerlinNoise(64, 128, BLACK, WHITE, Vector2(8, 2),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 0.5, 0) }, 0, 0, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial);
    //context[3] = Variant(noise);
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
