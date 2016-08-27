const uint NUM_OBJECTS = 1000;
for (uint i = 0; i < NUM_OBJECTS; ++i)
{
    Node@ treeNode = scene.CreateChild("Tree");
    treeNode.position = Vector3(Random(1000.0f) - 500.0f, 0.0f, Random(1000.0f) - 500.0f);
    treeNode.rotation = Quaternion(0.0f, Random(360.0f), 0.0f);
    StaticModel@ treeObject = treeNode.CreateComponent("StaticModel");
    treeObject.model = cache.GetResource("Model", "proxy.mdl");
    treeObject.material = cache.GetResource("Material", "proxy.xml");
}
