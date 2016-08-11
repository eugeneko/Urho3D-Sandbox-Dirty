#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/Log.h>

namespace Urho3D
{

class Model;
struct VertexElement;
class XMLElement;

}

namespace FlexEngine
{

/// Maximum number of texture coordinates.
static const unsigned MAX_VERTEX_TEXCOORD = 4;

/// Maximum number of bones that affect one vertex.
static const unsigned MAX_VERTEX_BONES = 4;

/// Vertex that contain all attribute data with maximum precision.
struct DefaultVertex
{
    /// Vertex position.
    Vector3 position_;
    /// Normal of model geometry. Should not be used for lighting.
    Vector3 geometryNormal_;

    /// Tangent, X axis.
    Vector3 tangent_;
    /// Binormal, Y axis.
    Vector3 binormal_;
    /// Normal, Z axis.
    Vector3 normal_;

    /// Texture coordinates.
    Vector4 uv_[MAX_VERTEX_TEXCOORD];

    /// Skeleton bone indices.
    unsigned char boneIndices_[MAX_VERTEX_BONES];
    /// Skeleton bone weights.
    float boneWeights_[MAX_VERTEX_BONES];

    /// Get vertex elements.
    static const PODVector<VertexElement>& GetVertexElements();
    /// Construct from XML.
    static DefaultVertex ConstructFromXML(const XMLElement& element);
    /// Get packed tangent.
    Vector4 GetPackedTangentBinormal() const;
};

/// Interpolate between vertices.
DefaultVertex LerpVertices(const DefaultVertex& lhs, const DefaultVertex& rhs, float factor);

/// Interpolate between four vertices. @see QLerp
DefaultVertex QLerpVertices(const DefaultVertex& v0, const DefaultVertex& v1, const DefaultVertex& v2, const DefaultVertex& v3,
    const float factor1, const float factor2);

/// Add base index to each index.
template <class T>
void AdjustIndicesBase(T* indexData, unsigned numIndices, unsigned baseIndex)
{
    for (unsigned i = 0; i < numIndices; ++i)
    {
        indexData[i] += baseIndex;
    }
}

/// Add base index to each index.
void AdjustIndicesBase(unsigned char* indexData, unsigned indexDataSize, bool largeIndices, unsigned baseIndex);

/// Geometry buffer for model factory.
struct ModelGeometryBuffer
{
    /// Vertex data.
    PODVector<unsigned char> vertexData;
    /// Index data.
    PODVector<unsigned char> indexData;
};

/// Helper class for building model data with per-material geometry. Morphing and multiple vertex buffers are not supported.
class ModelFactory : public Object
{
    URHO3D_OBJECT(ModelFactory, Object);

public:
    /// Construct.
    ModelFactory(Context* context);
    /// Destruct.
    virtual ~ModelFactory();

    /// Reset all stored data.
    void Reset();
    /// Initialize model factory. Vertex and index format must be the same for whole model.
    void Initialize(const PODVector<VertexElement>& vertexElements, bool largeIndices);
    /// Set current level for further data write operations.
    void SetLevel(unsigned level);
    /// Set current material for further data write operations.
    void SetMaterial(SharedPtr<Material> material);
    /// Push vertex and index data.
    void Push(const void* vertexData, unsigned numVertices, const void* indexData, unsigned numIndices, bool adjustIndices);
    /// Push vertex and index data.
    template <class T, class U>
    void Push(const PODVector<T>& vertices, const PODVector<U>& indices, bool adjustIndices)
    {
        if (sizeof(T) != GetVertexSize())
        {
            URHO3D_LOGERROR("Invalid vertex format");
            return;
        }

        if (sizeof(U) != GetIndexSize())
        {
            URHO3D_LOGERROR("Invalid index format");
            return;
        }

        Push(vertices.Buffer(), vertices.Size(), indices.Buffer(), indices.Size(), adjustIndices);
    }
    /// Push vertex and index data.
    template <class T, unsigned N, class U, unsigned M>
    void Push(const T (&vertices)[N], const U (&indices)[M], bool adjustIndices)
    {
        if (sizeof(T) != GetVertexSize())
        {
            URHO3D_LOGERROR("Invalid vertex format");
            return;
        }

        if (sizeof(U) != GetIndexSize())
        {
            URHO3D_LOGERROR("Invalid index format");
            return;
        }

        Push(vertices, N, indices, M, adjustIndices);
    }

    /// Build model from stored data.
    SharedPtr<Model> BuildModel(const Vector<SharedPtr<Material>>& materials) const;

    /// Get vertex size.
    unsigned GetVertexSize() const { return vertexSize_; }
    /// Get index size.
    unsigned GetIndexSize() const { return largeIndices_ ? 4 : 2; }
    /// Get materials.
    Vector<SharedPtr<Material>> GetMaterials() const;

private:
    /// Vertex elements.
    PODVector<VertexElement> vertexElements_;
    /// Vertex size.
    unsigned vertexSize_ = 0;
    /// Set to use 32-bit indices.
    bool largeIndices_ = false;
    /// Current level.
    unsigned currentLevel_ = 0;
    /// Current material.
    SharedPtr<Material> currentMaterial_;
    /// Geometry container.
    using GeometryBufferMap = HashMap<SharedPtr<Material>, Vector<ModelGeometryBuffer>>;
    /// Geometry data grouped by material and levels.
    GeometryBufferMap geometry_;
};

}