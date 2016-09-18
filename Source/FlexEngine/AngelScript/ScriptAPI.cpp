#include <FlexEngine/AngelScript/ScriptAPI.h>
#include <FlexEngine/Factory/ModelFactory.h>

#include <Urho3D/AngelScript/APITemplates.h>
#include <Urho3D/AngelScript/Script.h>

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

static void DefaultVertex_Contruct(DefaultVertex* ptr)
{
    new(ptr) DefaultVertex();
}

Vector4 DefaultVertex_GetUV(unsigned int idx, DefaultVertex* vertex)
{
    if (idx >= MAX_VERTEX_TEXCOORD) return Vector4::ZERO;
    return vertex->uv_[idx];
}
void DefaultVertex_SetUV(unsigned int idx, Vector4 value, DefaultVertex* vertex)
{
    if (idx >= MAX_VERTEX_TEXCOORD) return;
    vertex->uv_[idx] = value;
}

}

void RegisterAPI(asIScriptEngine* engine)
{
    engine->RegisterObjectType("DefaultVertex", sizeof(DefaultVertex), asOBJ_VALUE | asOBJ_POD);
    engine->RegisterObjectBehaviour("DefaultVertex", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DefaultVertex_Contruct), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty("DefaultVertex", "Vector3 position", offsetof(DefaultVertex, position_));
    engine->RegisterObjectMethod("DefaultVertex", "Vector4 get_uv(uint)", asFUNCTION(DefaultVertex_GetUV), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("DefaultVertex", "void set_uv(uint, Vector4)", asFUNCTION(DefaultVertex_SetUV), asCALL_CDECL_OBJLAST);

    engine->RegisterObjectType("ModelFactory", sizeof(ModelFactory), asOBJ_REF);
    engine->RegisterObjectBehaviour("ModelFactory", asBEHAVE_ADDREF, "void f()", asFUNCTION(FakeAddRef), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("ModelFactory", asBEHAVE_RELEASE, "void f()", asFUNCTION(FakeReleaseRef), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("ModelFactory", "void PushVertex(DefaultVertex &in vertex)", asMETHOD(ModelFactory, AddVertex), asCALL_THISCALL);
    engine->RegisterObjectMethod("ModelFactory", "void PushIndex(uint index)", asMETHOD(ModelFactory, AddIndex), asCALL_THISCALL);
    engine->RegisterObjectMethod("ModelFactory", "uint GetNumVerticesInBucket() const", asMETHOD(ModelFactory, GetCurrentNumVertices), asCALL_THISCALL);
}

}
