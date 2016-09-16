#include <FlexEngine/Factory/TreeHost.h>

#include <FlexEngine/Core/Attribute.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Factory/ProxyGeometryFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>

namespace FlexEngine
{

namespace
{

static const char* branchDistributionNames[] =
{
    "Alternate",
    0
};

static const char* treeLodTypeNames[] =
{
    "Geometry",
    "Billboard"
};

void GenerateChildren(Node& node, TreeHost& host)
{
    for (SharedPtr<Node> child : node.GetChildren())
    {
        PODVector<TreeElement*> elements;
        child->GetDerivedComponents<TreeElement>(elements);
        for (TreeElement* element : elements)
        {
            element->Generate(host);
        }
    }
}

void TriangulateChildren(Node& node, ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod)
{
    for (SharedPtr<Node> child : node.GetChildren())
    {
        PODVector<TreeElement*> elements;
        child->GetDerivedComponents<TreeElement>(elements);
        for (TreeElement* element : elements)
        {
            element->Triangulate(factory, host, lod);
        }
    }
}

}

extern const char* inputParameterUniform[];

//////////////////////////////////////////////////////////////////////////
TreeHost::TreeHost(Context* context)
    : ProceduralComponent(context)
{
    ResetToDefault();
}

TreeHost::~TreeHost()
{
}

void TreeHost::RegisterObject(Context* context)
{
    context->RegisterFactory<TreeHost>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Destination Model", GetDestinationModelAttr, SetDestinationModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);

}

void TreeHost::OnBranchGenerated(const BranchDescription& /*branch*/, const BranchShapeSettings& /*shape*/)
{

}

void TreeHost::OnLeafGenerated(const LeafDescription& leaf, const LeafShapeSettings& /*shape*/)
{
    leavesPositions_.Push(leaf.location_.position_);
}

void TreeHost::SetDestinationModelAttr(const ResourceRef& value)
{
    destinationModelName_ = value.name_;
    MarkNeedUpdate();
}

ResourceRef TreeHost::GetDestinationModelAttr() const
{
    return ResourceRef(Model::GetTypeStatic(), destinationModelName_);
}

void TreeHost::UpdateViews()
{
    StaticModel* staticModel = node_->GetComponent<StaticModel>();
    if (staticModel)
    {
        staticModel->SetModel(model_);
        for (unsigned i = 0; i < materials_.Size(); ++i)
        {
            staticModel->SetMaterial(i, materials_[i]);
        }
    }
}

void TreeHost::GenerateTreeTopology()
{
    // Generate tree
    leavesPositions_.Clear();
    GenerateChildren(*node_, *this);

    // Compute foliage center
    foliageCenter_ = Vector3::ZERO;
    for (const Vector3& position : leavesPositions_)
    {
        foliageCenter_ += position;
    }
    if (!leavesPositions_.Empty())
    {
        foliageCenter_ /= static_cast<float>(leavesPositions_.Size());
    }
}

void TreeHost::DoUpdate()
{
    GenerateTreeTopology();

    // Update list of LODs
    PODVector<TreeLevelOfDetail*> lods;
    GetComponents(lods);

    // Triangulate tree
    ModelFactory factory(context_);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);

    for (unsigned i = 0; i < lods.Size(); ++i)
    {
        factory.SetLevel(i);
        TriangulateChildren(*node_, factory, *this, *lods[i]);
    }

    materials_ = factory.GetMaterials();
    model_ = factory.BuildModel(materials_);
    for (unsigned i = 0; i < lods.Size(); ++i)
    {
        for (unsigned j = 0; j < model_->GetNumGeometries(); ++j)
        {
            if (Geometry* geometry = model_->GetGeometry(j, i))
            {
                geometry->SetLodDistance(lods[i]->GetDistance());
            }
        }
    }

    // Get proxy component
    PODVector<TreeProxy*> proxies;
    GetComponents(proxies);

    if (!proxies.Empty())
    {
        if (proxies.Size() > 1)
        {
            URHO3D_LOGWARNING("Tree must have at most one proxy level");
        }

        // Append proxy
        TreeProxy& treeProxy = *proxies[0];
        AppendEmptyLOD(*model_, treeProxy.GetDistance());
        AppendModelGeometries(*model_, *treeProxy.Generate(model_, materials_));
        materials_.Push(treeProxy.GetProxyMaterial());
    }

    // Save model
    if (!destinationModelName_.Empty() && model_)
    {
        // Write texture to file and re-load it
        model_->SetName(destinationModelName_);
        SaveResource(*model_);
    }

    // Update model and materials
    UpdateViews();
}

//////////////////////////////////////////////////////////////////////////
TreeElement::TreeElement(Context* context)
    : ProceduralComponentAgent(context)
{
    ResetToDefault();
}

