void MainNoise(ProceduralContext@ context)
{
    context[0] = Variant(1);
    context[1] = Variant(StringHash("Texture2D"));
    context[2] = Variant(StringHash("Image"));

    XMLFile@ transparentRP = cache.GetResource("XMLFile", "RenderPaths/BakeTransparent.xml");
    Material@ perlinNoiseMaterial = cache.GetResource("Material", "Materials/Procedural/PerlinNoise.xml");
    Model@ quadModel = context.CreateQuadModel();

    Image@ noise = context.GeneratePerlinNoise(1024, 1024, GRAY, WHITE, Vector2(128, 128),
        Array<Vector4> = { Vector4(1, 1, 1, 0), Vector4(2, 2, 1, 0), Vector4(4, 4, 1, 0), Vector4(8, 8, 1, 0) },
        0, 0, Vector2(0, 1),
        transparentRP, quadModel, perlinNoiseMaterial);

    noise.PrecalculateLevels();

    context[3] = Variant(noise);

}
