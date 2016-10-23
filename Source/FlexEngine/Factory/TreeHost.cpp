#include <FlexEngine/Factory/TreeHost.h>

#include <FlexEngine/Core/Attribute.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Factory/ProxyGeometryFactory.h>
#include <FlexEngine/Graphics/Wind.h>
#include <FlexEngine/Math/Hash.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>

namespace FlexEngine
{

namespace
{

static const char* spawnModeNames[] =
{
    "Explicit",
    "Absolute",
    "Relative",
    0
};

static const char* branchDistributionNames[] =
{
    "Alternate",
    "Opposite",
    0
};

static const char* normalTypeNames[] =
{
    "Fair",
    "Fake",
    0
};

static const char* treeProxyTypeNames[] =
{
    "Plane X0Y",
    "Cylinder",
    0
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
    URHO3D_MEMBER_ATTRIBUTE("Main Wind", float, windMainMagnitude_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Turbulence Magnitude", float, windTurbulenceMagnitude_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Oscillation Magnitude", float, windOscillationMagnitude_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Turbulence Frequency", float, windTurbulenceFrequency_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Oscillation Frequency", float, windOscillationFrequency_, 1.0f, AM_DEFAULT);
}

void TreeHost::EnumerateResources(Vector<ResourceRef>& resources)
{
    if (!destinationModelName_.Empty())
        resources.Push(ResourceRef(Model::GetTypeStatic(), destinationModelName_));
    if (TreeProxy* proxy = GetComponent<TreeProxy>())
    {
        resources.Push(ResourceRef(Image::GetTypeStatic(), proxy->GetDestinationProxyDiffuseAttr().name_));
        resources.Push(ResourceRef(Image::GetTypeStatic(), proxy->GetDestinationProxyNormalAttr().name_));
    }
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
    MarkResourceListDirty();
}

ResourceRef TreeHost::GetDestinationModelAttr() const
{
    return ResourceRef(Model::GetTypeStatic(), destinationModelName_);
}

void TreeHost::UpdateViews()
{
    StaticModel* staticModel = node_->GetDerivedComponent<StaticModel>();
    if (staticModel)
    {
        staticModel->SetModel(model_);
        for (unsigned i = 0; i < materials_.Size(); ++i)
        {
            staticModel->SetMaterial(i, materials_[i]);
        }
    }
}

bool TreeHost::ComputeHash(Hash& hash) const
{
    hash.HashString(destinationModelName_);
    hash.HashFloat(windMainMagnitude_);
    hash.HashFloat(windTurbulenceMagnitude_);
    hash.HashFloat(windOscillationMagnitude_);
    hash.HashFloat(windTurbulenceFrequency_);
    hash.HashFloat(windOscillationFrequency_);
    return true;
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

void TreeHost::DoGenerateResources(Vector<SharedPtr<Resource>>& resources)
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

    // Update ground adherence
    float maxMainAdherence = M_LARGE_EPSILON;
    float maxTurbulenceAdherence = M_LARGE_EPSILON;
    factory.ForEachVertex<DefaultVertex>(
        [&maxMainAdherence, &maxTurbulenceAdherence](unsigned, unsigned, unsigned, DefaultVertex& vertex)
    {
        maxMainAdherence = Max(maxMainAdherence, vertex.colors_[1].r_);
        maxTurbulenceAdherence = Max(maxTurbulenceAdherence, vertex.colors_[1].g_);
    });
    factory.ForEachVertex<DefaultVertex>(
        [&maxMainAdherence, &maxTurbulenceAdherence, this](unsigned, unsigned, unsigned, DefaultVertex& vertex)
    {
        vertex.colors_[1].r_ *= windMainMagnitude_ / maxMainAdherence;
        vertex.colors_[1].g_ *= windTurbulenceMagnitude_ / maxTurbulenceAdherence;
        vertex.colors_[1].a_ *= windOscillationMagnitude_;
        vertex.colors_[2].r_ = windTurbulenceFrequency_;
        vertex.colors_[2].g_ = windOscillationFrequency_;
        vertex.colors_[3].r_ = vertex.geometryNormal_.x_;
        vertex.colors_[3].g_ = vertex.geometryNormal_.y_;
        vertex.colors_[3].b_ = vertex.geometryNormal_.z_;
    });

    // Generate and setup
    materials_ = factory.GetMaterials();
    model_ = factory.BuildModel();
    resources.Push(model_);
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

        // Generate proxy
        Renderer* renderer = GetSubsystem<Renderer>();
        TreeProxy& treeProxy = *proxies[0];
        const bool hadInstancing = renderer->GetDynamicInstancing();
        renderer->SetDynamicInstancing(false);
        TreeProxy::GeneratedData data = treeProxy.Generate(model_, materials_);
        renderer->SetDynamicInstancing(hadInstancing);

        // Append proxy
        AppendEmptyLOD(*model_, treeProxy.GetDistance());
        AppendModelGeometries(*model_, *data.model_);
        resources.Push(data.diffuseImage_);
        resources.Push(data.normalImage_);
        materials_.Push(treeProxy.GetProxyMaterial());
    }

