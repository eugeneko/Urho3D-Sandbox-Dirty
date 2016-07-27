#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>

namespace Urho3D
{

class XMLElement;

}

namespace FlexEngine
{

/// Maximum number of texture coordinates.
static const unsigned MAX_VERTEX_TEXCOORD = 8;

/// Maximum number of bones that affect one vertex.
static const unsigned MAX_VERTEX_BONES = 4;

/// Vertex that contain all attribute data in maximum precision.
struct FatVertex
{
    /// Vertex position.
    Vector3 position_;
    /// Normal of model geometry. Should not be used for lighting.
    Vector3 geometryNormal_;

    /// Tangent, X axis.
    Vector3 tangent_;
    /// Tangent, X axis.
    Vector3 binormal_;
    /// Tangent, X axis.
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

    /// Construct from XML.
    static FatVertex ConstructFromXML(const XMLElement& element);
    /// Get packed tangent.
    Vector4 GetPackedTangentBinormal() const;
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
