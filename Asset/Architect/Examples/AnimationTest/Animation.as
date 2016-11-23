class Animator : ScriptObject
{
    bool disableFoot = false;
    void DelayedStart()
    {
        AnimationController@ animController = node.GetComponent("AnimationController");
        animController.Play("Swat_WalkFwd.ani", 0, true, 0.2f);
        animController.SetSpeed("Swat_WalkFwd.ani", 1);
        animController.SetWeight("Swat_WalkFwd.ani", 1);
        animController.Play("Swat_WalkBwd.ani", 0, true, 0.2f);
        animController.SetSpeed("Swat_WalkBwd.ani", 1);
        animController.SetWeight("Swat_WalkBwd.ani", 0.00001);
        animController.Play("Swat_WalkLeft.ani", 0, true, 0.2f);
        animController.SetSpeed("Swat_WalkLeft.ani", 1);
        animController.SetWeight("Swat_WalkLeft.ani", 0.00001);
        animController.Play("Swat_WalkRight.ani", 0, true, 0.2f);
        animController.SetSpeed("Swat_WalkRight.ani", 1);
        animController.SetWeight("Swat_WalkRight.ani", 0.00001);
        
        if (disableFoot)
        {
            AnimatedModel@ animModel = node.GetComponent("AnimatedModel");
            animModel.skeleton.GetBone("swat:LeftUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftFoot").animated = false;
            animModel.skeleton.GetBone("swat:RightUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightFoot").animated = false;
        }
    }
    void Update(float timeStep)
    {
    }
}
