# Urho3D-Sandbox
Non-organized storage of Urho3D-related stuff

You may pick any piece of it if you find it useful. Distributed under WTFPL.

[WIP] I am going to organize and prepare ready-to-use libraries for some parts of this Sandbox.

You may find here:
- Concept of procedural content pipeline;
- **Prototype of vegetation generator**;
- Extended StaticModel that supports **per-instance materials, wind and smooth LOD switch**;
  + Corresponding HLSL shader;
- Possion random generator;
- Non-finished grass renderer;
- My experiments with animation.

Build Steps (I don't promise that they are correct):
1) Build CMake project, C++11 and DX11 shall be used;
2) Create folder `Asset/Cache`;
3) Run Urho3D Editor via built launcher with command line like this: `-pp Asset -p Data;CoreData;Architect;Cache`
4) Open Asset/Architect/Examples/ProceduralSandbox.xml scene and pray that it will work without errors;
5) ...
6) Play in sandbox