    // Save model
//     if (!destinationModelName_.Empty() && model_)
//     {
//         // Write texture to file and re-load it
//         model_->SetName(destinationModelName_);
//         SaveResource(*model_);
//     }

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
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Spawn Mode", TreeElementSpawnMode, distribution_.spawnMode_, spawnModeNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Frequency", float, distribution_.frequency_, 0, AM_DEFAULT);

    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Location", Vector2, distribution_.location_, GetVector, SetVector, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Density", String, distribution_.density_, GetCurveString, SetCurveString, "one", AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Distribution", TreeElementDistributionType, distribution_.distributionType_, branchDistributionNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle step", float, distribution_.twirlStep_, 180.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle random", float, distribution_.twirlNoise_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Twirl angle base", float, distribution_.twirlBase_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Relative Size", bool, distribution_.relativeSize_, true, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Scale", Vector2, distribution_.growthScale_, GetResultRange, SetResultRange, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Scale Curve", String, distribution_.growthScale_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Angle", Vector2, distribution_.growthAngle_, GetResultRange, SetResultRange, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Angle Curve", String, distribution_.growthAngle_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Twirl", Vector2, distribution_.growthTwirl_, GetResultRange, SetResultRange, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Growth Twirl Curve", String, distribution_.growthTwirl_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);

}

void TreeElement::Triangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    DoTriangulate(factory, host, lod);
    TriangulateChildren(*node_, factory, host, lod);
}

