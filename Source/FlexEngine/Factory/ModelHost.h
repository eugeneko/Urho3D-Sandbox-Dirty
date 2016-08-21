#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

namespace Urho3D
{

}

namespace FlexEngine
{

class ModelFactory;

/// Host component of procedural model.
class ModelHost : public ProceduralComponent
{
    URHO3D_OBJECT(ModelHost, ProceduralComponent);

public:
    /// Construct.
    ModelHost(Context* context);
    /// Destruct.
    virtual ~ModelHost();
    /// Register object factory.
    static void RegisterObject(Context* context);

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Model.
    SharedPtr<Model> model_;

};

}
