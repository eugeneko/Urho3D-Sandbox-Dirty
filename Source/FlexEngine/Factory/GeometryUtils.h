#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Math/BoundingBox.h>

namespace Urho3D
{

namespace FlexEngine
{

struct DefaultVertex;

/// Calculate Bounding Box.
BoundingBox CalculateBoundingBox(DefaultVertex vertices[], unsigned numVertices);

/// Calculate normals.
void CalculateNormals(DefaultVertex vertices[], unsigned numVertices, const unsigned indices[], unsigned numTriangles);

/// Calculate normals.
inline void CalculateNormals(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices)
{
    CalculateNormals(vertices.Buffer(), vertices.Size(), indices.Buffer(), indices.Size() / 3);
}

/// Calculate triangle tangent space.
void CalculateTangent(const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, Vector3& tangent, Vector3& binormal);

/// Calculate mesh tangent space.
void CalculateTangents(DefaultVertex vertices[], unsigned numVertices, const unsigned indices[], unsigned numTriangles);

/// Calculate mesh tangent space.
inline void CalculateTangents(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices)
{
    CalculateTangents(vertices.Buffer(), vertices.Size(), indices.Buffer(), indices.Size() / 3);
}

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
void AppendQuadToIndices(PODVector<unsigned>& indices,
    const unsigned base, unsigned v0, unsigned v1, unsigned v2, unsigned v3, bool flipped = false);

/// Append quad as pair of triangles to index and vertex data.
/// @see AppendQuadToIndices
void AppendQuadToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3, bool flipped = false);

/// Append quad grid as triangle list to index and vertex data.
/// @see AppendQuadToIndices
void AppendQuadGridToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3,
    const unsigned numX, const unsigned numZ, bool flipped = false);

/// Append geometry to index and vertex data. Indexes are adjusted.
void AppendGeometryToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const PODVector<DefaultVertex>& newVertices, const PODVector<unsigned>& newIndices);

}

}
