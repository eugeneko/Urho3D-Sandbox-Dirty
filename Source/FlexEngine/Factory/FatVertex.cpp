#include <FlexEngine/Factory/FatVertex.h>

namespace FlexEngine
{

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

