#pragma once

#include <FlexEngine/Common.h>
#include <FlexEngine/Factory/FatVertex.h>

#include <Urho3D/Math/BoundingBox.h>

namespace FlexEngine
{

/// Calculate Bounding Box.
BoundingBox CalculateBoundingBox(FatVertex vertices[], unsigned numVertices);

/// Calculate normals.
void CalculateNormals(FatVertex vertices[], unsigned numVertices, const FatIndex indices[], unsigned numTriangles);

/// Calculate triangle tangent space.
void CalculateTangent(const FatVertex& v0, const FatVertex& v1, const FatVertex& v2, Vector3& tangent, Vector3& binormal);

/// Calculate mesh tangent space.
void CalculateTangents(FatVertex vertices[], unsigned numVertices, const FatIndex indices[], unsigned numTriangles);

/// Append quad as pair of triangles to index data.
///
/// Orientation:
/// @code
///     z
/// y   /_____ 
/// |  /2   3/
/// | /     /
/// |/_____/__x
///  0   1
///  
/// ^ is normal
/// v is flipped
/// @endcode
void AppendQuadToIndices(Vector<FatIndex>& indices,
    const FatIndex base, FatIndex v0, FatIndex v1, FatIndex v2, FatIndex v3, bool flipped = false);

/// Append quad as pair of triangles to index and vertex data.
/// @see AppendQuadToIndices
void AppendQuadToVertices(Vector<FatVertex>& vertices, Vector<FatIndex>& indices,
    const FatVertex& v0, const FatVertex& v1, const FatVertex& v2, const FatVertex& v3, bool flipped = false);

/// Append quad grid as triangle list to index and vertex data.
/// @see AppendQuadToIndices
void AppendQuadGridToVertices(Vector<FatVertex>& vertices, Vector<FatIndex>& indices,
    const FatVertex& v0, const FatVertex& v1, const FatVertex& v2, const FatVertex& v3,
    const unsigned numX, const unsigned numZ, bool flipped = false);

}
