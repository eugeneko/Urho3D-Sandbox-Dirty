Model@ model = cache.GetResource("Model", "Objects/Swat/Swat.mdl");
CharacterSkeleton@ skeleton = cache.GetResource("CharacterSkeleton", "Objects/Swat/Skeleton.xml");
ImportCharacterAnimation("Objects/Swat/Idle.ani", skeleton, model);
ImportCharacterAnimation("Objects/Swat/WalkZero.ani", skeleton, model);
ImportCharacterAnimation("Objects/Swat/WalkFwd.ani", skeleton, model);
ImportCharacterAnimation("Objects/Swat/WalkBwd.ani", skeleton, model);
ImportCharacterAnimation("Objects/Swat/WalkLeft.ani", skeleton, model);
ImportCharacterAnimation("Objects/Swat/WalkRight.ani", skeleton, model);