TreeElement::~TreeElement()
{

}

void TreeElement::RegisterObject(Context* context)
{
    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponentAgent);

    URHO3D_MEMBER_ATTRIBUTE("Seed", unsigned, distribution_.seed_, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Frequency", unsigned, distribution_.frequency_, 0, AM_DEFAULT);

    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Location", Vector2, distribution_.location_, GetVector, SetVector, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Density", String, distribution_.density_, GetCurveString, SetCurveString, "one", AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Distribution", TreeElementDistributionType, distribution_.distributionType_, branchDistributionNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle step", float, distribution_.twirlStep_, 180.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle random", float, distribution_.twirlNoise_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle base", float, distribution_.twirlBase_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Scale", Vector2, distribution_.growthScale_, GetResultRange, SetResultRange, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Scale Curve", String, distribution_.growthScale_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Angle", Vector2, distribution_.growthAngle_, GetResultRange, SetResultRange, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Angle Curve", String, distribution_.growthAngle_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);

}

void TreeElement::Triangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    DoTriangulate(factory, host, lod);
    TriangulateChildren(*node_, factory, host, lod);
}

//////////////////////////////////////////////////////////////////////////
BranchGroup::BranchGroup(Context* context)
    : TreeElement(context)
{
    ResetToDefault();
}

BranchGroup::~BranchGroup()
{
}

void BranchGroup::RegisterObject(Context* context)
{
    context->RegisterFactory<BranchGroup>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TreeElement);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("UV Scale", Vector2, shape_.textureScale_, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Quality", float, shape_.quality_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Length", Vector2, shape_.length_, GetVector, SetVector, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Relative Length", bool, shape_.relativeLength_, true, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Fake Ending", bool, shape_.fakeEnding_, false, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Radius", Vector2, shape_.radius_, GetResultRange, SetResultRange, Vector2(0.5f, 0.1f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Radius Curve", String, shape_.radius_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Gravity Intensity", float, shape_.gravityIntensity_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Gravity Resistance", float, shape_.gravityResistance_, 0.5f, AM_DEFAULT);
}

void BranchGroup::Generate(TreeHost& host)
{
    // Initialize transform
    distribution_.position_ = node_->GetPosition();
    distribution_.direction_ = node_->GetRotation() * Vector3::UP;

    // Generate this level
    branches_.Clear();
    if (distribution_.frequency_ == 0)
    {
        branches_ = InstantiateBranchGroup(BranchDescription(), distribution_, shape_, minNumKnots_);
    }
    else
    {
        BranchGroup* parentGroup = node_->GetParentComponent<BranchGroup>();
        if (!parentGroup)
        {
            URHO3D_LOGERROR("BranchGroup with frequency > 0 must have parent BranchGroup");
            return;
        }
        for (const BranchDescription& parentBranch : parentGroup->GetBranches())
        {
            branches_ += InstantiateBranchGroup(parentBranch, distribution_, shape_, minNumKnots_);
        }
    }

    // Notify host
    for (const BranchDescription& branch : branches_)
    {
        host.OnBranchGenerated(branch, shape_);
    }

    // Generate children
    GenerateChildren(*node_, host);
}

void BranchGroup::DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    TreeLodDescription lodDesc;
    lodDesc.branchTessellationQuality_.maxNumSegments_ = lod.GetMaxBranchSegments();
    lodDesc.branchTessellationQuality_.minNumSegments_ = lod.GetMinBranchSegments();
    lodDesc.branchTessellationQuality_.minAngle_ = lod.GetMinAngle();

    factory.SetMaterial(material_);
    for (const BranchDescription& branch : branches_)
    {
        if (!branch.fake_)
        {
            const TessellatedBranchPoints tessellatedPoints =
                TessellateBranch(branch.positionsCurve_, branch.radiusesCurve_, lodDesc.branchTessellationQuality_);

            GenerateBranchGeometry(factory, tessellatedPoints, shape_, lod.GetNumRadialSegments());
        }
    }
}

void BranchGroup::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef BranchGroup::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

//////////////////////////////////////////////////////////////////////////
LeafGroup::LeafGroup(Context* context)
    : TreeElement(context)
{
    ResetToDefault();
}

LeafGroup::~LeafGroup()
{

}

void LeafGroup::RegisterObject(Context* context)
{
    context->RegisterFactory<LeafGroup>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TreeElement);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);

    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Size", Vector2, shape_.size_, GetVector, SetVector, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Scale", Vector3, shape_.scale_, Vector3::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Adjust to Global", Vector2, shape_.adjustToGlobal_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Align Vertical", Vector2, shape_.alignVertical_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Junction Offset", Vector3, shape_.junctionOffset_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Gravity Intensity", Vector3, shape_.gravityIntensity_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Gravity Resistance", Vector3, shape_.gravityResistance_, Vector3::ONE * 0.5f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Bump Normals", float, shape_.bumpNormals_, 0.0f, AM_DEFAULT);
}