bool TreeElement::ComputeHash(Hash& hash) const
{
    hash.HashUInt(distribution_.seed_);
    hash.HashFloat(distribution_.frequency_);
    hash.HashVector3(distribution_.position_);
    hash.HashQuaternion(distribution_.rotation_);
    hash.HashEnum(distribution_.distributionType_);
    hash.HashVector2(distribution_.location_);
    hash.HashString(distribution_.density_.GetCurveString());
    hash.HashVector2(distribution_.density_.GetResultRange());
    hash.HashFloat(distribution_.twirlStep_);
    hash.HashFloat(distribution_.twirlNoise_);
    hash.HashFloat(distribution_.twirlBase_);
    hash.HashUInt(distribution_.relativeSize_);
    hash.HashString(distribution_.growthScale_.GetCurveString());
    hash.HashVector2(distribution_.growthScale_.GetResultRange());
    hash.HashString(distribution_.growthAngle_.GetCurveString());
    hash.HashVector2(distribution_.growthAngle_.GetResultRange());
    hash.HashString(distribution_.growthTwirl_.GetCurveString());
    hash.HashVector2(distribution_.growthTwirl_.GetResultRange());
    return true;
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

    URHO3D_MEMBER_ATTRIBUTE("Generate Branch", bool, generateBranch_, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Branch Material", GetBranchMaterialAttr, SetBranchMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("UV Scale", Vector2, branchShape_.textureScale_, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Quality", float, branchShape_.quality_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Length", Vector2, branchShape_.length_, GetVector, SetVector, Vector2::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Fake Ending", bool, branchShape_.fakeEnding_, false, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Radius", Vector2, branchShape_.radius_, GetResultRange, SetResultRange, Vector2(0.5f, 0.1f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Radius Curve", String, branchShape_.radius_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Resistance", float, branchShape_.resistance_, 0.5f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Gravity Intensity", float, branchShape_.gravityIntensity_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Main", float, branchShape_.windMainMagnitude_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Turbulence", float, branchShape_.windTurbulenceMagnitude_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Phase", float, branchShape_.windPhaseOffset_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Generate Frond", bool, generateFrond_, false, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Frond Material", GetFrondMaterialAttr, SetFrondMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Frond Size", Vector2, frondShape_.size_, GetResultRange, SetResultRange, Vector2(1.0f, 1.0f), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Frond Size Curve", String, frondShape_.size_, GetCurveString, SetCurveString, "linear", AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Frond Bending", float, frondShape_.bendingAngle_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Frond Rotation", Vector2, frondShape_.rotationAngle_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
}

void BranchGroup::Generate(TreeHost& host)
{
    // Initialize transform
    distribution_.position_ = node_->GetPosition();
    distribution_.rotation_ = node_->GetRotation();

    // Generate this level
    branches_.Clear();
    switch (distribution_.spawnMode_)
    {
    case TreeElementSpawnMode::Explicit:
        branches_ = InstantiateBranchGroup(BranchDescription(), distribution_, branchShape_, frondShape_, minNumKnots_);
        break;
    case TreeElementSpawnMode::Absolute:
    case TreeElementSpawnMode::Relative:
        {
            BranchGroup* parentGroup = node_->GetParentComponent<BranchGroup>();
            if (!parentGroup)
            {
                URHO3D_LOGERROR("BranchGroup with frequency > 0 must have parent BranchGroup");
                return;
            }

            for (const BranchDescription& parentBranch : parentGroup->GetBranches())
                branches_ += InstantiateBranchGroup(parentBranch, distribution_, branchShape_, frondShape_, minNumKnots_);
        }

    default:
        break;
    }

    // Notify host
    for (const BranchDescription& branch : branches_)
    {
        host.OnBranchGenerated(branch, branchShape_);
    }

    // Generate children
    GenerateChildren(*node_, host);
}

bool BranchGroup::ComputeHash(Hash& hash) const
{
    TreeElement::ComputeHash(hash);
    hash.HashUInt(generateBranch_);
    hash.HashString(branchMaterial_ ? branchMaterial_->GetName() : String::EMPTY);
    hash.HashVector2(branchShape_.textureScale_);
    hash.HashFloat(branchShape_.quality_);
    hash.HashVector2(branchShape_.length_);
    hash.HashUInt(branchShape_.fakeEnding_);
    hash.HashString(branchShape_.radius_.GetCurveString());
    hash.HashVector2(branchShape_.radius_.GetResultRange());
    hash.HashFloat(branchShape_.resistance_);
    hash.HashFloat(branchShape_.gravityIntensity_);
    hash.HashFloat(branchShape_.windMainMagnitude_);
    hash.HashFloat(branchShape_.windTurbulenceMagnitude_);
    hash.HashFloat(branchShape_.windPhaseOffset_);
    hash.HashUInt(generateFrond_);
    hash.HashString(frondMaterial_ ? frondMaterial_->GetName() : String::EMPTY);
    hash.HashString(frondShape_.size_.GetCurveString());
    hash.HashVector2(frondShape_.size_.GetResultRange());
    hash.HashFloat(frondShape_.bendingAngle_);
    hash.HashVector2(frondShape_.rotationAngle_);
    return true;
}

void BranchGroup::DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    BranchTessellationQualityParameters branchQuality;
    branchQuality.maxNumSegments_ = lod.GetMaxBranchSegments();
    branchQuality.minNumSegments_ = lod.GetMinBranchSegments();
    branchQuality.minAngle_ = lod.GetMinAngle();

    // Tessellate branches
    Vector<TessellatedBranchPoints> tessellatedBranches(branches_.Size());
    for (unsigned i = 0; i < branches_.Size(); ++i)
        tessellatedBranches[i] = TessellateBranch(branches_[i], branchShape_.quality_, branchQuality);

    // Generate branches
    if (generateBranch_)
    {
        factory.AddGeometry(branchMaterial_);
        for (unsigned i = 0; i < branches_.Size(); ++i)
        {
            if (!branches_[i].fake_)
                GenerateBranchGeometry(factory, branches_[i], tessellatedBranches[i], branchShape_, lod.GetNumRadialSegments());
        }
    }

    // Generate fronds
    if (generateFrond_)
    {
        factory.AddGeometry(frondMaterial_);
        for (unsigned i = 0; i < branches_.Size(); ++i)
        {
            if (!branches_[i].fake_)
                GenerateFrondGeometry(factory, branches_[i], tessellatedBranches[i], frondShape_);
        }
    }
}

