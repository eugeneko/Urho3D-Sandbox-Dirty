#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/TreeFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Graphics/Model.h>

namespace Urho3D
{

class Material;
class XMLFile;

}

namespace FlexEngine
{

class ModelFactory;
class TreeLevelOfDetail;

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

    /// Set destination model attribute.
    void SetDestinationModelAttr(const ResourceRef& value);
    /// Get destination model attribute.
    ResourceRef GetDestinationModelAttr() const;

private:
    /// Generate tree topology and invariants.
    void GenerateTreeTopology();
    /// Implementation of procedural generator.
    virtual void DoUpdate() override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Name of destination texture.
    String destinationModelName_;
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
    URHO3D_OBJECT(TreeElement, ProceduralComponentAgent);

public:
    /// Construct.
    TreeElement(Context* context);
    /// Destruct.
    virtual ~TreeElement();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate.
    virtual void Generate(TreeHost& host) = 0;
    /// Triangulate tree element recursively.
    void Triangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const;

private:
    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const = 0;

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
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const override;

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
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const override;

protected:
    /// Leaf material.
    SharedPtr<Material> material_;
    /// Leaf shape settings.
    LeafShapeSettings shape_;
    /// Leaves.
    PODVector<LeafDescription> leaves_;
};

/// Level of detail component.
class TreeLevelOfDetail : public ProceduralComponentAgent
{
    URHO3D_OBJECT(TreeLevelOfDetail, ProceduralComponentAgent);

public:
    /// Construct.
    TreeLevelOfDetail(Context* context);
    /// Destruct.
    virtual ~TreeLevelOfDetail();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Get distance.
    float GetDistance() const { return distance_; }
    /// Get maximum number of branch segments.
    unsigned GetMaxBranchSegments() const { return maxBranchSegments_; }
    /// Get minimum number of branch segments.
    unsigned GetMinBranchSegments() const { return minBranchSegments_; }
    /// Get minimum angle.
    float GetMinAngle() const { return minAngle_; }
    /// Get number of radial segments.
    unsigned GetNumRadialSegments() const { return numRadialSegments_; }

protected:
    /// Distance.
    float distance_ = 0.0f;
    /// Maximum number of branch segments.
    unsigned maxBranchSegments_ = 0;
    /// Minimum number of branch segments.
    unsigned minBranchSegments_ = 0;
    /// If branch direction change within some region is greater than minimum angle, the region is divided into several segments.
    float minAngle_ = 0.0f;
    /// Number of radial segments.
    unsigned numRadialSegments_ = 0;

};

/// Tree proxy component. Proxy is the last level of detail. Each tree could have no more than one such component.
class TreeProxy : public ProceduralComponentAgent
{
    URHO3D_OBJECT(TreeProxy, ProceduralComponentAgent);

public:
    /// Construct.
    TreeProxy(Context* context);
    /// Destruct.
    virtual ~TreeProxy();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate proxy model and textures.
    SharedPtr<Model> Generate(SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials) const;

    /// Set LOD distance.
    void SetDistance(float distance) { distance_ = distance; }
    /// Get LOD distance.
    float GetDistance() const { return distance_; }
    /// Set proxy material.
    void SetProxyMaterial(SharedPtr<Material> material) { proxyMaterial_ = material; }
    /// Get proxy material.
    SharedPtr<Material> GetProxyMaterial() const { return proxyMaterial_; }

    /// Set destination proxy diffuse texture attribute.
    void SetDestinationProxyDiffuseAttr(const ResourceRef& value);
    /// Get destination proxy diffuse texture attribute.
    ResourceRef GetDestinationProxyDiffuseAttr() const;
    /// Set destination proxy normal texture attribute.
    void SetDestinationProxyNormalAttr(const ResourceRef& value);
    /// Get destination proxy normal texture attribute.
    ResourceRef GetDestinationProxyNormalAttr() const;
    /// Set proxy material attribute.
    void SetProxyMaterialAttr(const ResourceRef& value);
    /// Get proxy material attribute.
    ResourceRef GetProxyMaterialAttr() const;
    /// Set diffuse render path attribute.
    void SetDiffuseRenderPathAttr(const ResourceRef& value);
    /// Get diffuse render path attribute.
    ResourceRef GetDiffuseRenderPathAttr() const;
    /// Set normal render path attribute.
    void SetNormalRenderPathAttr(const ResourceRef& value);
    /// Get normal render path attribute.
    ResourceRef GetNormalRenderPathAttr() const;
    /// Set fill gap path attribute.
    void SetFillGapPathAttr(const ResourceRef& value);
    /// Get fill gap path attribute.
    ResourceRef GetFillGapPathAttr() const;
    /// Set fill gap material attribute.
    void SetFillGapMaterialAttr(const ResourceRef& value);
    /// Get fill gap material attribute.
    ResourceRef GetFillGapMaterialAttr() const;

private:
    /// Proxy LOD distance.
    float distance_ = 0.0f;
    /// Number of slices.
    unsigned numPlanes_ = 0;
    /// Number of vertical segments.
    unsigned numVerticalSegments_ = 0;

    /// Proxy texture width.
    unsigned proxyTextureWidth_ = 0;
    /// Proxy texture height.
    unsigned proxyTextureHeight_ = 0;
    /// Destination proxy diffuse map.
    String destinationProxyDiffuseName_;
    /// Destination proxy normal map.
    String destinationProxyNormalName_;
    /// Proxy material.
    SharedPtr<Material> proxyMaterial_;
    /// Diffuse render path.
    SharedPtr<XMLFile> diffuseRenderPath_;
    /// Normal render path.
    SharedPtr<XMLFile> normalRenderPath_;

    /// Fill gap render path.
    SharedPtr<XMLFile> fillGapRenderPath_;
    /// Fill gap material.
    SharedPtr<Material> fillGapMaterial_;
    /// Fill gap depth.
    unsigned fillGapDepth_ = 0;
};

}