void LeafGroup::Generate(TreeHost& host)
{
    // Initialize transform
    distribution_.position_ = node_->GetWorldPosition();
    distribution_.direction_ = node_->GetWorldRotation() * Vector3::UP;

    // Generate this level
    leaves_.Clear();
    if (distribution_.frequency_ == 0)
    {
        leaves_ = InstantiateLeafGroup(BranchDescription(), distribution_, shape_);
    }
    else
    {
        BranchGroup* parentGroup = node_->GetParentComponent<BranchGroup>();
        if (!parentGroup)
        {
            URHO3D_LOGERROR("LeafGroup with frequency > 0 must have parent BranchGroup");
            return;
        }
        for (const BranchDescription& parentBranch : parentGroup->GetBranches())
        {
            leaves_ += InstantiateLeafGroup(parentBranch, distribution_, shape_);
        }
    }

    // Notify host
    for (const LeafDescription& leaf : leaves_)
    {
        host.OnLeafGenerated(leaf, shape_);
    }
}

void LeafGroup::DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    factory.SetMaterial(material_);
    for (const LeafDescription& leaf : leaves_)
    {
        GenerateLeafGeometry(factory, shape_, leaf.location_, host.GetFoliageCenter());
    }
}

void LeafGroup::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef LeafGroup::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

//////////////////////////////////////////////////////////////////////////
TreeLevelOfDetail::TreeLevelOfDetail(Context* context)
    : ProceduralComponentAgent(context)
{
    ResetToDefault();
}

TreeLevelOfDetail::~TreeLevelOfDetail()
{
}

void TreeLevelOfDetail::RegisterObject(Context* context)
{
    context->RegisterFactory<TreeLevelOfDetail>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponentAgent);

    URHO3D_MEMBER_ATTRIBUTE("Distance", float, distance_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Max Branch Segments", unsigned, maxBranchSegments_, 100, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Min Branch Segments", unsigned, minBranchSegments_, 4, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Min Angle", float, minAngle_, 10.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Num Radial Segments", unsigned, numRadialSegments_, 5, AM_DEFAULT);
}

//////////////////////////////////////////////////////////////////////////
TreeProxy::TreeProxy(Context* context)
    : ProceduralComponentAgent(context)
{
    ResetToDefault();
}

TreeProxy::~TreeProxy()
{

}