void BranchGroup::SetBranchMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    branchMaterial_ = cache->GetResource<Material>(value.name_);
}

ResourceRef BranchGroup::GetBranchMaterialAttr() const
{
    return GetResourceRef(branchMaterial_, Material::GetTypeStatic());
}

void BranchGroup::SetFrondMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    frondMaterial_ = cache->GetResource<Material>(value.name_);
}

ResourceRef BranchGroup::GetFrondMaterialAttr() const
{
    return GetResourceRef(frondMaterial_, Material::GetTypeStatic());
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

    URHO3D_MEMBER_ATTRIBUTE("Scale", Vector3, shape_.scale_, Vector3::ONE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Adjust to Global", Vector2, shape_.adjustToGlobal_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Align Vertical", Vector2, shape_.alignVertical_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE_ACCESSOR("Rotate Z", Vector2, shape_.rotateZ_, GetVector, SetVector, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Junction Offset", Vector3, shape_.junctionOffset_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Bending", float, shape_.bending_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ENUM_ATTRIBUTE("Normal Type", LeafNormalType, shape_.normalType_, normalTypeNames, 0, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Bump Normals", float, shape_.bumpNormals_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Color 1", Color, shape_.firstColor_, Color::WHITE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Color 2", Color, shape_.secondColor_, Color::WHITE, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Main", Vector2, shape_.windMainMagnitude_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Turbulence", Vector2, shape_.windTurbulenceMagnitude_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Oscillation", Vector2, shape_.windOscillationMagnitude_, Vector2::ZERO, AM_DEFAULT);
}

void LeafGroup::Generate(TreeHost& host)
{
    // Initialize transform
    distribution_.position_ = node_->GetWorldPosition();
    distribution_.rotation_ = node_->GetWorldRotation();

    // Generate this level
    leaves_.Clear();
    switch (distribution_.spawnMode_)
    {
    case TreeElementSpawnMode::Explicit:
        leaves_ = InstantiateLeafGroup(BranchDescription(), distribution_, shape_);
        break;
    case TreeElementSpawnMode::Absolute:
    case TreeElementSpawnMode::Relative:
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
        break;
    default:
        break;
    }

    // Notify host
    for (const LeafDescription& leaf : leaves_)
    {
        host.OnLeafGenerated(leaf, shape_);
    }
}

bool LeafGroup::ComputeHash(Hash& hash) const
{
    TreeElement::ComputeHash(hash);
    hash.HashString(material_ ? material_->GetName() : String::EMPTY);
    hash.HashVector3(shape_.scale_);
    hash.HashVector2(shape_.adjustToGlobal_);
    hash.HashVector2(shape_.alignVertical_);
    hash.HashVector2(shape_.rotateZ_);
    hash.HashVector3(shape_.junctionOffset_);
    hash.HashFloat(shape_.bending_);
    hash.HashEnum(shape_.normalType_);
    hash.HashFloat(shape_.bumpNormals_);
    hash.HashVector2(shape_.windMainMagnitude_);
    hash.HashVector2(shape_.windTurbulenceMagnitude_);
    hash.HashVector2(shape_.windOscillationMagnitude_);
    return true;
}

void LeafGroup::DoTriangulate(ModelFactory& factory, TreeHost& host, TreeLevelOfDetail& lod) const
{
    factory.AddGeometry(material_);
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

bool TreeLevelOfDetail::ComputeHash(Hash& hash) const
{
    hash.HashFloat(distance_);
    hash.HashUInt(maxBranchSegments_);
    hash.HashUInt(minBranchSegments_);
    hash.HashFloat(minAngle_);
    hash.HashUInt(numRadialSegments_);
    return true;
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

    URHO3D_MEMBER_ENUM_ATTRIBUTE("Type", TreeProxyType, type_, treeProxyTypeNames, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Distance", GetDistance, SetDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Number of Planes", unsigned, numPlanes_, 8, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Number of Segments", unsigned, numVerticalSegments_, 3, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Resistance", float, resistance_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Wind Magnitude", float, windMagnitude_, 0.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Proxy Width", unsigned, proxyTextureWidth_, 1024, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Proxy Height", unsigned, proxyTextureHeight_, 256, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Diffuse", GetDestinationProxyDiffuseAttr, SetDestinationProxyDiffuseAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Normal", GetDestinationProxyNormalAttr, SetDestinationProxyNormalAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Proxy Material", GetProxyMaterialAttr, SetProxyMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("RP Diffuse", GetDiffuseRenderPathAttr, SetDiffuseRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("RP Normal", GetNormalRenderPathAttr, SetNormalRenderPathAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Fill Gap Precision", unsigned, fillGapPrecision_, 2, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Adjust Alpha", float, adjustAlpha_, 1.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Dithering Granularity", float, ditheringGranularity_, 100.0f, AM_DEFAULT);
    URHO3D_MEMBER_ATTRIBUTE("Flip Normals", bool, flipNormals_, false, AM_DEFAULT);
}

TreeProxy::GeneratedData TreeProxy::Generate(SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials) const
{
    ModelFactory factory(context_);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);
    factory.AddGeometry(proxyMaterial_);

    // Add empty geometry to hide proxy LOD at small distances
    factory.SetLevel(0);
    factory.AddEmpty();

    // Add actual proxy geometry
    factory.SetLevel(1);

    // Generate proxy geometry
    Vector<OrthoCameraDescription> cameras;
    PODVector<DefaultVertex> vertices;
    PODVector<unsigned> indices;
    const BoundingBox boundingBox = model->GetBoundingBox();
    switch (type_)
    {
    case TreeProxyType::PlaneX0Y:
        GeneratePlainProxy(boundingBox, Max(1u, proxyTextureWidth_), Max(1u, proxyTextureHeight_), cameras, vertices, indices);
        break;
    case FlexEngine::TreeProxyType::Cylider:
        {
            CylinderProxyParameters param;
            param.centerPositions_ = true;
            param.generateDiagonal_ = false;
            param.diagonalAngle_ = 0;
            param.numSurfaces_ = numPlanes_;
            param.numVertSegments_ = numVerticalSegments_;
            GenerateCylinderProxy(boundingBox, param, Max(1u, proxyTextureWidth_), Max(1u, proxyTextureHeight_),
                cameras, vertices, indices);
        }
        break;
    default:
        break;
    }

    // Fill parameters
    float maxHeight = 0.0f;
    for (unsigned i = 0; i < vertices.Size(); ++i)
        maxHeight = Max(maxHeight, (vertices[i].position_.y_ + vertices[i].uv_[1].y_));
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
            vertex.uv_[2].w_ = ditheringGranularity_;
            const float relativeHeight = Clamp((vertex.position_.y_+ vertex.uv_[1].y_) / maxHeight, 0.0f, 1.0f);
            vertex.colors_[1].r_ = windMagnitude_ * Pow(relativeHeight, 1.0f / (1.0f - resistance_));
        }
    }

    // Fill parameters
    factory.AddPrimitives(vertices, indices, false);

    // Update bounding box and set LOD distance
    SharedPtr<Model> proxyModel = factory.BuildModel();
    proxyModel->SetBoundingBox(boundingBox);
    proxyModel->GetGeometry(0, 1)->SetLodDistance(distance_);

    // Render proxy textures
    TextureDescription desc;
    desc.color_ = Color::TRANSPARENT;
    desc.width_ = Max(1u, proxyTextureWidth_);
    desc.height_ = Max(1u, proxyTextureHeight_);
    GeometryDescription geometryDesc;
    geometryDesc.model_ = model;
    geometryDesc.materials_ = materials;
    desc.geometries_.Push(geometryDesc);
    desc.cameras_.Push(cameras);
    desc.parameters_.Populate(VSP_WINDDIRECTION, Vector4::ZERO);
    desc.parameters_.Populate(VSP_WINDPARAM, Vector4::ZERO);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    desc.renderPath_ = diffuseRenderPath_;
    SharedPtr<Texture2D> diffuseTexture = RenderTexture(context_, desc, TextureMap());
    diffuseTexture->SetName(destinationProxyDiffuseName_);
    SharedPtr<Image> diffuseImage = ConvertTextureToImage(diffuseTexture);
    FillImageGaps(diffuseImage, fillGapPrecision_);
    diffuseImage->PrecalculateLevels();
    AdjustImageLevelsAlpha(*diffuseImage, adjustAlpha_);

    desc.renderPath_ = normalRenderPath_;
    SharedPtr<Texture2D> normalTexture = RenderTexture(context_, desc, TextureMap());
    normalTexture->SetName(destinationProxyNormalName_);
    SharedPtr<Image> normalImage = ConvertTextureToImage(normalTexture);
    if (flipNormals_)
        FlipNormalMapZ(*normalImage);
    BuildNormalMapAlpha(normalImage);
    FillImageGaps(normalImage, fillGapPrecision_);
    normalImage->PrecalculateLevels();

    GeneratedData result;
    result.model_ = proxyModel;
    result.diffuseImage_ = diffuseImage;
    result.normalImage_ = normalImage;
    return result;
}

void TreeProxy::SetDestinationProxyDiffuseAttr(const ResourceRef& value)
{
    destinationProxyDiffuseName_ = value.name_;
    MarkResourceListDirty();
}

ResourceRef TreeProxy::GetDestinationProxyDiffuseAttr() const
{
    return ResourceRef(Texture2D::GetTypeStatic(), destinationProxyDiffuseName_);
}

void TreeProxy::SetDestinationProxyNormalAttr(const ResourceRef& value)
{
    destinationProxyNormalName_ = value.name_;
    MarkResourceListDirty();
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

bool TreeProxy::ComputeHash(Hash& hash) const
{
    hash.HashEnum(type_);
    hash.HashFloat(distance_);
    hash.HashUInt(numPlanes_);
    hash.HashUInt(numVerticalSegments_);
    hash.HashFloat(resistance_);
    hash.HashFloat(windMagnitude_);
    hash.HashUInt(proxyTextureWidth_);
    hash.HashUInt(proxyTextureHeight_);
    hash.HashUInt(fillGapPrecision_);
    hash.HashFloat(adjustAlpha_);
    hash.HashUInt(flipNormals_);
    return true;
}

}
