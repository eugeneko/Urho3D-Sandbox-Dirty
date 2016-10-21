Array<Node@>@ terrainNodes = scene.GetChildrenWithComponent("Terrain");
if (terrainNodes.empty)
    return;
Node@ terrain = terrainNodes[0];

Node@ destNode = scene.GetChild("Forest");
if (destNode !is null)
    destNode.Remove();
destNode = scene.CreateChild("Forest");

destNode.RemoveAllChildren();

XMLFile@ t29 = cache.GetResource("XMLFile", "Examples/DemoScene/Tree29.xml");
XMLFile@ t19 = cache.GetResource("XMLFile", "Examples/DemoScene/Tree19.xml");
XMLFile@ t11 = cache.GetResource("XMLFile", "Examples/DemoScene/Tree11.xml");
XMLFile@ t5  = cache.GetResource("XMLFile", "Examples/DemoScene/Tree5.xml");

float size = 200;
TODO_CoverTerrainWithObjects(terrain, destNode, t29, 35.0, 13.0, Vector2(-size, -size), Vector2(size, size));
TODO_CoverTerrainWithObjects(terrain, destNode, t19, 23.0, 10.0, Vector2(-size, -size), Vector2(size, size));
TODO_CoverTerrainWithObjects(terrain, destNode, t11, 17.0, 8.0, Vector2(-size, -size), Vector2(size, size));
TODO_CoverTerrainWithObjects(terrain, destNode, t5,  8.0, 4.0, Vector2(-size, -size), Vector2(size, size));

log.Info("Success!!");
