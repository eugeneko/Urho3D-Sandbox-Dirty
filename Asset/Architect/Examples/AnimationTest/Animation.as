class DirectionBlender
{
    void set_threshold(float threshold) { threshold_ = threshold; }
    void set_speed(float speed) { speed_ = speed; }
    void set_targetDirection(Vector2 targetDirection) { targetDirection_ = targetDirection; }
    Vector2 get_direction() { return direction_; }

    void Update(float timeStep)
    {
        bool forceFast = direction_.length < epsilon_ || targetDirection_.length < epsilon_;
        bool fastSwitch = forceFast || direction_.Angle(targetDirection_) <= threshold_;
        if (fastSwitch)
            move(targetDirection_, timeStep);
        else
        {
            float remaining = move(Vector2(0, 0), timeStep);
            if (remaining > 0)
                move(targetDirection_, remaining);
        }
    }
    
    private float move(Vector2 target, float timeStep)
    {
        Vector2 delta = target - direction_;
        if (delta.length < epsilon_)
            return timeStep;
        direction_ += delta.Normalized() * Min(speed_ * timeStep, delta.length);
        return Max(0.0, timeStep - delta.length / speed_);
    }

    float epsilon_ = 0.0001;
    float threshold_ = 1;
    float speed_ = 1;
    Vector2 direction_ = Vector2(0, 0);
    Vector2 targetDirection_ = Vector2(0, 0);
};


class Animator : ScriptObject
{
    bool disableFoot = false;
    
    float _time = 0.0f;
    Vector3 _prevPosition;
    float _idleTimer;
    bool _idleCasted;
    WeightBlender _weightBlender;
    DirectionBlender _directionBlender;
    
    void DelayedStart()
    {
        AnimationController@ animController = node.GetComponent("AnimationController");
        AnimatedModel@ animModel = node.GetComponent("AnimatedModel");
        
        if (disableFoot)
        {
            animModel.skeleton.GetBone("swat:LeftUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftFoot").animated = false;
            animModel.skeleton.GetBone("swat:RightUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightFoot").animated = false;
        }
        
        animModel.skeleton.GetBone("swat:LeftShoulder").animated = false;
        animModel.skeleton.GetBone("swat:LeftArm").animated = false;
        animModel.skeleton.GetBone("swat:LeftForeArm").animated = false;
        animModel.skeleton.GetBone("swat:RightShoulder").animated = false;
        animModel.skeleton.GetBone("swat:RightArm").animated = false;
        animModel.skeleton.GetBone("swat:RightForeArm").animated = false;
        
        _time = 0.0f;
        _prevPosition = node.position;
        _idleTimer = 0;
        _idleCasted = false;
        
        _directionBlender.threshold = 120;
        _directionBlender.speed = 2;
        _directionBlender.targetDirection = Vector2(0, 0);
    }

    void Update(float timeStep)
    {
        UpdateController(timeStep);
        ApplyMovement(timeStep);
    }

    void UpdateController(float timeStep)
    {
        float fadeTime = 0.5;
        float idleTimeout = 0.5;
        String animIdle = "Swat_Idle.ani";
        String animFwd = "Swat_WalkFwd.ani";
        String animBwd = "Swat_WalkBwd.ani";
        String animLeft = "Swat_WalkLeft.ani";
        String animRight = "Swat_WalkRight.ani";
        
        AnimationController@ animController = node.GetComponent("AnimationController");
        Vector3 velocity = (node.position - _prevPosition) / timeStep;
        _prevPosition = node.position;
        
        animController.Play(animIdle, 0, true);
        animController.Play(animFwd, 0, true);
        animController.Play(animBwd, 0, true);
        animController.Play(animLeft, 0, true);
        animController.Play(animRight, 0, true);
        
        Vector3 scaledVelocity = velocity / 1.7;
        if ((scaledVelocity * Vector3(1, 0, 1)).length < 0.01)
        {
            if (!_idleCasted && _idleTimer <= 0)
            {
                _idleCasted = true;
                _directionBlender.targetDirection = Vector2(0, 0);
            }
        }
        else
        {
            Vector2 direction = Vector2(scaledVelocity.x, -scaledVelocity.z) / (Abs(scaledVelocity.x) + Abs(scaledVelocity.z));
            _directionBlender.targetDirection = direction;
            
            _idleTimer = idleTimeout;
            _idleCasted = false;
        }
        
        Vector2 direction = _directionBlender.direction;
        animController.SetWeight(animIdle, 1 - Abs(direction.x) - Abs(direction.y));
        animController.SetWeight(animFwd, Max(0.0, direction.y));
        animController.SetWeight(animBwd, Max(0.0, -direction.y));
        animController.SetWeight(animLeft, Max(0.0, direction.x));
        animController.SetWeight(animRight, Max(0.0, -direction.x));
            
        animController.SetTime(animFwd, Fract(_time));
        animController.SetTime(animBwd, Fract(_time + 0.066666));
        animController.SetTime(animRight, Fract(_time - 0.066666));
        animController.SetTime(animLeft, Fract(_time + 0.1));
        
        _time += timeStep * scaledVelocity.length;
        if (_idleTimer > 0) _idleTimer -= timeStep;
        _directionBlender.Update(timeStep);
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
                _movementDirection = Vector3(Random(-1, 1), 0.0, Random(-1, 1)).Normalized();
            }
            node.position = node.position + _movementDirection * movementVelocity * timeStep;
            _movementRemainingTime -= timeStep;
            break;
        }
        }
    }
}
