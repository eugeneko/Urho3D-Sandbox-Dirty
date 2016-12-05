
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

    WeightBlender _directionBlender;
    float _timeY = 0;
    float _timeX = 0;
    void Update(float timeStep)
    {
        //Update2(timeStep);
        Update1(timeStep);

        ApplyMovement(timeStep);
    }
    
    void Update2(float timeStep)
    {
        float rotationTime = 0.5;
        float fadeTime = 0.5;
        float idleTimeout = 1;
        String animIdle = "Swat_Idle.ani";
        String animFwd = "Swat_WalkFwd.ani";
        String animBwd = "Swat_WalkBwd.ani";
        String animLeft = "Swat_WalkLeft.ani";
        String animRight = "Swat_WalkRight.ani";
        
        AnimationController@ animController = node.GetComponent("AnimationController");
        Vector3 velocity = (node.position - _prevPosition) / timeStep;
        _prevPosition = node.position;
        
        Vector3 velocityScaled = velocity / 1.7;
        
        if (velocity.length > 0.01)
        {
            float sideness = Abs(velocity.x) / (Abs(velocity.x) + Abs(velocity.z));
            _directionBlender.SetWeight("Direct", 1 - sideness, rotationTime);
            _directionBlender.SetWeight("Side", sideness, rotationTime);
        }

        float scalarVelocity = velocityScaled.length;
        
        float minSync = 0.1;
        float syncX = velocityScaled.x > 0 ? 1.0 - Fract(_timeX) : Fract(_timeX);
        if (syncX < minSync) syncX += 1;
        float syncY = velocityScaled.z > 0 ? 1.0 - Fract(_timeY) : Fract(_timeY);
        if (syncY < minSync) syncY += 1;
        
        Print(_timeX + "/" + _timeY + "/" + syncX + "/" + syncY);
        _timeX += timeStep * (velocityScaled.x > 0 ? 1.0 : -1.0) * scalarVelocity * syncX / syncY;
        _timeY += timeStep * (velocityScaled.z < 0 ? 1.0 : -1.0) * scalarVelocity;
        if (_idleTimer > 0) _idleTimer -= timeStep;
        _weightBlender.Update(timeStep);
        
        animController.Play(animFwd, 0, true);
        animController.SetTime(animFwd, Fract(_timeY));
        animController.SetWeight(animFwd, _directionBlender.GetNormalizedWeight("Direct"));

        animController.Play(animLeft, 0, true);
        animController.SetTime(animLeft, Fract(_timeX));
        animController.SetWeight(animLeft, _directionBlender.GetNormalizedWeight("Side"));

        _directionBlender.Update(timeStep);
    }

    void Update1(float timeStep)
    {
        float fadeTime = 0.5;
        float idleTimeout = 1;
        String animIdle = "Swat_Idle.ani";
        String animFwd = "Swat_WalkFwd.ani";
        String animBwd = "Swat_WalkBwd.ani";
        String animLeft = "Swat_WalkLeft.ani";
        String animRight = "Swat_WalkRight.ani";
        
        AnimationController@ animController = node.GetComponent("AnimationController");
        Vector3 velocity = (node.position - _prevPosition) / timeStep;
        _prevPosition = node.position;
        
        float velocityScale = velocity.length / 1.7;
        //if (_weightBlender.GetNormalizedWeight(animIdle) == 1.0)
        //{
        //    _time = 0;
        //}
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
        animController.SetTime(animBwd, Fract(_time + 0.066666));
        animController.SetTime(animRight, Fract(_time - 0.066666));
        animController.SetTime(animLeft, Fract(_time + 0.1));
        
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

    int movementType = 0;
    float movementScale = 5.0;
    float movementVelocity = 1.0;
    
    float _movementTime = 0;
    float _movementRemainingTime = 0;
    Vector3 _movementDirection;
    void ApplyMovement(float timeStep)
    {
        _movementTime += timeStep;
        switch (movementType)
        {
        case 1:
        {
            int dirIndex = int(_movementTime / movementScale) % 4;
            Vector3 dir = dirIndex == 0 ? Vector3(1, 0, 0) : dirIndex == 1 ? Vector3(0, 0, 1) : dirIndex == 2 ? Vector3(-1, 0, 0) : Vector3(0, 0, -1);
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 2:
        {
            int dirIndex = int(_movementTime / movementScale) % 2;
            Vector3 dir = dirIndex == 0 ? Vector3(0, 0, 1) : Vector3(0, 0, -1);
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 3:
        {
            int dirIndex = int(_movementTime / movementScale) % 2;
            Vector3 dir = dirIndex == 0 ? Vector3(1, 0, 1) : Vector3(-1, 0, -1);
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 4:
        {
            int dirIndex = int(_movementTime / movementScale) % 2;
            Vector3 dir = dirIndex == 0 ? Vector3(-1, 0, 1) : Vector3(1, 0, -1);
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 5:
        {
            if (_movementRemainingTime < 0)
            {
                _movementRemainingTime = Random(1.0, 5.0);
                _movementDirection = Vector3(Random(-1.0, 1.0), 0.0, Random(-1.0, 1.0)).Normalized();
            }
            node.position = node.position + _movementDirection * movementVelocity * timeStep;
            _movementRemainingTime -= timeStep;
            break;
        }
        }
    }
}
