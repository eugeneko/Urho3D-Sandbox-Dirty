const int CTRL_FORWARD = 1;
const int CTRL_BACK = 2;
const int CTRL_LEFT = 4;
const int CTRL_RIGHT = 8;
const int CTRL_JUMP = 16;

const float MOVE_FORCE = 0.8f;
const float INAIR_MOVE_FORCE = 0.02f;
const float BRAKE_FORCE = 0.2f;
const float JUMP_FORCE = 7.0f;
const float YAW_SENSITIVITY = 0.1f;
const float INAIR_THRESHOLD_TIME = 0.1f;

class Character : ScriptObject
{
    // Character controls.
    Controls controls;
    // Grounded flag for movement.
    bool onGround = false;
    // Jump flag.
    bool okToJump = true;
    // In air timer. Due to possible physics inaccuracy, character can be off ground for max. 1/10 second and still be allowed to move.
    float inAirTimer = 0.0f;

    void Start()
    {
        SubscribeToEvent(node, "NodeCollision", "HandleNodeCollision");
    }

    void Load(Deserializer& deserializer)
    {
        controls.yaw = deserializer.ReadFloat();
        controls.pitch = deserializer.ReadFloat();
    }

    void Save(Serializer& serializer)
    {
        serializer.WriteFloat(controls.yaw);
        serializer.WriteFloat(controls.pitch);
    }

    void HandleNodeCollision(StringHash eventType, VariantMap& eventData)
    {
        VectorBuffer contacts = eventData["Contacts"].GetBuffer();

        while (!contacts.eof)
        {
            Vector3 contactPosition = contacts.ReadVector3();
            Vector3 contactNormal = contacts.ReadVector3();
            float contactDistance = contacts.ReadFloat();
            float contactImpulse = contacts.ReadFloat();

            // If contact is below node center and pointing up, assume it's a ground contact
            if (contactPosition.y < (node.position.y + 1.0f))
            {
                float level = contactNormal.y;
                if (level > 0.75)
                    onGround = true;
            }
        }
    }

    void FixedUpdate(float timeStep)
    {
        /// \todo Could cache the components for faster access instead of finding them each frame
        RigidBody@ body = node.GetComponent("RigidBody");

        // Update the in air timer. Reset if grounded
        if (!onGround)
            inAirTimer += timeStep;
        else
            inAirTimer = 0.0f;
        // When character has been in air less than 1/10 second, it's still interpreted as being on ground
        bool softGrounded = inAirTimer < INAIR_THRESHOLD_TIME;

        // Update movement & animation
        Quaternion rot = node.rotation;
        Vector3 moveDir(0.0f, 0.0f, 0.0f);
        Vector3 velocity = body.linearVelocity;
        // Velocity on the XZ plane
        Vector3 planeVelocity(velocity.x, 0.0f, velocity.z);

        if (controls.IsDown(CTRL_FORWARD))
            moveDir += Vector3(0.0f, 0.0f, 1.0f);
        if (controls.IsDown(CTRL_BACK))
            moveDir += Vector3(0.0f, 0.0f, -1.0f);
        if (controls.IsDown(CTRL_LEFT))
            moveDir += Vector3(-1.0f, 0.0f, 0.0f);
        if (controls.IsDown(CTRL_RIGHT))
            moveDir += Vector3(1.0f, 0.0f, 0.0f);

        // Normalize move vector so that diagonal strafing is not faster
        if (moveDir.lengthSquared > 0.0f)
            moveDir.Normalize();

        // If in air, allow control, but slower than when on ground
        body.ApplyImpulse(rot * moveDir * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

        if (softGrounded)
        {
            // When on ground, apply a braking force to limit maximum ground velocity
            Vector3 brakeForce = -planeVelocity * BRAKE_FORCE;
            body.ApplyImpulse(brakeForce);

            // Jump. Must release jump control between jumps
            if (controls.IsDown(CTRL_JUMP))
            {
                if (okToJump)
                {
                    body.ApplyImpulse(Vector3(0.0f, 1.0f, 0.0f) * JUMP_FORCE);
                    okToJump = false;
                }
            }
            else
                okToJump = true;
        }

        // Reset grounded flag for next frame
        onGround = false;
    }

    void Update(float timeStep)
    {
        // Clear previous controls
        controls.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using keys (desktop)
        if (ui.focusElement is null)
        {
            controls.Set(CTRL_FORWARD, input.keyDown[KEY_W]);
            controls.Set(CTRL_BACK, input.keyDown[KEY_S]);
            controls.Set(CTRL_LEFT, input.keyDown[KEY_A]);
            controls.Set(CTRL_RIGHT, input.keyDown[KEY_D]);
            controls.Set(CTRL_JUMP, input.keyDown[KEY_SPACE]);

            // Add character yaw & pitch from the mouse motion or touch input
            controls.yaw += input.mouseMoveX * YAW_SENSITIVITY;
            controls.pitch += input.mouseMoveY * YAW_SENSITIVITY;

            // Limit pitch
            controls.pitch = Clamp(controls.pitch, -80.0f, 80.0f);
            // Set rotation already here so that it's updated every rendering frame instead of every physics frame
            node.rotation = Quaternion(controls.yaw, Vector3(0.0f, 1.0f, 0.0f));
            
            Node@ cameraNode = node.GetChild("Camera");
            if (cameraNode !is null)
            {
                cameraNode.rotation = Quaternion(controls.pitch, Vector3(1.0f, 0.0f, 0.0f));
            }
        }
    }

}

