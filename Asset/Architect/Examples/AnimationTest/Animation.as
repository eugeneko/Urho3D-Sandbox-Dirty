
class Animator : ScriptObject
{
    bool disableFoot = false;
    
    float _time = 0.0f;
    Vector3 _prevPosition;
    float _idleTimer;
    bool _idleCasted;
    WeightBlender _weightBlender;
    
    void DelayedStart()
    {
        AnimationController@ animController = node.GetComponent("AnimationController");
        
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
        
        _time = 0.0f;
        _prevPosition = node.position;
        _idleTimer = 0;
        _idleCasted = false;
    }
    void Update(float timeStep)
    {
        float fadeTime = 0.333;
        float idleTimeout = 0.666;
        String animIdle = "Swat_Idle.ani";
        String animFwd = "Swat_WalkFwd.ani";
        String animBwd = "Swat_WalkBwd.ani";
        String animLeft = "Swat_WalkLeft.ani";
        String animRight = "Swat_WalkRight.ani";
        
        AnimationController@ animController = node.GetComponent("AnimationController");
        Vector3 velocity = (node.position - _prevPosition) / timeStep;
        _prevPosition = node.position;
        
        float velocityScale = velocity.length / 1.7;
        if (velocity.length < 0.01)
        {
            if (!_idleCasted && _idleTimer <= 0)
            {
                _idleCasted = true;
                _weightBlender.SetWeight(animIdle, 1, fadeTime);
                _weightBlender.SetWeight(animFwd, 0, fadeTime);
                _weightBlender.SetWeight(animBwd, 0, fadeTime);
                _weightBlender.SetWeight(animLeft, 0, fadeTime);
                _weightBlender.SetWeight(animRight, 0, fadeTime);
            }
        }
        else
        {
            Vector4 directionFactors(1, 0, 0, 0);
            directionFactors.x = Max(0.0f, -velocity.z);
            directionFactors.y = Max(0.0f, velocity.z);
            directionFactors.z = Max(0.0f, velocity.x);
            directionFactors.w = Max(0.0f, -velocity.x);
            directionFactors /= directionFactors.x + directionFactors.y + directionFactors.z + directionFactors.w;
            
            _weightBlender.SetWeight(animIdle, 0, fadeTime);
            _weightBlender.SetWeight(animFwd, directionFactors.x, fadeTime);
            _weightBlender.SetWeight(animBwd, directionFactors.y, fadeTime);
            _weightBlender.SetWeight(animLeft, directionFactors.z, fadeTime);
            _weightBlender.SetWeight(animRight, directionFactors.w, fadeTime);
            _idleTimer = idleTimeout;
            _idleCasted = false;
        }
        
        animController.Play(animIdle, 0, true);
        animController.Play(animFwd, 0, true);
        animController.Play(animBwd, 0, true);
        animController.Play(animLeft, 0, true);
        animController.Play(animRight, 0, true);
        animController.SetWeight(animIdle, _weightBlender.GetNormalizedWeight(animIdle));
        animController.SetWeight(animFwd, _weightBlender.GetNormalizedWeight(animFwd));
        animController.SetWeight(animBwd, _weightBlender.GetNormalizedWeight(animBwd));
        animController.SetWeight(animRight, _weightBlender.GetNormalizedWeight(animRight));
        animController.SetWeight(animLeft, _weightBlender.GetNormalizedWeight(animLeft));
            
        animController.SetTime(animFwd, Fract(_time));
        animController.SetTime(animBwd, Fract(_time + 0.5));
        animController.SetTime(animRight, Fract(_time));
        animController.SetTime(animLeft, Fract(_time + 0.5));
        
        _time += timeStep * velocityScale;
        if (_idleTimer > 0) _idleTimer -= timeStep;
        _weightBlender.Update(timeStep);
        /*    

        
        float R = 10;
        float v = 1.7;
        float T = 2 * M_PI * R / v;
        float radius = 5;
        //node.position = node.position + Vector3(Cos(_time / T * 360), 0, Sin(_time / T * 360)) * timeStep * 2 * M_PI * R / T;*/
    }
}
