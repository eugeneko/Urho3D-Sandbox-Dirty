#include <FlexEngine/Factory/FatVertex.h>

#include <FlexEngine/Math/MathDefs.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

struct FatVertex FatVertex::ConstructFromXML(const XMLElement& element)
{
    FatVertex result;
    result.position_ = element.GetVector3("position");
    result.geometryNormal_ = element.GetVector3("geomnormal");
    result.tangent_ = element.GetVector3("tangent");
    result.binormal_ = element.GetVector3("binormal");
    result.normal_ = element.GetVector3("normal");
    for (unsigned i = 0; i < MAX_VERTEX_TEXCOORD; ++i)
    {
        result.uv_[i] = element.GetVector4(ToString("uv%d", i));
    }
    for (unsigned i = 0; i < MAX_VERTEX_BONES; ++i)
    {
        result.boneIndices_[i] = element.GetUInt(ToString("i%d", i));
        result.boneWeights_[i] = element.GetFloat(ToString("w%d", i));
    }
    result.mainAdherence_ = element.GetFloat("mainAdherence");
    result.branchAdherence_ = element.GetFloat("branchAdherence");
    result.phase_ = element.GetFloat("phase");
    result.edgeOscillation_ = element.GetFloat("edgeOscillation");
    return result;
}

Vector4 FatVertex::GetPackedTangentBinormal() const
{
    return Vector4(tangent_, DotProduct(CrossProduct(tangent_, normal_), binormal_) > 0 ? 1.0f : -1.0f);
}

FatVertex LerpVertices(const FatVertex& lhs, const FatVertex& rhs, float factor)
{
    FatVertex result;

    result.position_ = Lerp(lhs.position_, rhs.position_, factor);
    result.tangent_  = Lerp(lhs.tangent_, rhs.tangent_, factor);
    result.binormal_ = Lerp(lhs.binormal_, rhs.binormal_, factor);
    result.normal_   = Lerp(lhs.normal_, rhs.normal_, factor);
    for (unsigned i = 0; i < MAX_VERTEX_TEXCOORD; ++i)
    {
        result.uv_[i] = Lerp(lhs.uv_[i], rhs.uv_[i], factor);
    }
    for (unsigned i = 0; i < MAX_VERTEX_BONES; ++i)
    {
        result.boneIndices_[i] = lhs.boneIndices_[i];
        result.boneWeights_[i] = Urho3D::Lerp(lhs.boneWeights_[i], rhs.boneWeights_[i], factor);
    }
    result.mainAdherence_   = Urho3D::Lerp(lhs.mainAdherence_, rhs.mainAdherence_, factor);
    result.branchAdherence_ = Urho3D::Lerp(lhs.branchAdherence_, rhs.branchAdherence_, factor);
    result.phase_           = Urho3D::Lerp(lhs.phase_, rhs.phase_, factor);
    result.edgeOscillation_ = Urho3D::Lerp(lhs.edgeOscillation_, rhs.edgeOscillation_, factor);

    return result;
}

}

