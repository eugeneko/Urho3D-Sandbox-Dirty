class DirectionBlender
{
    void set_switchSpeed(float switchSpeed) { switchSpeed_ = switchSpeed; }
    void set_pendingDirection(Vector2 pendingDirection) { pendingDirection_ = pendingDirection; }
    Vector2 get_pendingDirection() const { return pendingDirection_; }
    
    Vector4 get_weights() const { return getCurrentWeights(); }

    void Update(float timeStep, float timeRate)
    {
        // Start new interpolation
        if (!isInterpolating_ && (firstDirection_ - pendingDirection_).length > epsilon_)
        {
            isInterpolating_ = true;
            interpolateFactor_ = 0;
            secondDirection_ = pendingDirection_;
        }
        
        // Interpolate
        if (isInterpolating_)
        {
            interpolateFactor_ += timeStep * switchSpeed_;
            if (interpolateFactor_ >= 1)
            {
                firstDirection_ = secondDirection_;
                isInterpolating_ = false;
            }
        }
    }
    
    // Return (Forward, Backward, Left, Right) weights.
    private Vector4 getWeights(Vector2 direction) const
    {
        return Vector4(Max(0.0, direction.y), Max(0.0, -direction.y), Max(0.0, direction.x), Max(0.0, -direction.x));
    }
    
    private Vector4 getCurrentWeights() const
    {
        return isInterpolating_ ? (getWeights(firstDirection_) * (1 - interpolateFactor_) +  getWeights(secondDirection_) * interpolateFactor_) : getWeights(firstDirection_);
    }

    float epsilon_ = 0.0001;
    float switchSpeed_ = 1;
    
    Vector2 firstDirection_;
    Vector2 secondDirection_;
    float interpolateFactor_ = -1;
    bool isInterpolating_ = false;
    Vector2 pendingDirection_;
};


class Animator : ScriptObject
{
    bool useCharacterAnimation = false;
    float footRotationAmount = 0;
    float footRotationGlobal = 0.5;
    float footOffset = 0;
    
    Vector3 _prevPosition;
    float _idleTimer;
    bool _idleCasted;
    DirectionBlender _directionBlender;
    
    AnimationController@ GetAnimationController()
    {
        return node.GetComponent(useCharacterAnimation ? "CharacterAnimationController" : "AnimationController");
    }
    void DelayedStart()
    {
        AnimationController@ animController = GetAnimationController();
        AnimatedModel@ animModel = node.GetComponent("AnimatedModel");
        
        if (useCharacterAnimation)
        {
            animModel.skeleton.GetBone("swat:LeftUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftLeg").animated = false;
            animModel.skeleton.GetBone("swat:LeftFoot").animated = false;
            animModel.skeleton.GetBone("swat:RightUpLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightLeg").animated = false;
            animModel.skeleton.GetBone("swat:RightFoot").animated = false;
        }
        
        _prevPosition = node.position;
        _idleTimer = 0;
        _idleCasted = false;
        
        _directionBlender.switchSpeed = 4;
        _directionBlender.pendingDirection = Vector2(0, 0);
    }

    void Update(float timeStep)
    {
        UpdateController(timeStep);
        ApplyMovement(timeStep);
    }
    
    private float computeDistance(float time1, float length1, float time2, float length2)
    {
        float dist1 = time2 - time1;
        float dist2 = time1 < time2 ? time2 - (time1 + length1) : (time2 + length2) - time1;
        return Abs(dist1) < Abs(dist2) ? dist1 : dist2;
    }
    
    private void updateAnimation(AnimationController@ animController, const String &in anim, float weight, float speed, float epsilon)
    {
        if (weight > epsilon)
            animController.Play(anim, 0, true);
        animController.SetSpeed(anim, speed);
        animController.SetWeight(anim, weight);
    }
    
