class Animator : ScriptObject
{
    bool disableFoot = false;
    void DelayedStart()
    {
        AnimationController@ animController = node.GetComponent("AnimationController");
        animController.Play("Swat_WalkFwd.ani", 0, true, 0.2f);
        animController.SetSpeed("Swat_WalkFwd.ani", 1);
        
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
