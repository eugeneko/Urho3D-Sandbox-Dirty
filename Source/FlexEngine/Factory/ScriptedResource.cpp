#include <FlexEngine/Factory/ScriptedResource.h>

#include <FlexEngine/Math/Hash.h>
#include <FlexEngine/Factory/TextureFactory.h> // #TODO Remove

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/Image.h> // #TODO Remove
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/AngelScript/ScriptFile.h>

namespace Urho3D
{

namespace FlexEngine
{

ScriptedResource::ScriptedResource(Context* context)
    : ProceduralComponent(context)
    , resources_(ScriptFile::GetTypeStatic())
{
}

ScriptedResource::~ScriptedResource()
{
}

void ScriptedResource::RegisterObject(Context* context)
{
    context->RegisterFactory<ScriptedResource>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);

    URHO3D_ACCESSOR_ATTRIBUTE("Type", GetTypeAttr, SetTypeAttr, StringHash, StringHash(), AM_FILE | AM_NOEDIT);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Script", GetScriptAttr, SetScriptAttr, ResourceRef, ResourceRef(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entry Point", GetEntryPoint, SetEntryPoint, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Resources", GetResourcesAttr, SetResourcesAttr, ResourceRefList, ResourceRefList(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Num Parameters", GetNumParametersAttr, SetNumParametersAttr, unsigned, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Parameters", GetParametersAttr, SetParametersAttr, VariantMap, Variant::emptyVariantVector, AM_DEFAULT);
}

void ScriptedResource::EnumerateResources(Vector<ResourceRef>& resources)
{
    for (unsigned i = 0; i < resources_.names_.Size(); ++i)
        resources.Push(ResourceRef(type_, resources_.names_[i]));
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

void ScriptedResource::SetParametersAttr(const VariantMap& parameters)
{
    for (unsigned i = 0; i < Min(parameters.Size(), parameters_.Size()); ++i)
    {
        const Variant* elem = parameters[StringHash(i)];
        parameters_[i] = elem ? elem->GetVector4() : Vector4::ZERO;
    }
}

const VariantMap& ScriptedResource::GetParametersAttr() const
{
    parametersAttr_.Clear();
    for (unsigned i = 0; i < parameters_.Size(); ++i)
        parametersAttr_[StringHash(i)] = parameters_[i];
    return parametersAttr_;
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
    hash.HashUInt(parameters_.Size());
    for (unsigned i = 0; i < parameters_.Size(); ++i)
        hash.HashVector4(parameters_[i]);
    return true;
}

void ScriptedResource::DoGenerateResources(Vector<SharedPtr<Resource>>& resources)
{
    static const unsigned startParam = 3;

    // Prepare context
    ScriptContext output;
    output.context_ = GetContext();
    output.items_.Resize(parameters_.Size());
    for (unsigned i = 0; i < parameters_.Size(); ++i)
        output.items_[i] = parameters_[i];

    // Call script
    if (script_)
    {
        const String entryPoint = entryPoint_.Empty() ? "Main" : entryPoint_;
        const VariantVector parameters = { &output };
        script_->Execute("void " + entryPoint + "(ProceduralContext@)", parameters);
    }

    // Update resource list
    if (output.items_.Size() >= startParam)
    {
        resources_.names_.Resize(output.items_[0].GetUInt());
        resources_.type_ = output.items_[1].GetStringHash();
        type_ = output.items_[2].GetStringHash();
    }

    // Update resources
    for (unsigned i = 0; i < resources_.names_.Size(); ++i)
    {
        resources.Push(i + startParam < output.items_.Size()
                       ? DynamicCast<Resource>(SharedPtr<RefCounted>(output.items_[i + startParam].GetPtr()))
                       : nullptr);
    }
}

}

}
