#include <FlexEngine/AngelScript/ScriptAPI.h>

#include <FlexEngine/Animation/FootAnimation.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/ScriptedResource.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Math/PoissonRandom.h>
#include <FlexEngine/Math/WeightBlender.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/AngelScript/APITemplates.h>
#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

namespace
{

void FakeAddRef(void* ptr)
{
}

void FakeReleaseRef(void* ptr)
{
}

void DefaultVertex_Contruct(DefaultVertex* ptr)
{
    new(ptr) DefaultVertex();
}

Vector4 DefaultVertex_GetUV(unsigned idx, DefaultVertex* ptr)
{
    if (idx >= MAX_VERTEX_TEXCOORD) return Vector4::ZERO;
    return ptr->uv_[idx];
}

void DefaultVertex_SetUV(unsigned idx, Vector4 value, DefaultVertex* ptr)
{
    if (idx >= MAX_VERTEX_TEXCOORD) return;
    ptr->uv_[idx] = value;
}

void ScriptContext_SetItem(unsigned idx, const Variant& value, ScriptContext* ptr)
{
    if (ptr->items_.Size() <= idx)
        ptr->items_.Resize(idx + 1);
    ptr->items_[idx] = value;
    if (value.GetType() == VAR_PTR)
        ptr->objects_.Push(SharedPtr<RefCounted>(value.GetPtr()));
}

const Variant& ScriptContext_GetItem(unsigned idx, ScriptContext* ptr)
{
    if (ptr->items_.Size() <= idx)
        return Variant::EMPTY;
    return ptr->items_[idx];
}

void ScriptContext_ClearItems(ScriptContext* ptr)
{
    ptr->items_.Clear();
}

ModelFactory* ProceduralContext_CreateModelFactory(ScriptContext* ptr)
{
    SharedPtr<ModelFactory> modelFactory = MakeShared<ModelFactory>(ptr->context_);
    modelFactory->Initialize(DefaultVertex::GetVertexElements(), true);
    return modelFactory.Detach();
}

Model* ProceduralContext_CreateQuadModel(ScriptContext* ptr)
{
    return CreateQuadModel(ptr->context_).Detach();
}

Model* ProceduralContext_CreateModel(ModelFactory* modelFactory, ScriptContext* ptr)
{
    return modelFactory ? modelFactory->BuildModel().Detach() : nullptr;
}

Texture2D* ProceduralContext_RenderTextureBase(unsigned width, unsigned height, const Color& color,
    XMLFile* renderPath, Model* model, const Vector<Material*>& materials, const Vector3& modelPosition, const Vector2& size,
    const Vector<Texture2D*>& inputTextures, const Vector<Vector4>& inputParameters, ScriptContext* ptr)
{
    TextureDescription desc;
    desc.renderPath_ = renderPath;
    desc.color_ = color;
    desc.width_ = Max(1u, width);
    desc.height_ = Max(1u, height);

    // Setup input geometry
    if (model)
    {
        GeometryDescription geometryDesc;
        geometryDesc.model_ = model;
        for (Material* material : materials)
            geometryDesc.materials_.Push(SharedPtr<Material>(material));

        desc.geometries_.Push(geometryDesc);
    }

    // Setup input cameras
    OrthoCameraDescription camera = OrthoCameraDescription::Identity(desc.width_, desc.height_, -modelPosition);
    camera.size_ = size;
    desc.cameras_.Push(camera);

    // Setup input textures
    TextureMap inputMap;
    static const unsigned MAX_INPUT_TEXTURES = 4;
    static const TextureUnit units[MAX_INPUT_TEXTURES] = { TU_DIFFUSE, TU_NORMAL, TU_SPECULAR, TU_EMISSIVE };
    static const char* names[MAX_INPUT_TEXTURES] = { "Unit0", "Unit1", "Unit2", "Unit3" };
    for (unsigned i = 0; i < Min(MAX_INPUT_TEXTURES, inputTextures.Size()); ++i)
    {
        if (inputTextures[i])
        {
            desc.textures_.Populate(units[i], names[i]);
            inputMap.Populate(names[i], SharedPtr<Texture2D>(inputTextures[i]));
        }
    }

    // Setup input parameters
    for (unsigned i = 0; i < Min(MAX_INPUT_UNIFORM_PARAMETERS, inputParameters.Size()); ++i)
    {
        desc.parameters_.Populate(inputParameterUniform[i], inputParameters[i]);
    }

    return RenderTexture(ptr->context_, desc, inputMap).Detach();
}

Texture2D* ProceduralContext_RenderTexture0(unsigned width, unsigned height, const Color& color,
    XMLFile* renderPath, Model* model, CScriptArray* materials, const Vector3& modelPosition, const Vector2& size,
    CScriptArray* inputTextures, CScriptArray* inputParameters, ScriptContext* ptr)
{
    return ProceduralContext_RenderTextureBase(width, height, color, renderPath, model, ArrayToVector<Material*>(materials),
        modelPosition, size, ArrayToVector<Texture2D*>(inputTextures), ArrayToVector<Vector4>(inputParameters), ptr);
}

Texture2D* ProceduralContext_RenderTexture1(unsigned width, unsigned height, const Color& color,
    XMLFile* renderPath, Model* model, Material* material, const Vector3& modelPosition, const Vector2& size,
    CScriptArray* inputTextures, const Vector4& inputParameter, ScriptContext* ptr)
{
    return ProceduralContext_RenderTextureBase(width, height, color, renderPath, model, { material },
        modelPosition, size, ArrayToVector<Texture2D*>(inputTextures), { inputParameter }, ptr);
}

Texture2D* ProceduralContext_RenderTexture2(unsigned width, unsigned height, const Color& color,
    XMLFile* renderPath, Model* model, Material* material, const Vector3& modelPosition, const Vector2& size,
    const Vector4& inputParameter, ScriptContext* ptr)
{
    return ProceduralContext_RenderTextureBase(width, height, color, renderPath, model, { material },
        modelPosition, size, { }, { inputParameter }, ptr);
}

Texture2D* ProceduralContext_RenderTexture3(const Color& color, unsigned width, unsigned height, ScriptContext* ptr)
{
    return ProceduralContext_RenderTextureBase(width, height, color, nullptr, nullptr, { }, Vector3::ZERO, Vector2::ZERO, { }, { }, ptr);
}

Image* ProceduralContext_GeneratePerlinNoise(unsigned width, unsigned height, const Color& firstColor, const Color& secondColor,
    const Vector2& baseScale, CScriptArray* octaves, float bias, float contrast, const Vector2& range,
    XMLFile* renderPath, Model* model, Material* material)
{
    PODVector<Vector4> octavesUnwrapped = ArrayToPODVector<Vector4>(octaves);
    for (Vector4& octave : octavesUnwrapped)
    {
        octave.x_ *= baseScale.x_;
        octave.y_ *= baseScale.y_;
    }
    return GeneratePerlinNoise(renderPath, model, material, width, height, firstColor, secondColor,
        octavesUnwrapped, bias, contrast, range).Detach();
}

void Image_BuildNormalMapAlpha(Image* image)
{
    BuildNormalMapAlpha(SharedPtr<Image>(image));
}

void Image_FillGaps(unsigned downsample, Image* image)
{
    FillImageGaps(SharedPtr<Image>(image), downsample);
}

void Image_AdjustAlpha(float power, Image* image)
{
    AdjustImageLevelsAlpha(*image, power);
}

Texture2D* Image_GetTexture2D(const Image* image)
{
    return ConvertImageToTexture(image).Detach();
}

Animation* BlendAnimations_wrapper(Model* model, CharacterSkeleton* skeleton, CScriptArray* animations, CScriptArray* weights, CScriptArray* offsets, CScriptArray* timestamps)
{
    return !model ? nullptr : BlendAnimations(*model, skeleton, ArrayToPODVector<Animation*>(animations),
        ArrayToPODVector<float>(weights), ArrayToPODVector<float>(offsets), ArrayToPODVector<float>(timestamps)).Detach();
}

static const float TODO_poissonStep = 0.05f;
const PointCloud2DNorm& TODO_GetDefaultCloud()
{
    static PoissonRandom TODO_random(0);
    static const PointCloud2DNorm TODO_cloud = TODO_random.generate(TODO_poissonStep, 30, 10000);
    return TODO_cloud;
}

void TODO_CoverTerrainWithObjects(Node* terrainNode, Node* destNode, XMLFile* prefab,
    float minDistance, float objectRadius, const Vector2& begin, const Vector2& end)
{
    const float scale = minDistance / TODO_poissonStep;
    PointCloud2D points = samplePointCloud(TODO_GetDefaultCloud(), begin, end, scale);

    Scene* scene = terrainNode->GetScene();
    Octree* octree = scene->GetComponent<Octree>();
    Terrain* terrain = terrainNode->GetComponent<Terrain>();

    Node* prefabNode = scene->InstantiateXML(prefab->GetRoot(), Vector3::ZERO, Quaternion::IDENTITY);
    prefabNode->SetTemporary(true);

    PODVector<Drawable*> result;
    for (const Vector2& pos2 : points)
    {
        Vector3 position = Vector3(pos2.x_, 0, pos2.y_);
        position.y_ = terrain->GetHeight(position);
        result.Clear();
        SphereOctreeQuery query(result, Sphere(position, objectRadius));
        octree->GetDrawables(query);
        bool skip = false;
        for (Drawable* drawable : result)
        {
            if ((drawable->GetNode()->GetWorldPosition() - position).Length() < objectRadius)
            {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;

        float angle = rand() % 1000 / 1000.0f * 360;
        Node* child = prefabNode->Clone();
        child->SetTransform(position, Quaternion(0, angle, 0));
        destNode->AddChild(child);
    }

    scene->RemoveChild(prefabNode);
}

void RegisterScriptContext(asIScriptEngine* engine, const char* name)
{
    engine->RegisterObjectType(name, sizeof(ScriptContext), asOBJ_REF);
    engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()", asFUNCTION(FakeAddRef), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()", asFUNCTION(FakeReleaseRef), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(name, "void set_opIndex(uint, const Variant&in)", asFUNCTION(ScriptContext_SetItem), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(name, "const Variant& get_opIndex(uint)", asFUNCTION(ScriptContext_GetItem), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(name, "void Clear()", asFUNCTION(ScriptContext_ClearItems), asCALL_CDECL_OBJLAST);
}

void WeightBlender_Contruct(WeightBlender* ptr)
{
    new(ptr) WeightBlender();
}

void WeightBlender_Desturct(WeightBlender* ptr)
{
    ptr->~WeightBlender();
}

void WeightBlender_SetWeight(const String& key, float weight, float fadeTime, WeightBlender* ptr)
{
    ptr->SetWeight(key, weight, fadeTime);
}

float WeightBlender_GetWeight(const String& key, const WeightBlender* ptr)
{
    return ptr->GetWeight(key);
}

float WeightBlender_GetNormalizedWeight(const String& key, const WeightBlender* ptr)
{
    return ptr->GetNormalizedWeight(key);
}

void RegisterWeightBlender(asIScriptEngine* engine)
{
    engine->RegisterObjectType("WeightBlender", sizeof(WeightBlender), asOBJ_VALUE);
    engine->RegisterObjectBehaviour("WeightBlender", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WeightBlender_Contruct), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("WeightBlender", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(WeightBlender_Desturct), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("WeightBlender", "void SetWeight(const String&in, float, float=0)", asFUNCTION(WeightBlender_SetWeight), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("WeightBlender", "float GetWeight(const String&in) const", asFUNCTION(WeightBlender_GetWeight), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("WeightBlender", "float GetNormalizedWeight(const String&in) const", asFUNCTION(WeightBlender_GetNormalizedWeight), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("WeightBlender", "void Update(float, bool=false)", asMETHOD(WeightBlender, Update), asCALL_THISCALL);
}

//////////////////////////////////////////////////////////////////////////
void RegisterCharacterSkeleton(asIScriptEngine* engine)
{
    RegisterResource<CharacterSkeleton>(engine, "CharacterSkeleton");
}

//////////////////////////////////////////////////////////////////////////
bool CharacterAnimation_ImportAnimation(CharacterSkeleton* characterSkeleton, Model* model, Animation* animation,
    CharacterAnimation* characterAnimation)
{
    return characterSkeleton && model && animation
        ? characterAnimation->ImportAnimation(*characterSkeleton, *model, *animation)
        : nullptr;
}

bool ImportCharacterAnimation(const String& animationName, CharacterSkeleton* characterSkeleton, Model* model)
{
    ResourceCache* cache = GetScriptContext()->GetSubsystem<ResourceCache>();
    Animation* animation = cache->GetResource<Animation>(animationName);
    CharacterAnimation characterAnimation(GetScriptContext());;
    if (animation && characterSkeleton && model)
    {
        characterAnimation.SetName(animationName + ".xml");
        if (characterAnimation.ImportAnimation(*characterSkeleton, *model, *animation))
            return SaveResource(characterAnimation);
    }
    return false;
}

void RegisterCharacterAnimation(asIScriptEngine* engine)
{
    RegisterResource<CharacterAnimation>(engine, "CharacterAnimation");
    engine->RegisterObjectMethod("CharacterAnimation", "bool ImportAnimation(CharacterSkeleton@+, Model@+, Animation@+)", asFUNCTION(CharacterAnimation_ImportAnimation), asCALL_CDECL_OBJLAST);

    engine->RegisterGlobalFunction("bool ImportCharacterAnimation(const String&in, CharacterSkeleton@+, Model@+)", asFUNCTION(ImportCharacterAnimation), asCALL_CDECL);
}

void CharacterAnimationController_SetTargetTransform(const String& segment, const Matrix3x4& transform,
    CharacterAnimationController* characterAnimationController)
{
    characterAnimationController->SetTargetTransform(segment, transform);
}

void RegisterCharacterAnimationController(asIScriptEngine* engine)
{
    RegisterComponent<CharacterAnimationController>(engine, "CharacterAnimationController");
    RegisterSubclass<CharacterAnimationController, AnimationController>(engine, "AnimationController", "CharacterAnimationController");

    engine->RegisterObjectMethod("CharacterAnimationController", "void SetTargetTransform(const String&in, const Matrix3x4&in)", asFUNCTION(CharacterAnimationController_SetTargetTransform), asCALL_CDECL_OBJLAST);
}

}

void RegisterAPI(asIScriptEngine* engine)
{
    engine->RegisterObjectType("DefaultVertex", sizeof(DefaultVertex), asOBJ_VALUE | asOBJ_POD);
    engine->RegisterObjectBehaviour("DefaultVertex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DefaultVertex_Contruct), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty("DefaultVertex", "Vector3 position", offsetof(DefaultVertex, position_));
    engine->RegisterObjectMethod("DefaultVertex", "Vector4 get_uv(uint)", asFUNCTION(DefaultVertex_GetUV), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("DefaultVertex", "void set_uv(uint, Vector4)", asFUNCTION(DefaultVertex_SetUV), asCALL_CDECL_OBJLAST);

    RegisterObject<ModelFactory>(engine, "ModelFactory");
    engine->RegisterObjectMethod("ModelFactory", "void PushVertex(DefaultVertex &in)", asMETHOD(ModelFactory, AddVertex), asCALL_THISCALL);
    engine->RegisterObjectMethod("ModelFactory", "void PushIndex(uint)", asMETHOD(ModelFactory, AddIndex), asCALL_THISCALL);
    engine->RegisterObjectMethod("ModelFactory", "uint GetNumVerticesInBucket() const", asMETHOD(ModelFactory, GetCurrentNumVertices), asCALL_THISCALL);

    RegisterScriptContext(engine, "ScriptContext");
    RegisterScriptContext(engine, "ProceduralContext");

    engine->RegisterObjectMethod("ProceduralContext", "ModelFactory@ CreateModelFactory()", asFUNCTION(ProceduralContext_CreateModelFactory), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Model@+ CreateModel(ModelFactory@)", asFUNCTION(ProceduralContext_CreateModel), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Model@+ CreateQuadModel()", asFUNCTION(ProceduralContext_CreateQuadModel), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Texture2D@+ RenderTexture(uint, uint, const Color&in, XMLFile@+, Model@+, Array<Material@>@+, const Vector3&in, const Vector2&in, Array<Texture2D@>@+, Array<Vector4>@+)", asFUNCTION(ProceduralContext_RenderTexture0), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Texture2D@+ RenderTexture(uint, uint, const Color&in, XMLFile@+, Model@+, Material@+, const Vector3&in, const Vector2&in, Array<Texture2D@>@+, const Vector4&in = Vector4(1,1,1,1))", asFUNCTION(ProceduralContext_RenderTexture1), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Texture2D@+ RenderTexture(uint, uint, const Color&in, XMLFile@+, Model@+, Material@+, const Vector3&in, const Vector2&in, const Vector4&in = Vector4(1,1,1,1))", asFUNCTION(ProceduralContext_RenderTexture2), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Texture2D@+ RenderTexture(const Color&in, uint=1, uint=1)", asFUNCTION(ProceduralContext_RenderTexture3), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ProceduralContext", "Image@+ GeneratePerlinNoise(uint, uint, const Color&in, const Color&in, const Vector2&in, Array<Vector4>@+, float, float, const Vector2&in, XMLFile@+, Model@+, Material@+)", asFUNCTION(ProceduralContext_GeneratePerlinNoise), asCALL_CDECL_OBJLAST);

    engine->RegisterObjectMethod("Image", "void PrecalculateLevels()", asMETHOD(Image, PrecalculateLevels), asCALL_THISCALL);
    engine->RegisterObjectMethod("Image", "void AdjustAlpha(float)", asFUNCTION(Image_AdjustAlpha), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Image", "void BuildNormalMapAlpha()", asFUNCTION(Image_BuildNormalMapAlpha), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Image", "void FillGaps(uint=0)", asFUNCTION(Image_FillGaps), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Image", "Texture2D@+ GetTexture2D() const", asFUNCTION(Image_GetTexture2D), asCALL_CDECL_OBJLAST);

    RegisterWeightBlender(engine);

    engine->RegisterGlobalFunction("void TODO_CoverTerrainWithObjects(Node@+, Node@+, XMLFile@+, float, float, const Vector2&in, const Vector2&in)", asFUNCTION(TODO_CoverTerrainWithObjects), asCALL_CDECL);

    RegisterCharacterSkeleton(engine);
    RegisterCharacterAnimation(engine);
    RegisterCharacterAnimationController(engine);

    engine->RegisterGlobalFunction("Animation@+ BlendAnimations(Model@+, CharacterSkeleton@+, Array<Animation@>@+, Array<float>@+, Array<float>@+, Array<float>@+)", asFUNCTION(BlendAnimations_wrapper), asCALL_CDECL);
}

}
