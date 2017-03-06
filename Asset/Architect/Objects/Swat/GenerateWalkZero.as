Model@ model = cache.GetResource("Model", "Objects/Swat/Swat.mdl");
CharacterSkeleton@ skeleton = cache.GetResource("CharacterSkeleton", "Objects/Swat/Skeleton.xml");
Animation@ walkFwd = cache.GetResource("Animation", "Objects/Swat/WalkFwd.ani");
Animation@ walkBwd = cache.GetResource("Animation", "Objects/Swat/WalkBwd.ani");
Animation@ walkLeft = cache.GetResource("Animation", "Objects/Swat/WalkLeft.ani");
Animation@ walkRight = cache.GetResource("Animation", "Objects/Swat/WalkRight.ani");
Animation@ walkZero = BlendAnimations(model, skeleton,
    Array<Animation@> = { walkFwd, walkBwd, walkLeft, walkRight },
    Array<float> = {0.25, 0.25, 0.25, 0.25},
    Array<float> = {0, 0, 0, 0},
    Array<float> = {0});
walkZero.Save("D:/Root/Repos/FlexEngine/Asset/Cache/Objects/Swat/WalkZero.ani");
