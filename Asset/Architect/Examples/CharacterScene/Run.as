Scene@ scene_;

void Start()
{
    scene_ = Scene();
    scene_.LoadXML(cache.GetFile("Examples/CharacterScene/Scene.xml"));

    Camera@ camera = scene_.GetComponent("Camera", true);
    Viewport@ viewport = Viewport(scene_, camera);
    renderer.viewports[0] = viewport;
}

// Create XML patch instructions for screen joystick layout specific to this sample app
String patchInstructions = "";
