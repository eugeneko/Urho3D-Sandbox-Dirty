#include <FlexEngine/Factory/ScriptedResource.h>

// #include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Math/Hash.h>
// #include <FlexEngine/Resource/ResourceCacheHelpers.h>
// #include <FlexEngine/Resource/ResourceHash.h>
#include <FlexEngine/Factory/TextureFactory.h> // #TODO Remove

#include <Urho3D/Core/Context.h>
// #include <Urho3D/Core/CoreEvents.h>
// #include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/Image.h> // #TODO Remove
#include <Urho3D/Resource/ResourceCache.h>
// #include <Urho3D/Scene/Node.h>
// #include <Urho3D/Scene/Scene.h>
// #include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/AngelScript/ScriptFile.h>

namespace FlexEngine
{


ScriptedResource::ScriptedResource(Context* context)
    : ProceduralComponent(context)
    , resources_(ScriptFile::GetTypeStatic(), StringVector(1))
{
}

ScriptedResource::~ScriptedResource()
{
}

void ScriptedResource::RegisterObject(Context* context)
{
    context->RegisterFactory<ScriptedResource>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Script", GetScriptAttr, SetScriptAttr, ResourceRef, ResourceRef(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entry Point", GetEntryPoint, SetEntryPoint, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Resources", GetResourcesAttr, SetResourcesAttr, ResourceRefList, ResourceRefList(ScriptFile::GetTypeStatic(), StringVector(1)), AM_DEFAULT);
}

void ScriptedResource::ApplyAttributes()
{

}

void ScriptedResource::EnumerateResources(Vector<ResourceRef>& resources)
{
    for (unsigned i = 0; i < resources_.names_.Size(); ++i)
        resources.Push(ResourceRef(resources_.type_, resources_.names_[i]));
}

void ScriptedResource::SetScriptAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    script_ = cache->GetResource<ScriptFile>(value.name_);
}

ResourceRef ScriptedResource::GetScriptAttr() const
{
    return GetResourceRef(script_, ScriptFile::GetTypeStatic());
}

bool ScriptedResource::ComputeHash(Hash& hash) const
{
    if (script_)
    {
        PODVector<unsigned char> buffer;
        script_->SaveByteCode(MemoryBuffer(buffer));
        hash.HashBuffer(buffer);
    }
    hash.HashString(entryPoint_);
    hash.HashUInt(resources_.type_);
    hash.HashUInt(resources_.names_.Size());
    for (unsigned i = 0; i < resources_.names_.Size(); ++i)
        hash.HashString(resources_.names_[i]);
    return true;
}

void ScriptedResource::DoGenerateResources(Vector<SharedPtr<Resource>>& resources)
{
    // Call script
    ScriptContext output;
    output.context_ = GetContext();
    if (script_)
    {
        const String entryPoint = entryPoint_.Empty() ? "Main" : entryPoint_;
        const VariantVector parameters = { &output };
        script_->Execute("void " + entryPoint + "(ProceduralContext@)", parameters);
    }

    // Update resource list
    if (output.items_.Size() >= 2)
    {
        resources_.names_.Resize(output.items_[0].GetUInt());
        resources_.type_ = output.items_[1].GetStringHash();
    }

    // Update resources
    for (unsigned i = 0; i < resources_.names_.Size(); ++i)
    {
        resources.Push(i + 2 < output.items_.Size()
                       ? DynamicCast<Resource>(SharedPtr<RefCounted>(output.items_[i + 2].GetPtr()))
                       : nullptr);
    }
}

}
