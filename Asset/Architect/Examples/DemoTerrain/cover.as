Array<Node@>@ terrainNodes = scene.GetChildrenWithComponent("Terrain");
if (terrainNodes.empty)
    return;
Node@ terrain = terrainNodes[0];

Node@ destNode = scene.GetChild("Forest");
if (destNode !is null)
    destNode.Remove();
destNode = scene.CreateChild("Forest");

destNode.RemoveAllChildren();

XMLFile@ t29 = cache.GetResource("XMLFile", "Examples/DemoTerrain/t29.xml");
XMLFile@ t19 = cache.GetResource("XMLFile", "Examples/DemoTerrain/t19.xml");
XMLFile@ t11 = cache.GetResource("XMLFile", "Examples/DemoTerrain/t11.xml");

TODO_CoverTerrainWithObjects(terrain, destNode, t29, 30.0, 13.0, Vector2(-500, -500), Vector2(500, 500));
TODO_CoverTerrainWithObjects(terrain, destNode, t19, 20.0, 10.0, Vector2(-500, -500), Vector2(500, 500));
TODO_CoverTerrainWithObjects(terrain, destNode, t11, 15.0, 8.0, Vector2(-500, -500), Vector2(500, 500));

log.Info("Success!!");
