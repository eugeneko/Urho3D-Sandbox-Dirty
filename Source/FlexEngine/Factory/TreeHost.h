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

    /// Set seed attribute.
    void SetSeedAttr(unsigned seed) { distribution_.seed_ = seed; }
    /// Get seed attribute.
    unsigned GetSeedAttr() const { return distribution_.seed_; }
    /// Set frequency attribute.
    void SetFrequencyAttr(unsigned frequency) { distribution_.frequency_ = frequency; }
    /// Get frequency attribute.
    unsigned GetFrequencyAttr() const { return distribution_.frequency_; }
    /// Set distribution type attribute.
    void SetDistributionTypeAttr(unsigned distributionType) { distribution_.distributionType_ = static_cast<TreeElementDistributionType>(distributionType); }
    /// Get distribution type attribute.
    unsigned GetDistributionTypeAttr() const { return static_cast<unsigned>(distribution_.distributionType_); }
    /// Set location attribute.
    void SetLocationAttr(const Vector2& location) { distribution_.location_.Set(location); }
    /// Get location attribute.
    const Vector2& GetLocationAttr() const { return distribution_.location_; }
    /// Set density attribute.
    void SetDensityAttr(const String& density) { distribution_.density_.SetCurveString(density); }
    /// Get density attribute.
    const String& GetDensityAttr() const { return distribution_.density_.GetCurveString(); }
    /// Set twirl step attribute.
    void SetTwirlStepAttr(float twirlStep) { distribution_.twirlStep_ = twirlStep; }
    /// Get twirl step attribute.
    float GetTwirlStepAttr() const { return distribution_.twirlStep_; }
    /// Set twirl noise attribute.
    void SetTwirlNoiseAttr(float twirlNoise) { distribution_.twirlNoise_ = twirlNoise; }
    /// Get twirl noise attribute.
    float GetTwirlNoiseAttr() const { return distribution_.twirlNoise_; }
    /// Set twirl base attribute.
    void SetTwirlBaseAttr(float twirlBase) { distribution_.twirlBase_ = twirlBase; }
    /// Get twirl base attribute.
    float GetTwirlBaseAttr() const { return distribution_.twirlBase_; }
    /// Set growth scale attribute.
    void SetGrowthScaleAttr(const Vector2& growthScaleRange) { distribution_.growthScale_.SetResultRange(growthScaleRange); }
    /// Get growth scale attribute.
    const Vector2& GetGrowthScaleAttr() const { return distribution_.growthScale_.GetResultRange(); }
    /// Set growth scale curve attribute.
    void SetGrowthScaleCurveAttr(const String& growthScale) { distribution_.growthScale_.SetCurveString(growthScale); }
    /// Get growth scale curve attribute.
    const String& GetGrowthScaleCurveAttr() const { return distribution_.growthScale_.GetCurveString(); }
    /// Set growth angle attribute.
    void SetGrowthAngleAttr(const Vector2& growthAngleRange) { distribution_.growthAngle_.SetResultRange(growthAngleRange); }
    /// Get growth angle attribute.
    const Vector2& GetGrowthAngleAttr() const { return distribution_.growthAngle_.GetResultRange(); }
    /// Set growth angle curve attribute.
    void SetGrowthAngleCurveAttr(const String& growthAngle) { distribution_.growthAngle_.SetCurveString(growthAngle); }
    /// Get growth angle curve attribute.
    const String& GetGrowthAngleCurveAttr() const { return distribution_.growthAngle_.GetCurveString(); }

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

    /// Set texture scale attribute.
    void SetTextureScaleAttr(const Vector2& textureScale) { shape_.textureScale_ = textureScale; }
    /// Get texture scale attribute.
    const Vector2& GetTextureScaleAttr() const { return shape_.textureScale_; }
    /// Set quality attribute.
    void SetQualityAttr(float quality) { shape_.quality_ = quality; }
    /// Get quality attribute.
    float GetQualityAttr() const { return shape_.quality_; }
    /// Set length attribute.
    void SetLengthAttr(const Vector2& length) { shape_.length_.Set(length); }
    /// Get length attribute.
    const Vector2& GetLengthAttr() const { return shape_.length_; }
    /// Set relative length attribute.
    void SetRelativeLengthAttr(bool relativeLength) { shape_.relativeLength_ = relativeLength; }
    /// Get relative length attribute.
    bool GetRelativeLengthAttr() const { return shape_.relativeLength_; }
    /// Set fake ending attribute.
    void SetFakeEndingAttr(bool fakeEnding) { shape_.fakeEnding_ = fakeEnding; }
    /// Get fake ending attribute.
    bool GetFakeEndingAttr() const { return shape_.fakeEnding_; }
    /// Set radius attribute.
    void SetRadiusAttr(const Vector2& radiusRange) { shape_.radius_.SetResultRange(radiusRange); }
    /// Get radius attribute.
    const Vector2& GetRadiusAttr() const { return shape_.radius_.GetResultRange(); }
    /// Set radius curve attribute.
    void SetRadiusCurveAttr(const String& radius) { shape_.radius_.SetCurveString(radius); }
    /// Get radius curve attribute.
    const String& GetRadiusCurveAttr() const { return shape_.radius_.GetCurveString(); }
    /// Set gravity intensity attribute.
    void SetGravityIntensityAttr(float gravityIntensity) { shape_.gravityIntensity_ = gravityIntensity; }
    /// Get gravity intensity attribute.
    float GetGravityIntensityAttr() const { return shape_.gravityIntensity_; }
    /// Set gravity resistance attribute.
    void SetGravityResistanceAttr(float gravityResistance) { shape_.gravityResistance_ = gravityResistance; }
    /// Get gravity resistance attribute.
    float GetGravityResistanceAttr() const { return shape_.gravityResistance_; }

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
    /// Set size attribute.
    void SetSizeAttr(const Vector2& size) { shape_.size_.Set(size); }
    /// Get size attribute.
    const Vector2& GetSizeAttr() const { return shape_.size_; }
    /// Set scale attribute.
    void SetScaleAttr(const Vector3& scale) { shape_.scale_ = scale; }
    /// Get scale attribute.
    const Vector3& GetScaleAttr() const { return shape_.scale_; }
    /// Set adjust to global attribute.
    void SetAdjustToGlobalAttr(const Vector2& adjustToGlobal) { shape_.adjustToGlobal_.Set(adjustToGlobal); }
    /// Get adjust to global attribute.
    const Vector2& GetAdjustToGlobalAttr() const { return shape_.adjustToGlobal_; }
    /// Set align vertical attribute.
    void SetAlignVerticalAttr(const Vector2& alignVertical) { shape_.alignVertical_.Set(alignVertical); }
    /// Get align vertical attribute.
    const Vector2& GetAlignVerticalAttr() const { return shape_.alignVertical_; }
    /// Set junction offset attribute.
    void SetJunctionOffsetAttr(const Vector3& junctionOffset) { shape_.junctionOffset_ = junctionOffset; }
    /// Get junction offset attribute.
    const Vector3& GetJunctionOffsetAttr() const { return shape_.junctionOffset_; }
    /// Set gravity intensity attribute.
    void SetGravityIntensityAttr(const Vector3& gravityIntensity) { shape_.gravityIntensity_ = gravityIntensity; }
    /// Get gravity intensity attribute.
    const Vector3& GetGravityIntensityAttr() const { return shape_.gravityIntensity_; }
    /// Set gravity resistance attribute.
    void SetGravityResistanceAttr(const Vector3& gravityResistance) { shape_.gravityResistance_ = gravityResistance; }
    /// Get gravity resistance attribute.
    const Vector3& GetGravityResistanceAttr() const { return shape_.gravityResistance_; }
    /// Set bump normals attribute.
    void SetBumpNormalsAttr(float bumpNormals) { shape_.bumpNormals_ = bumpNormals; }
    /// Get bump normals attribute.
    float GetBumpNormalsAttr() const { return shape_.bumpNormals_; }

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
