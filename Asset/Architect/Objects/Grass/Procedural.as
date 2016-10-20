#include "Scripts/ModelFactoryWrapper.as"

class InstanceLinearStripParam
{
    Vector2 cvLocation;
    float cfStep;
    float cfLocationNoise;
    Vector2 cvWidth;
    Vector2 cvHeight;
    float cfAngleNoise;
};

Vector2 StableRandom2(Vector2 uv)
{
    return Vector2(StableRandom(uv), StableRandom(uv + Vector2(1, 3)));
}

Vector4 StableRandom4(Vector2 uv)
{
    return Vector4(StableRandom(uv), StableRandom(uv + Vector2(1, 1)), StableRandom(uv + Vector2(2, 2)), StableRandom(uv + Vector2(3, 3)));
}

void ComputeInstanceLinearStrip(InstanceLinearStripParam sParam,
    Array<Vector2>& positions, Array<float>& rotations, Array<Vector2>& sizes, Array<float>& noises)
{
    // Limit instances
    int iCount = int(1.0 / sParam.cfStep);
    positions.Resize(iCount);
    rotations.Resize(iCount);
    sizes.Resize(iCount);
    noises.Resize(iCount);
    for (int i = 0; i < iCount; ++i)
    {
        // Compute location
        positions[i] = Vector2(Lerp(sParam.cvLocation.x, sParam.cvLocation.y, (i + 0.5) / iCount), 0.0);
        
        // Generate noise
        Vector4 vNoise = StableRandom4(positions[i]);

        // Randomize location
        positions[i].x -= sParam.cfLocationNoise * (vNoise.w * 2.0 - 1.0);
        
        // Compute rotation
        rotations[i] = sParam.cfAngleNoise * (vNoise.x * 2.0 - 1.0);
        
        // Compute size
        sizes[i].x = Lerp(sParam.cvWidth.x, sParam.cvWidth.y, vNoise.x);
        sizes[i].y = Lerp(sParam.cvHeight.x, sParam.cvHeight.y, vNoise.y);
        
        // Compute class
        noises[i] = vNoise.z;
    }
}

Model@ MainGrassMask(ProceduralContext@ context)
{
    ModelFactory@ factory = context.CreateModelFactory();
    ModelFactoryWrapper model(factory);

    InstanceLinearStripParam sParam;
    sParam.cvLocation       = Vector2(0.1, 0.9);
    sParam.cfStep           = 0.02;
    sParam.cfLocationNoise  = 0.02;
    sParam.cvWidth          = Vector2(0.01, 0.01);
    sParam.cvHeight         = Vector2(0.8, 1.0);
    sParam.cfAngleNoise     = 7.0;

    Array<Vector2> positions;
    Array<float> rotations;
    Array<Vector2> sizes;
    Array<float> noises;
    ComputeInstanceLinearStrip(sParam, positions, rotations, sizes, noises);
    
    for (uint i = 0; i < positions.length; ++i)
    {
        model.AddRect2D(Vector3(positions[i], noises[i]), 0.0, sizes[i], Vector2(0, 0), Vector2(1, 1), Vector2(0, 1), Vector4(1.0, 10.0, 1.0, 1.0));
    }

    return context.CreateModel(factory);
}

void MainGrass(ProceduralContext@ context)
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

    Model@ grassMaskModel = MainGrassMask(context);
    Texture2D@ grassMask = context.RenderTexture(
        256, 256, TRANSPARENT, opaqueRP, grassMaskModel, maskAddMaterial, Vector3(0, 0, 0), Vector2(1, 1));

    Texture2D@ grassDiffuse = context.RenderTexture(
        256, 256, BLACK, transparentRP, quadModel, mixColorMaterial, Vector3(), Vector2(1, 1), Array<Texture2D@> =
        {
            grassMask,
            context.RenderTexture(Color(0.2, 0.6, 0.1, 1)),
            context.RenderTexture(Color(0.2, 0.6, 0.1, 1)),
            context.RenderTexture(Color(0, 0, 0, 0))
        });

    Image@ grassDiffuseImage = grassDiffuse.GetImage();
    grassDiffuseImage.FillGaps(2);
    grassDiffuseImage.PrecalculateLevels();
    grassDiffuseImage.AdjustAlpha(1.18);

    context[3] = Variant(grassDiffuseImage);
}
