#include <FlexEngine/Factory/GeometryUtils.h>

#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Math/MathDefs.h>

namespace FlexEngine
{

BoundingBox CalculateBoundingBox(DefaultVertex vertices[], unsigned numVertices)
{
    assert(numVertices > 0);
    BoundingBox box;
    for (unsigned i = 0; i < numVertices; ++i)
    {
        box.Merge(vertices[i].position_);
    }
    return box;
}

void CalculateNormals(DefaultVertex vertices[], unsigned numVertices, const unsigned indices[], unsigned numTriangles)
{
    // Compute normals
    for (unsigned i = 0; i < numTriangles; ++i)
    {
        const unsigned a1 = indices[3 * i + 0];
        const unsigned a2 = indices[3 * i + 1];
        const unsigned a3 = indices[3 * i + 2];

        const Vector3 pos1 = vertices[a1].position_;
        const Vector3 pos2 = vertices[a2].position_;
        const Vector3 pos3 = vertices[a3].position_;
        const Vector3 normal = CrossProduct(pos2 - pos1, pos3 - pos1).Normalized();

        vertices[a1].normal_ += normal;
        vertices[a2].normal_ += normal;
        vertices[a3].normal_ += normal;
    }

    // Normalize normals
    for (unsigned i = 0; i < numVertices; ++i)
    {
        vertices[i].normal_.Normalize();
    }
}

void CalculateTangent(const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, Vector3& tangent, Vector3& binormal)
{
    const Vector3 vector1 = v1.position_ - v0.position_;
    const Vector3 vector2 = v2.position_ - v0.position_;

    const Vector4 uv1 = v1.uv_[0] - v0.uv_[0];
    const Vector4 uv2 = v2.uv_[0] - v0.uv_[0];

    const float cp = uv1.x_ * uv2.y_ - uv2.x_ * uv1.y_;
    if (Equals(cp, 0.0f))
    {
        tangent = Vector3::ZERO;
        binormal = Vector3::ZERO;
        return;
    }
    const float den = 1.0f / cp;

    tangent.x_ = (uv2.y_ * vector1.x_ - uv1.y_ * vector2.x_) * den;
    tangent.y_ = (uv2.y_ * vector1.y_ - uv1.y_ * vector2.y_) * den;
    tangent.z_ = (uv2.y_ * vector1.z_ - uv1.y_ * vector2.z_) * den;

    binormal.x_ = (uv1.x_ * vector2.x_ - uv2.x_ * vector1.x_) * den;
    binormal.y_ = (uv1.x_ * vector2.y_ - uv2.x_ * vector1.y_) * den;
    binormal.z_ = (uv1.x_ * vector2.z_ - uv2.x_ * vector1.z_) * den;
}

void CalculateTangents(DefaultVertex vertices[], unsigned numVertices, const unsigned indices[], unsigned numTriangles)
{
    // Compute
    for (unsigned i = 0; i < numTriangles; ++i)
    {
        const unsigned a1 = indices[3 * i + 0];
        const unsigned a2 = indices[3 * i + 1];
        const unsigned a3 = indices[3 * i + 2];
        Vector3 tangent, binormal;
        CalculateTangent(vertices[a1], vertices[a2], vertices[a3],
            tangent, binormal);

        vertices[a1].tangent_ += tangent;
        vertices[a2].tangent_ += tangent;
        vertices[a3].tangent_ += tangent;
        vertices[a1].binormal_ += binormal;
        vertices[a2].binormal_ += binormal;
        vertices[a3].binormal_ += binormal;
    }

    // Normalize
    for (unsigned i = 0; i < numVertices; ++i)
    {
        vertices[i].tangent_.Normalize();
        vertices[i].binormal_.Normalize();
    }
}

void AppendQuadToIndices(
    PODVector<unsigned>& indices, const unsigned base, unsigned v0, unsigned v1, unsigned v2, unsigned v3, bool flipped /*= false*/)
{
    if (!flipped)
    {
        indices.Push(base + v0);
        indices.Push(base + v2);
        indices.Push(base + v3);
        indices.Push(base + v0);
        indices.Push(base + v3);
        indices.Push(base + v1);
    }
    else
    {
        indices.Push(base + v0);
        indices.Push(base + v3);
        indices.Push(base + v2);
        indices.Push(base + v0);
        indices.Push(base + v1);
        indices.Push(base + v3);
    }
}

void AppendQuadToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3, bool flipped /*= false*/)
{
    const unsigned base = vertices.Size();

    // Append vertices
    vertices.Push(v0);
    vertices.Push(v1);
    vertices.Push(v2);
    vertices.Push(v3);

    // Append indices
    AppendQuadToIndices(indices, base, 0, 1, 2, 3, flipped);
}

void AppendQuadGridToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3,
    const unsigned numX, const unsigned numZ, bool flipped /*= false*/)
{
    const unsigned base = vertices.Size();

    // Generate vertices
    for (float j = 0; j <= numZ; ++j)
    {
        for (float i = 0; i <= numX; ++i)
        {
            vertices.Push(QLerpVertices(v0, v1, v2, v3, i / numX, j / numZ));
        }
    }

    // Generate indices
    for (unsigned j = 0; j < numZ; ++j)
    {
        for (unsigned i = 0; i < numX; ++i)
        {
            AppendQuadToIndices(indices, base,
                j * (numX + 1) + i,
                j * (numX + 1) + i + 1,
                (j + 1) * (numX + 1) + i,
                (j + 1) * (numX + 1) + i + 1,
                flipped);
        }
    }
}

void AppendGeometryToVertices(PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices,
    const PODVector<DefaultVertex>& newVertices, const PODVector<unsigned>& newIndices)
{
    const unsigned baseVertex = vertices.Size();
    const unsigned baseIndex = indices.Size();
    vertices += newVertices;
    indices += newIndices;
    for (unsigned i = baseIndex; i < indices.Size(); ++i)
    {
        indices[i] += baseVertex;
    }
}

}
