#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>

namespace FlexEngine
{

//////////////////////////////////////////////////////////////////////////
ProceduralComponent::ProceduralComponent(Context* context)
    : DynamicComponent(context)
{

}

ProceduralComponent::~ProceduralComponent()
{

}

void ProceduralComponent::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(DynamicComponent);
    URHO3D_ACCESSOR_ATTRIBUTE("Seed", GetSeedAttr, SetSeedAttr, unsigned, 0, AM_DEFAULT);
}

void ProceduralComponent::SetSeedAttr(unsigned seed)
{
    seed_ = seed;
    MarkNeedUpdate();
}

unsigned ProceduralComponent::GetSeedAttr() const
{
    return seed_;
}

//////////////////////////////////////////////////////////////////////////
ProceduralComponentAgent::ProceduralComponentAgent(Context* context)
    : Component(context)
{

}

ProceduralComponentAgent::~ProceduralComponentAgent()
{

}

void ProceduralComponentAgent::RegisterObject(Context* context)
{
}

void ProceduralComponentAgent::ApplyAttributes()
{
    // Find host in this and parent nodes
    ProceduralComponent* root = node_->GetDerivedComponent<ProceduralComponent>();
    if (!root)
    {
        root = node_->GetParentDerivedComponent<ProceduralComponent>(true);
    }

    // Root must be found
    if (!root)
    {
        URHO3D_LOGERRORF("Host component for %s was not found", this->GetTypeName().CString());
        return;
    }

    // Mark host as dirty
    root->MarkNeedUpdate();
}

}
