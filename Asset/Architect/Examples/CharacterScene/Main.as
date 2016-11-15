class Main : ScriptObject
{
    Array<Viewport@> _viewports;
    void DelayedStart()
    {
        Print("Start");
        _viewports.Resize(renderer.numViewports);
        for (uint i = 0; i < _viewports.length; ++i)
            _viewports[i] = renderer.viewports[i];
        
        RenderPath@ renderPath = renderer.viewports[0].renderPath;
        Node@ characterNode = scene.GetChild("Character");
        Node@ cameraNode = characterNode.GetChild("Camera");
        //Quaternion worldRotation = renderer.viewports[0].camera.node.worldRotation;
        characterNode.worldPosition = renderer.viewports[0].camera.node.worldPosition;
        //characterNode.rotation = Quaternion(worldRotation.yaw, Vector3(0.0f, 1.0f, 0.0f));
        //cameraNode.rotation = Quaternion(worldRotation.pitch, Vector3(1.0f, 0.0f, 0.0f));
        Camera@ camera = cast<Camera@>(cameraNode.GetComponent("Camera"));
        IntRect viewRect(0, 0, graphics.width - 1, graphics.height - 1);
        renderer.numViewports = 1;
        renderer.viewports[0] = Viewport(scene, camera, viewRect, renderPath);
    }
    void Update(float timeStep)
    {
        Vector3 cameraWorldPosition = renderer.viewports[0].camera.node.worldPosition;
        Quaternion cameraWorldRotation = renderer.viewports[0].camera.node.worldRotation;
        _viewports[0].camera.node.worldPosition = cameraWorldPosition;
        _viewports[0].camera.node.worldRotation = cameraWorldRotation;
    }
    void Stop()
    {
        Print("Stop");
        renderer.numViewports = _viewports.length;
        for (uint i = 0; i < _viewports.length; ++i)
            renderer.viewports[i] = _viewports[i];
    }
}
