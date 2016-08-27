#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/TreeFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Graphics/Model.h>

namespace Urho3D
{

class Material;

}

namespace FlexEngine
{

class ModelFactory;

/// Host component of tree editor.
class TreeHost : public ProceduralComponent
{
    URHO3D_OBJECT(TreeHost, ProceduralComponent);

public:
    /// Construct.
    TreeHost(Context* context);
    /// Destruct.
    virtual ~TreeHost();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Called during element generation for each generated branch.
    void OnBranchGenerated(const BranchDescription& branch, const BranchShapeSettings& shape);
    /// Called during element generation for each generated leaf.
    void OnLeafGenerated(const LeafDescription& leaf, const LeafShapeSettings& shape);

    /// Get model.
    SharedPtr<Model> GetModel() const { return model_; }
    /// Get foliage center.
    const Vector3& GetFoliageCenter() const { return foliageCenter_; }

private:
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Tree model.
    SharedPtr<Model> model_;
    /// Tree model materials.
    Vector<SharedPtr<Material>> materials_;

    /// Positions of leaves.
    PODVector<Vector3> leavesPositions_;
    /// Center of leaves.
    Vector3 foliageCenter_;

};

/// Tree element component.
class TreeElement : public ProceduralComponentAgent
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
    virtual void Generate(TreeHost& host) = 0;
    /// Triangulate tree element recursively.
    void Triangulate(ModelFactory& factory, TreeHost& host) const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host) const = 0;

protected:
    /// Distribution settings.
    TreeElementDistribution distribution_;

};

/// Branch group component.
class BranchGroup : public TreeElement
{
    URHO3D_OBJECT(BranchGroup, TreeElement);

public:
    /// Construct.
    BranchGroup(Context* context);
    /// Destruct.
    virtual ~BranchGroup();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate.
    virtual void Generate(TreeHost& host) override;
    /// Get branches.
    const Vector<BranchDescription>& GetBranches() const { return branches_; }

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host) const override;

protected:
    /// Branch material.
    SharedPtr<Material> material_;
    /// Shape settings.
    BranchShapeSettings shape_;
    /// Min number of knots for branch curves. Default value is 5.
    unsigned minNumKnots_ = 5;
    /// Branches.
    Vector<BranchDescription> branches_;
};

/// Leaf group component.
class LeafGroup : public TreeElement
{
    URHO3D_OBJECT(LeafGroup, TreeElement);

public:
    /// Construct.
    LeafGroup(Context* context);
    /// Destruct.
    virtual ~LeafGroup();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate.
    virtual void Generate(TreeHost& host) override;

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host) const override;

protected:
    /// Leaf material.
    SharedPtr<Material> material_;
    /// Leaf shape settings.
    LeafShapeSettings shape_;
    /// Leaves.
    PODVector<LeafDescription> leaves_;
};

}
