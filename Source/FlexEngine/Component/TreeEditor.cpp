#include <FlexEngine/Component/TreeEditor.h>

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

void GenerateChildren(Node& node)
{
    for (SharedPtr<Node> child : node.GetChildren())
    {
        PODVector<TreeElement*> elements;
        child->GetDerivedComponents<TreeElement>(elements);
        for (TreeElement* element : elements)
        {
            element->Generate();
        }
    }
}

void TriangulateChildren(Node& node, ModelFactory& factory)
{
    for (SharedPtr<Node> child : node.GetChildren())
    {
        PODVector<TreeElement*> elements;
        child->GetDerivedComponents<TreeElement>(elements);
        for (TreeElement* element : elements)
        {
            element->Triangulate(factory);
        }
    }
}

}

//////////////////////////////////////////////////////////////////////////
TreeEditor::TreeEditor(Context* context)
    : LogicComponent(context)
    , seed_(0)
    , updatePeriod_(0.2f)
    , needUpdate_(false)
    , elapsedTime_(0.0f)
{
}

TreeEditor::~TreeEditor()
{
}

void TreeEditor::RegisterObject(Context* context)
{
    context->RegisterFactory<TreeEditor>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(LogicComponent);
    URHO3D_TRIGGER_ATTRIBUTE("<Re-calculate>", ReCalculate);
    URHO3D_ATTRIBUTE("Seed", unsigned, seed_, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Update period", float, updatePeriod_, 0.2f, AM_DEFAULT);
}

void TreeEditor::ApplyAttributes()
{
    MarkTreeUpdate();
}

void TreeEditor::Update(float timeStep)
{
    elapsedTime_ += timeStep;
    if (needUpdate_ && elapsedTime_ >= updatePeriod_)
    {
        elapsedTime_ = 0.0f;
        needUpdate_ = false;
        Generate();
        UpdatePreview();
    }
}

void TreeEditor::MarkTreeUpdate()
{
    needUpdate_ = true;
}

void TreeEditor::Generate()
{
    GenerateChildren(*node_);

    ModelFactory factory(context_);
    factory.Initialize(DefaultVertex::GetVertexElements(), true);
    TriangulateChildren(*node_, factory);
    materials_ = factory.GetMaterials();
    model_ = factory.BuildModel(materials_);
}

void TreeEditor::UpdatePreview()
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

void TreeEditor::ReCalculate(bool)
{
    Generate();
    UpdatePreview();
}

//////////////////////////////////////////////////////////////////////////
TreeElement::TreeElement(Context* context)
    : Component(context)
{

}

TreeElement::~TreeElement()
{

}

void TreeElement::RegisterObject(Context* context)
{
    URHO3D_TRIGGER_ATTRIBUTE("<Reset transform>", ResetNodeTransform);
}

void TreeElement::ApplyAttributes()
{
    // Re-triangulate
    TreeEditor* root = node_->GetParentComponent<TreeEditor>(true);
    if (!root)
    {
        URHO3D_LOGERROR("BranchGroupEditor must have parent TreeEditor");
        return;
    }

    root->MarkTreeUpdate();
}

void TreeElement::Triangulate(ModelFactory& factory) const
{
    DoTriangulate(factory);
    TriangulateChildren(*node_, factory);
}

void TreeElement::ResetNodeTransform(bool)
{
    node_->SetWorldTransform(Vector3::ZERO, Quaternion::IDENTITY);
}

//////////////////////////////////////////////////////////////////////////
BranchGroupEditor::BranchGroupEditor(Context* context)
    : TreeElement(context)
{
}

BranchGroupEditor::~BranchGroupEditor()
{
}

void BranchGroupEditor::RegisterObject(Context* context)
{
    context->RegisterFactory<BranchGroupEditor>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TreeElement);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ATTRIBUTE("UV Scale", Vector2, desc_.material_.textureScale_, Vector2(1, 1), AM_DEFAULT);

    URHO3D_ATTRIBUTE("Seed", unsigned, desc_.distribution_.seed_, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", unsigned, desc_.distribution_.frequency_, 0, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Growth Location", Vector2, desc_.distribution_.location_, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Density", String, desc_.distribution_.density_.text_, "one", AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Distribution", desc_.distribution_.distributionType_, branchDistributionNames, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle step", float, desc_.distribution_.twirlStep_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle random", float, desc_.distribution_.twirlNoise_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle base", float, desc_.distribution_.twirlBase_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Scale", String, desc_.distribution_.growthScale_.text_, "one", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Angle", String, desc_.distribution_.growthAngle_.text_, "zero", AM_DEFAULT);

    URHO3D_ATTRIBUTE("Length", Vector2, desc_.shape_.length_, Vector2(1.0f, 1.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Relative Length", bool, desc_.shape_.relativeLength_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Fake ending", bool, desc_.shape_.fakeEnding_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Radius", String, desc_.shape_.radius_.text_, "one", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gravity Intensity", float, desc_.shape_.gravityIntensity_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gravity Resistance", float, desc_.shape_.gravityResistance_, 0.5f, AM_DEFAULT);
}

void BranchGroupEditor::Generate()
{
    // Initialize functions
    desc_.distribution_.growthScale_.Initialize();
    desc_.distribution_.growthAngle_.Initialize();
    desc_.distribution_.density_.Initialize();
    desc_.shape_.radius_.Initialize();

    // Initialize transform
    desc_.distribution_.position_ = node_->GetWorldPosition();
    desc_.distribution_.direction_ = node_->GetWorldRotation() * Vector3::UP;

    // Generate this level
    branches_.Clear();
    if (desc_.distribution_.frequency_ == 0)
    {
        branches_ = InstantiateBranchGroup(BranchDescription(), desc_);
    }
    else
    {
        BranchGroupEditor* parentGroup = node_->GetParentComponent<BranchGroupEditor>();
        if (!parentGroup)
        {
            URHO3D_LOGERROR("BranchGroupEditor with frequency > 0 must have parent BranchGroupEditor");
            return;
        }
        for (const BranchDescription& parentBranch : parentGroup->GetBranches())
        {
            branches_ += InstantiateBranchGroup(parentBranch, desc_);
        }
    }

    // Generate children
    GenerateChildren(*node_);
}

void BranchGroupEditor::DoTriangulate(ModelFactory& factory) const
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

            GenerateBranchGeometry(factory, tessellatedPoints, desc_.shape_, desc_.material_, 5u); // #TODO Fixme
        }
    }
}

