#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/TreeFactory.h>

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class Material;

}

namespace FlexEngine
{

class ModelFactory;

/// Helper for Serializable that provides triggers in the editor.
class EnableTriggers
{
protected:
    /// Always returns false.
    bool GetFalse() const { return false; }

};

/// Define a trigger attribute that call function on each invocation.
#define URHO3D_TRIGGER_ATTRIBUTE(name, callback) URHO3D_ACCESSOR_ATTRIBUTE(name, GetFalse, callback, bool, false, AM_EDIT)

/// Tree editor component.
class TreeEditor : public LogicComponent, public EnableTriggers
{
    URHO3D_OBJECT(TreeEditor, LogicComponent);

public:
    /// Construct.
    TreeEditor(Context* context);
    /// Destruct.
    virtual ~TreeEditor();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;
    /// Called on scene update, variable timestep.
    virtual void Update(float timeStep) override;

    /// Generate and triangulate tree.
    void Generate();
    /// Update preview in sibling StaticModel component.
    void UpdatePreview();

    /// Mark for tree re-generation on the next update.
    void MarkTreeUpdate();

    /// Get model.
    SharedPtr<Model> GetModel() const { return model_; }

private:
    /// Completely re-calculate and update geometry.
    void ReCalculate(bool);

protected:
    /// Seed of generation.
    unsigned seed_;
    /// Update period.
    float updatePeriod_;

    /// Is update required?
    bool needUpdate_;
    /// Accumulated time for update.
    float elapsedTime_;
    /// Tree model.
    SharedPtr<Model> model_;
    /// Tree model materials.
    Vector<SharedPtr<Material>> materials_;

};

/// Tree element component.
class TreeElement : public Component, public EnableTriggers
{
    URHO3D_OBJECT(TreeElement, Component);

public:
    /// Construct.
    TreeElement(Context* context);
    /// Destruct.
    virtual ~TreeElement();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes() override;

    /// Generate.
    virtual void Generate() = 0;
    /// Triangulate tree element recursively.
    void Triangulate(ModelFactory& factory) const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory) const = 0;

private:
    /// Reset node transform.
    void ResetNodeTransform(bool);

};

/// Branch group editor component.
class BranchGroupEditor : public TreeElement
{
    URHO3D_OBJECT(BranchGroupEditor, TreeElement);

public:
    /// Construct.
    BranchGroupEditor(Context* context);
    /// Destruct.
    virtual ~BranchGroupEditor();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate.
    virtual void Generate() override;
    /// Get branches.
    const Vector<BranchDescription>& GetBranches() const { return branches_; }

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory) const override;

protected:
    /// Branch material.
    SharedPtr<Material> material_;
    /// Branch group description.
    BranchGroupDescription desc_;
    /// Branches.
    Vector<BranchDescription> branches_;
};

/// Leaf group editor component.
class LeafGroupEditor : public TreeElement
{
    URHO3D_OBJECT(LeafGroupEditor, TreeElement);

public:
    /// Construct.
    LeafGroupEditor(Context* context);
    /// Destruct.
    virtual ~LeafGroupEditor();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate.
    virtual void Generate() override;

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory) const override;

protected:
    /// Leaf material.
    SharedPtr<Material> material_;
    /// Branch group description.
    LeafGroupDescription desc_;
    /// Leaves.
    PODVector<LeafDescription> leaves_;
};

}