    void UpdateController(float timeStep)
    {
        float idleTimeout = 0.5;
        String animIdle = "Objects/Swat/Idle.ani";
        String animFwd = "Objects/Swat/WalkFwd.ani";
        String animBwd = "Objects/Swat/WalkBwd.ani";
        String animLeft = "Objects/Swat/WalkLeft.ani";
        String animRight = "Objects/Swat/WalkRight.ani";
        
        AnimationController@ animController = GetAnimationController();
        Vector3 velocity = node.rotation.Inverse() * (node.worldPosition - _prevPosition) / timeStep;
        _prevPosition = node.worldPosition;
        
        Vector3 scaledVelocity = velocity / 1.7;
        if ((scaledVelocity * Vector3(1, 0, 1)).length < 0.01)
        {
            if (!_idleCasted && _idleTimer <= 0)
            {
                _idleCasted = true;
                _directionBlender.pendingDirection = Vector2(0, 0);
            }
        }
        else
        {
            Vector2 direction = Vector2(scaledVelocity.x, -scaledVelocity.z) / (Abs(scaledVelocity.x) + Abs(scaledVelocity.z));
            _directionBlender.pendingDirection = direction;
            
            _idleTimer = idleTimeout;
            _idleCasted = false;
        }
                
        if (_idleTimer > 0) _idleTimer -= timeStep;
        _directionBlender.Update(timeStep, scaledVelocity.length);

        Vector4 weight = _directionBlender.weights;
        float weightSum = weight.DotProduct(Vector4(1, 1, 1, 1));
        if (weightSum > 1)
        {
            weight /= weightSum;
            weightSum = 1;
        }

        float weightIdle = 1 - weightSum;
        float weightFwd = weight.x;
        float weightBwd = weight.y;
        float weightLeft = weight.z;
        float weightRight = weight.w;
        
        float epsilon = 0.0001;
        if (weightIdle > epsilon)
            animController.Play(animIdle, 0, true);
        animController.SetWeight(animIdle, weightIdle);
        
        updateAnimation(animController, animFwd, weightFwd, scaledVelocity.length, epsilon);
        updateAnimation(animController, animBwd, weightBwd, scaledVelocity.length, epsilon);
        updateAnimation(animController, animLeft, weightLeft, scaledVelocity.length, epsilon);
        updateAnimation(animController, animRight, weightRight, scaledVelocity.length, epsilon);
        
        // Synchronize mainstream
        float syncFactor = 0.2;
        Vector2 dir = _directionBlender.pendingDirection;
        String mainstreamY = dir.y > 0 ? animFwd : dir.y < 0 ? animBwd : "";
        String mainstreamX = dir.x > 0 ? animLeft : dir.x < 0 ? animRight : "";
        if (!mainstreamY.empty && !mainstreamX.empty && animController.GetWeight(mainstreamY) > 0 && animController.GetWeight(mainstreamX) > 0)
        {
            float timeY = animController.GetTime(mainstreamY);
            float timeX = animController.GetTime(mainstreamX);
            float speedY = animController.GetSpeed(mainstreamY);
            float speedX = animController.GetSpeed(mainstreamX);
            if (timeX != timeY && speedX == speedY)
            {
                float lengthY = animController.GetLength(mainstreamY);
                float lengthX = animController.GetLength(mainstreamX);

                float dist = computeDistance(timeY, lengthY, timeX, lengthX);
                if (dist < 0)
                {
                    float future = speedY * (1 - syncFactor) * timeStep;
                    if (future >= Abs(dist))
                        animController.SetTime(mainstreamY, timeX);
                    else
                        animController.SetSpeed(mainstreamY, speedY * syncFactor);
                }
                else
                {
                    float future = speedX * (1 - syncFactor) * timeStep;
                    if (future >= Abs(dist))
                        animController.SetTime(mainstreamX, timeY);
                    else
                        animController.SetSpeed(mainstreamX, speedX * syncFactor);
                }
            }  
        }
        
        // animController.type == StringHash("CharacterAnimationController")
        if (animController !is null && animController.typeName == "CharacterAnimationController")
        {
            CharacterAnimationController@ characterAnimController = animController;
            Node@ groundControl = node.GetChild("control:Ground");
            if (groundControl !is null)
            {
                //Vector3 
                //Quaternion rotateOffset(Vector3(1, 0, 0), Vector3(direction.x, 0, direction.y));
                Quaternion rotateOffset;
                Matrix3x4 leftOffset(rotateOffset * Vector3(footOffset, 0, 0), Quaternion(), Vector3(1, 1, 1));
                Matrix3x4 rightOffset(rotateOffset * Vector3(-footOffset, 0, 0), Quaternion(), Vector3(1, 1, 1));
                characterAnimController.SetTargetTransform("LeftFoot", groundControl.transform * leftOffset);
                characterAnimController.SetTargetTransform("RightFoot", groundControl.transform * rightOffset);
            }

            characterAnimController.SetTargetRotationAmount("LeftFoot", footRotationAmount);
            characterAnimController.SetTargetRotationAmount("RightFoot", footRotationAmount);
            characterAnimController.SetTargetRotationBalance("LeftFoot", footRotationGlobal);
            characterAnimController.SetTargetRotationBalance("RightFoot", footRotationGlobal);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
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
            Vector3 dir = dirIndex == 0 ? Vector3(-1, 0, 1).Normalized() : Vector3(1, 0, -1).Normalized();
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 4:
        {
            int dirIndex = int(_movementTime / movementScale) % 2;
            Vector3 dir = dirIndex == 0 ? Vector3(-1, 0, -1).Normalized() : Vector3(1, 0, 1).Normalized();
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        case 5:
        {
            if (_movementRemainingTime < 0)
            {
                _movementRemainingTime = Random(3.0, 4.0);
                _movementDirection = Vector3(Random(-1, 1), 0.0, Random(-1, 1)).Normalized();
            }
            node.position = node.position + _movementDirection * movementVelocity * timeStep;
            _movementRemainingTime -= timeStep;
            break;
        }
        case 6:
        {
            int dirIndex = int(_movementTime / movementScale) % 2;
            Vector3 dir = dirIndex == 0 ? Vector3(0, 0, 1) : Vector3(1, 0, 1).Normalized();
            node.position = node.position + dir * movementVelocity * timeStep;
            break;
        }
        }
    }
}