void BranchGroupEditor::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef BranchGroupEditor::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

//////////////////////////////////////////////////////////////////////////
LeafGroupEditor::LeafGroupEditor(Context* context)
    : TreeElement(context)
{

}

LeafGroupEditor::~LeafGroupEditor()
{

}

void LeafGroupEditor::RegisterObject(Context* context)
{
    context->RegisterFactory<LeafGroupEditor>(FLEXENGINE_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(TreeElement);

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);

    URHO3D_ATTRIBUTE("Seed", unsigned, desc_.distribution_.seed_, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Frequency", unsigned, desc_.distribution_.frequency_, 0, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Growth Location", Vector2, desc_.distribution_.location_, Vector2(0.0f, 1.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Density", String, desc_.distribution_.density_.text_, "one", AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Distribution", desc_.distribution_.distributionType_, branchDistributionNames, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle step", float, desc_.distribution_.twirlStep_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle random", float, desc_.distribution_.twirlNoise_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twirl angle base", float, desc_.distribution_.twirlBase_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Scale", String, desc_.distribution_.growthScale_.text_, "one", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Growth Angle", String, desc_.distribution_.growthAngle_.text_, "zero", AM_DEFAULT);

    URHO3D_ATTRIBUTE("Size", Vector2, desc_.shape_.size_, Vector2::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Geometry Scale", Vector3, desc_.shape_.scale_, Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Adjust to Global", Vector2, desc_.shape_.adjustToGlobal_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Align Vertical", Vector2, desc_.shape_.alignVerical_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Junction Offset", Vector3, desc_.shape_.junctionOffset_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gravity Intensity", Vector3, desc_.shape_.gravityIntensity_, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Gravity Resistance", Vector3, desc_.shape_.gravityResistance_, Vector3::ONE * 0.5f, AM_DEFAULT);
}

void LeafGroupEditor::Generate()
{
    // Initialize functions
    desc_.distribution_.growthScale_.Initialize();
    desc_.distribution_.growthAngle_.Initialize();
    desc_.distribution_.density_.Initialize();

    // Initialize transform
    desc_.distribution_.position_ = node_->GetWorldPosition();
    desc_.distribution_.direction_ = node_->GetWorldRotation() * Vector3::UP;

    // Generate this level
    leaves_.Clear();
    if (desc_.distribution_.frequency_ == 0)
    {
        leaves_ = InstantiateLeafGroup(BranchDescription(), desc_);
    }
    else
    {
        BranchGroupEditor* parentGroup = node_->GetParentComponent<BranchGroupEditor>();
        if (!parentGroup)
        {
            URHO3D_LOGERROR("LeafGroupEditor with frequency > 0 must have parent BranchGroupEditor");
            return;
        }
        for (const BranchDescription& parentBranch : parentGroup->GetBranches())
        {
            leaves_ += InstantiateLeafGroup(parentBranch, desc_);
        }
    }
}

void LeafGroupEditor::DoTriangulate(ModelFactory& factory) const
{
    factory.SetMaterial(material_);
    for (const LeafDescription& leaf : leaves_)
    {
        GenerateLeafGeometry(factory, leaf.shape_, leaf.location_);
    }
}

void LeafGroupEditor::SetMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    material_ = cache->GetResource<Material>(value.name_);
}

ResourceRef LeafGroupEditor::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

}
