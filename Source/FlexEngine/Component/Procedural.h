#include <FlexEngine/Common.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>

namespace FlexEngine
{

/// Procedural resource generation component.
class Procedural : Component
{
    URHO3D_OBJECT(Procedural, Component);

public:
    /// Construct.
    Procedural(Context* context);
    /// Destruct.
    ~Procedural();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set description attribute.
    void SetDescriptionAttr(const ResourceRef& value);
    /// Return description attribute.
    ResourceRef GetDescriptionAttr() const;

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

protected:
    /// Procedural objects description.
    SharedPtr<XMLFile> description_;
    /// Set to re-generate resources even if resources are already exist.
    bool forceGeneration_ = false;
};

}
