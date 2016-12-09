Model@ model = cache.GetResource("Model", "Swat.mdl");
Animation@ walkFwd = cache.GetResource("Animation", "Swat_WalkFwd.ani");
Animation@ walkBwd = cache.GetResource("Animation", "Swat_WalkBwd.ani");
Animation@ walkLeft = cache.GetResource("Animation", "Swat_WalkLeft.ani");
Animation@ walkRight = cache.GetResource("Animation", "Swat_WalkRight.ani");
Animation@ walkZero = BlendAnimations(model,
    Array<Animation@> = { walkFwd, walkBwd, walkLeft, walkRight },
    Array<float> = {0.25, 0.25, 0.25, 0.25},
    Array<float> = {0, 0, 0, 0},
    Array<float> = {0});
walkZero.Save("D:/Root/Repos/FlexEngine/Asset/Cache/Swat_WalkZero.ani");
