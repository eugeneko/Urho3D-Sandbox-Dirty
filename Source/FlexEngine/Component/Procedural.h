#include <FlexEngine/Common.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>

namespace FlexEngine
{

/// Procedural resource generation component.
class Procedural : public Component
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
    /// Set force generation attribute.
    void SetForceGenerationAttr(bool forceGeneration);
    /// Set force generation attribute.
    bool GetForceGenerationAttr() const;
    /// Set seed attribute.
    void SetSeedAttr(unsigned seed);
    /// Set seed attribute.
    unsigned GetSeedAttr() const;

    /// Generate resources.
    void GenerateResources(bool forceGeneration, unsigned seed);

protected:
    /// Procedural objects description.
    SharedPtr<XMLFile> description_;
    /// Set to re-generate resources even if resources are already exist.
    bool forceGeneration_ = false;
    /// Random generator seed.
    unsigned seed_ = 0;
};

}