void TreeProxy::RegisterObject(Context* context)
{
    context->RegisterFactory<TreeProxy>(FLEXENGINE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Distance", GetDistance, SetDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Number of Planes", unsigned, numPlanes_, 8, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Number of Segments", unsigned, numVerticalSegments_, 3, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Proxy Width", unsigned, proxyTextureWidth_, 1024, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Proxy Height", unsigned, proxyTextureHeight_, 256, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Diffuse", GetDestinationProxyDiffuseAttr, SetDestinationProxyDiffuseAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Normal", GetDestinationProxyNormalAttr, SetDestinationProxyNormalAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Material", GetProxyMaterialAttr, SetProxyMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("RP Diffuse", GetDiffuseRenderPathAttr, SetDiffuseRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("RP Normal", GetNormalRenderPathAttr, SetNormalRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Fill Gap RP", GetFillGapPathAttr, SetFillGapPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Fill Gap Material", GetFillGapMaterialAttr, SetFillGapMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Fill Gap Depth", unsigned, fillGapDepth_, 0, AM_DEFAULT);
}

SharedPtr<Model> TreeProxy::Generate(SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials) const
{
    ModelFactory factory(context_);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);
    factory.SetMaterial(proxyMaterial_);

    // Add empty geometry to hide proxy LOD at small distances
    factory.SetLevel(0);
    factory.PushNothing();

    // Add actual proxy geometry
    factory.SetLevel(1);

    // Setup parameters
    CylinderProxyParameters param;
    param.centerPositions_ = true;
    param.generateDiagonal_ = false;
    param.diagonalAngle_ = 0;
    param.numSurfaces_ = numPlanes_;
    param.numVertSegments_ = numVerticalSegments_;

    // Generate proxy geometry
    Vector<OrthoCameraDescription> cameras;
    PODVector<DefaultVertex> vertices;
    PODVector<unsigned> indices;
    const BoundingBox boundingBox = model->GetBoundingBox();
    GenerateCylinderProxy(boundingBox, param, Max(1u, proxyTextureWidth_), Max(1u, proxyTextureHeight_), cameras, vertices, indices);

    // Fill parameters
    for (unsigned i = 0; i < numPlanes_; ++i)
    {
        unsigned numVertices = vertices.Size() / numPlanes_;
        for (unsigned j = 0; j < numVertices; ++j)
        {
            const float sign = i % 2 ? 1.0f : -1.0f;
            DefaultVertex& vertex = vertices[numVertices * i + j];
            vertex.uv_[2].x_ = Cos(180.0f / numPlanes_ + 1.0f);
            vertex.uv_[2].y_ = 0.05f;
            vertex.uv_[2].z_ = sign;
            vertex.uv_[2].w_ = 50;
        }
    }

    // Fill parameters
    factory.Push(vertices, indices, false);

    // Update bounding box and set LOD distance
    SharedPtr<Model> proxyModel = factory.BuildModel(factory.GetMaterials());
    proxyModel->SetBoundingBox(boundingBox);
    proxyModel->GetGeometry(0, 1)->SetLodDistance(distance_);

    // Render proxy textures
    TextureDescription desc;
    desc.width_ = Max(1u, proxyTextureWidth_);
    desc.height_ = Max(1u, proxyTextureHeight_);
    GeometryDescription geometryDesc;
    geometryDesc.model_ = model;
    geometryDesc.materials_ = materials;
    desc.geometries_.Push(geometryDesc);
    desc.cameras_.Push(cameras);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    desc.renderPath_ = diffuseRenderPath_;
    SharedPtr<Texture2D> diffuseTexture = RenderTexture(context_, desc, TextureMap());
    diffuseTexture->SetName(destinationProxyDiffuseName_);
    SharedPtr<Image> diffuseImage = ConvertTextureToImage(diffuseTexture);
    diffuseImage = FillTextureGaps(diffuseImage, fillGapDepth_, true, fillGapRenderPath_, GetOrCreateQuadModel(context_), fillGapMaterial_, inputParameterUniform[0]);
    diffuseImage->PrecalculateLevels();
    SaveImage(cache, *diffuseImage);

    desc.renderPath_ = normalRenderPath_;
    SharedPtr<Texture2D> normalTexture = RenderTexture(context_, desc, TextureMap());
    normalTexture->SetName(destinationProxyNormalName_);
    SharedPtr<Image> normalImage = ConvertTextureToImage(normalTexture);
    normalImage = FillTextureGaps(normalImage, fillGapDepth_, false, fillGapRenderPath_, GetOrCreateQuadModel(context_), fillGapMaterial_, inputParameterUniform[0]);
    normalImage->PrecalculateLevels();
    SaveImage(cache, *normalImage);

    return proxyModel;
}

void TreeProxy::SetDestinationProxyDiffuseAttr(const ResourceRef& value)
{
    destinationProxyDiffuseName_ = value.name_;
}

ResourceRef TreeProxy::GetDestinationProxyDiffuseAttr() const
{
    return ResourceRef(Texture2D::GetTypeStatic(), destinationProxyDiffuseName_);
}

void TreeProxy::SetDestinationProxyNormalAttr(const ResourceRef& value)
{
    destinationProxyNormalName_ = value.name_;
}

ResourceRef TreeProxy::GetDestinationProxyNormalAttr() const
{
    return ResourceRef(Texture2D::GetTypeStatic(), destinationProxyNormalName_);
}

void TreeProxy::SetProxyMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    proxyMaterial_ = cache->GetResource<Material>(value.name_);
}

ResourceRef TreeProxy::GetProxyMaterialAttr() const
{
    return GetResourceRef(proxyMaterial_, Material::GetTypeStatic());
}

void TreeProxy::SetDiffuseRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    diffuseRenderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef TreeProxy::GetDiffuseRenderPathAttr() const
{
    return GetResourceRef(diffuseRenderPath_, XMLFile::GetTypeStatic());
}

void TreeProxy::SetNormalRenderPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    normalRenderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef TreeProxy::GetNormalRenderPathAttr() const
{
    return GetResourceRef(normalRenderPath_, XMLFile::GetTypeStatic());
}

void TreeProxy::SetFillGapPathAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    fillGapRenderPath_ = cache->GetResource<XMLFile>(value.name_);
}

ResourceRef TreeProxy::GetFillGapPathAttr() const
{
    return GetResourceRef(fillGapRenderPath_, XMLFile::GetTypeStatic());
}

void TreeProxy::SetFillGapMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    fillGapMaterial_ = cache->GetResource<Material>(value.name_);
}

ResourceRef TreeProxy::GetFillGapMaterialAttr() const
{
    return GetResourceRef(fillGapMaterial_, Material::GetTypeStatic());
}

}
