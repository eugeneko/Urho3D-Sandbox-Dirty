#include <FlexEngine/Factory/TreeHost.h>

#include <FlexEngine/Factory/ModelFactory.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
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

void TriangulateChildren(Node& node, ModelFactory& factory, TreeHost& host)
{
    for (SharedPtr<Node> child : node.GetChildren())
    {
        PODVector<TreeElement*> elements;
        child->GetDerivedComponents<TreeElement>(elements);
        for (TreeElement* element : elements)
        {
            element->Triangulate(factory, host);
        }
    }
}

}

//////////////////////////////////////////////////////////////////////////
TreeHost::TreeHost(Context* context)
    : ProceduralComponent(context)
{
}

TreeHost::~TreeHost()
{
}

void TreeHost::RegisterObject(Context* context)
{
    context->RegisterFactory<TreeHost>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(ProceduralComponent);
}

void TreeHost::OnBranchGenerated(const BranchDescription& /*branch*/, const BranchShapeSettings& /*shape*/)
{

}

void TreeHost::OnLeafGenerated(const LeafDescription& leaf, const LeafShapeSettings& /*shape*/)
{
    leavesPositions_.Push(leaf.location_.position_);
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

void TreeHost::DoUpdate()
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

    // Triangulate tree
    ModelFactory factory(context_);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);
    TriangulateChildren(*node_, factory, *this);
    materials_ = factory.GetMaterials();
    model_ = factory.BuildModel(materials_);

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

    URHO3D_ACCESSOR_ATTRIBUTE("Seed", GetSeedAttr, SetSeedAttr, unsigned, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Frequency", GetFrequencyAttr, SetFrequencyAttr, unsigned, 0, AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Growth Location", GetLocationAttr, SetLocationAttr, Vector2, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Growth Density", GetDensityAttr, SetDensityAttr, String, "one", AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Distribution", GetDistributionTypeAttr, SetDistributionTypeAttr, unsigned, branchDistributionNames, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Twirl angle step", GetTwirlStepAttr, SetTwirlStepAttr, float, 180.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Twirl angle random", GetTwirlNoiseAttr, SetTwirlNoiseAttr, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Twirl angle base", GetTwirlBaseAttr, SetTwirlBaseAttr, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Growth Scale", GetGrowthScaleAttr, SetGrowthScaleAttr, Vector2, Vector2::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Growth Scale Curve", GetGrowthScaleCurveAttr, SetGrowthScaleCurveAttr, String, "linear", AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Growth Angle", GetGrowthAngleAttr, SetGrowthAngleAttr, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Growth Angle Curve", GetGrowthAngleCurveAttr, SetGrowthAngleCurveAttr, String, "linear", AM_DEFAULT);
}

void TreeElement::ApplyAttributes()
{
    // Mark host as dirty
    TreeHost* root = node_->GetParentComponent<TreeHost>(true);
    if (!root)
    {
        URHO3D_LOGERROR("BranchGroup must have parent TreeHost");
        return;
    }

    root->MarkNeedUpdate();
}

void TreeElement::Triangulate(ModelFactory& factory, TreeHost& host) const
{
    DoTriangulate(factory, host);
    TriangulateChildren(*node_, factory, host);
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
    URHO3D_ACCESSOR_ATTRIBUTE("UV Scale", GetTextureScaleAttr, SetTextureScaleAttr, Vector2, Vector2::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Quality", GetQualityAttr, SetQualityAttr, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLengthAttr, SetLengthAttr, Vector2, Vector2::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Relative Length", GetRelativeLengthAttr, SetRelativeLengthAttr, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Fake Ending", GetFakeEndingAttr, SetFakeEndingAttr, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadiusAttr, SetRadiusAttr, Vector2, Vector2(0.5f, 0.1f), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius Curve", GetRadiusCurveAttr, SetRadiusCurveAttr, String, "linear", AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Gravity Intensity", GetGravityIntensityAttr, SetGravityIntensityAttr, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Gravity Resistance", GetGravityResistanceAttr, SetGravityResistanceAttr, float, 0.5f, AM_DEFAULT);
}

void BranchGroup::Generate(TreeHost& host)
{
    // Initialize transform
    distribution_.position_ = node_->GetWorldPosition();
    distribution_.direction_ = node_->GetWorldRotation() * Vector3::UP;

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

void BranchGroup::DoTriangulate(ModelFactory& factory, TreeHost& host) const
{
    // #TODO Fixme
    TreeLodDescription lod;
    lod.branchTessellationQuality_.maxNumSegments_ = 100;
    lod.branchTessellationQuality_.minNumSegments_ = 4;
    lod.branchTessellationQuality_.minAngle_ = 10.0f;

    factory.SetMaterial(material_);
    for (const BranchDescription& branch : branches_)
    {
        if (!branch.fake_)
        {
            const TessellatedBranchPoints tessellatedPoints =
                TessellateBranch(branch.positionsCurve_, branch.radiusesCurve_, lod.branchTessellationQuality_);

            GenerateBranchGeometry(factory, tessellatedPoints, shape_, 5u); // #TODO Fixme
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

    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSizeAttr, SetSizeAttr, Vector2, Vector2::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScaleAttr, SetScaleAttr, Vector3, Vector3::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Adjust to Global", GetAdjustToGlobalAttr, SetAdjustToGlobalAttr, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Align Vertical", GetAlignVerticalAttr, SetAlignVerticalAttr, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Junction Offset", GetJunctionOffsetAttr, SetJunctionOffsetAttr, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Gravity Intensity", GetGravityIntensityAttr, SetGravityIntensityAttr, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Gravity Resistance", GetGravityResistanceAttr, SetGravityResistanceAttr, Vector3, Vector3::ONE * 0.5f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Bump Normals", GetBumpNormalsAttr, SetBumpNormalsAttr, float, 0.0f, AM_DEFAULT);
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

void LeafGroup::DoTriangulate(ModelFactory& factory, TreeHost& host) const
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

}
