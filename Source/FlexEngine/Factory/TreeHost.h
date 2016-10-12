#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/TreeFactory.h>
#include <FlexEngine/Factory/ProceduralComponent.h>

#include <Urho3D/Graphics/Model.h>

namespace Urho3D
{

class Image;
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
    /// Enumerate resources.
    virtual void EnumerateResources(Vector<ResourceRef>& resources) override;

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
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;
    /// Generate tree topology and invariants.
    void GenerateTreeTopology();
    /// Generate resources.
    virtual void DoGenerateResources(Vector<SharedPtr<Resource>>& resources) override;

    /// Update views with generated resource.
    void UpdateViews();

protected:
    /// Name of destination texture.
    String destinationModelName_;
    /// Tree model.
    SharedPtr<Model> model_;
    /// Tree model materials.
    Vector<SharedPtr<Material>> materials_;

    /// Magnitude of deformations caused by main wind.
    float windMainMagnitude_ = 0.0f;
    /// Magnitude of deformations caused by turbulence.
    float windTurbulenceMagnitude_ = 0.0f;
    /// Magnitude of foliage oscillation.
    float windOscillationMagnitude_ = 0.0f;
    /// Frequency of deformations caused by turbulence.
    float windTurbulenceFrequency_ = 0.0f;
    /// Frequency of foliage oscillation.
    float windOscillationFrequency_ = 0.0f;

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
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;

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

    /// Set branch material attribute.
    void SetBranchMaterialAttr(const ResourceRef& value);
    /// Return branch material attribute.
    ResourceRef GetBranchMaterialAttr() const;
    /// Set frond material attribute.
    void SetFrondMaterialAttr(const ResourceRef& value);
    /// Return frond material attribute.
    ResourceRef GetFrondMaterialAttr() const;

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;

    /// Triangulate this tree element.
    virtual void DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const override;

protected:
    /// Whether to generate branch geometry.
    bool generateBranch_ = true;
    /// Branch material.
    SharedPtr<Material> branchMaterial_;
    /// Branch shape settings.
    BranchShapeSettings branchShape_;
    /// Whether to generate frond geometry.
    bool generateFrond_ = false;
    /// Frond material.
    SharedPtr<Material> frondMaterial_;
    /// Frond shape settings.
    FrondShapeSettings frondShape_;
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
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;

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

private:
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;

private:
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
    /// Tree proxy data.
    struct GeneratedData
    {
        /// Proxy model.
        SharedPtr<Model> model_;
        /// Diffuse texture.
        SharedPtr<Image> diffuseImage_;
        /// Normal texture.
        SharedPtr<Image> normalImage_;
    };

    /// Construct.
    TreeProxy(Context* context);
    /// Destruct.
    virtual ~TreeProxy();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Generate proxy model and textures.
    GeneratedData Generate(SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials) const;

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
    /// Compute hash.
    virtual bool ComputeHash(Hash& hash) const override;

private:
    /// Proxy LOD distance.
    float distance_ = 0.0f;
    /// Number of slices.
    unsigned numPlanes_ = 0;
    /// Number of vertical segments.
    unsigned numVerticalSegments_ = 0;
    /// Controls tree proxy bending caused by external force. Should be in range [0, 1).
    /// 0 is the linear deformation of the branch without geometry bending.
    float resistance_ = 0.0f;
    /// Value of deformations caused by main wind.
    float windMagnitude_ = 0.0f;

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
