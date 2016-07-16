#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace FlexEngine
{

/// Maximum number of texture coordinates.
static const unsigned MAX_VERTEX_TEXCOORD = 8;

/// Maximum number of bones that affect one vertex.
static const unsigned MAX_VERTEX_BONES = 4;

/// Vertex that contain all attribute data in maximum precision.
struct FatVertex
{
    Vector3 position_;

    Vector3 tangent_;
    Vector3 binormal_;
    Vector3 normal_;

    Vector4 uv_[MAX_VERTEX_TEXCOORD];

    int boneIndices_[MAX_VERTEX_BONES];
    float boneWeights_[MAX_VERTEX_BONES];

    /// @name Wind info
    /// @{
    float mainAdherence_;   ///< Ground vertex adherence
    float branchAdherence_; ///< Branch vertex adherence
    float phase_;           ///< Phase of oscillations
    float edgeOscillation_; ///< Edge oscillation factor
    /// @}
};

/// Interpolate between fat vertices.
FatVertex LerpVertices(const FatVertex& lhs, const FatVertex& rhs, float factor);

/// Index of maximum size.
using FatIndex = unsigned;

}

namespace Urho3D
{

template <>
inline FlexEngine::FatVertex Lerp(FlexEngine::FatVertex lhs, FlexEngine::FatVertex rhs, float t)
{ 
    return LerpVertices(lhs, rhs, t);
}

}
