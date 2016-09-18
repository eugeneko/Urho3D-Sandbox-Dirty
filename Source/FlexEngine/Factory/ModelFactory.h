#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/Log.h>

namespace Urho3D
{

class Model;
class ScriptFile;
class XMLElement;
struct VertexElement;

}

namespace FlexEngine
{

/// Maximum number of texture coordinates per vertex.
static const unsigned MAX_VERTEX_TEXCOORD = 4;

/// Maximum number of colors per vertex.
static const unsigned MAX_VERTEX_COLOR = 4;

/// Maximum number of bones per vertex.
static const unsigned MAX_VERTEX_BONES = 4;

/// Vertex that contain all attribute data with maximum precision.
struct DefaultVertex
{
    /// Vertex position.
    Vector3 position_;

    /// Tangent, X axis.
    Vector3 tangent_;
    /// Binormal, Y axis.
    Vector3 binormal_;
    /// Normal, Z axis.
    Vector3 normal_;

    /// Texture coordinates.
    Vector4 uv_[MAX_VERTEX_TEXCOORD];
    /// Colors.
    Color colors_[MAX_VERTEX_COLOR];

    /// Skeleton bone indices.
    unsigned char boneIndices_[MAX_VERTEX_BONES];
    /// Skeleton bone weights.
    float boneWeights_[MAX_VERTEX_BONES];

    /// Get vertex elements.
    static const PODVector<VertexElement>& GetVertexElements();
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

    /// Add new geometry and set current material for further data write operations.
    void AddGeometry(SharedPtr<Material> material, bool allowReuse = true);
    /// Set current level for further data write operations.
    void SetLevel(unsigned level);
    /// Add nothing. This call just creates empty geometry level.
    void AddEmpty();
    /// Add vertex and index data.
    void AddPrimitives(const void* vertexData, unsigned numVertices, const void* indexData, unsigned numIndices, bool adjustIndices);
    /// Add vertex and index data.
    template <class T, class U>
    void AddPrimitives(const PODVector<T>& vertices, const PODVector<U>& indices, bool adjustIndices)
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

        AddPrimitives(vertices.Buffer(), vertices.Size(), indices.Buffer(), indices.Size(), adjustIndices);
    }
    /// Add vertex and index data.
    template <class T, unsigned N, class U, unsigned M>
    void AddPrimitives(const T (&vertices)[N], const U (&indices)[M], bool adjustIndices)
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

        AddPrimitives(vertices, N, indices, M, adjustIndices);
    }
    /// Add vertex.
    void AddVertex(const DefaultVertex& vertex) { AddPrimitives(&vertex, 1, nullptr, 0, false); }
    /// Add index.
    void AddIndex(unsigned index) { AddPrimitives(nullptr, 0, &index, 1, false); };
    /// Iterate over vertices.
    template <class T, class U>
    void ForEachVertex(U function)
    {
        for (unsigned i = 0; i < GetNumGeometries(); ++i)
        {
            for (unsigned j = 0; j < GetNumGeometryLevels(i); ++j)
            {
                const unsigned n = GetNumVertices(i, j);
                T* vertices = GetVertices<T>(i, j);
                for (unsigned k = 0; k < n; ++k)
                {
                    function(i, j, k, vertices[k]);
                }
            }
        }
    }

    /// Get vertex size.
    unsigned GetVertexSize() const { return vertexSize_; }
    /// Get index size.
    unsigned GetIndexSize() const { return largeIndices_ ? 4 : 2; }
    /// Get number of geometries.
    unsigned GetNumGeometries() const;
    /// Get number of levels.
    unsigned GetNumGeometryLevels(unsigned geometry) const;
    /// Get number of vertices.
    unsigned GetNumVertices(unsigned geometry, unsigned level) const;
    /// Get number of indices.
    unsigned GetNumIndices(unsigned geometry, unsigned level) const;
    /// Get vertex data.
    const void* GetVertices(unsigned geometry, unsigned level) const;
    /// Get index data.
    const void* GetIndices(unsigned geometry, unsigned level) const;
    /// Get vertex data.
    template <class T>
    T* GetVertices(unsigned geometry, unsigned level) { return const_cast<T*>(const_cast<const ModelFactory*>(this)->GetVertices<T>(geometry, level)); }
    /// Get vertex data.
    template <class T>
    const T* GetVertices(unsigned geometry, unsigned level) const { assert(sizeof(T) == GetVertexSize()); return static_cast<const T*>(GetVertices(geometry, level)); }
    /// Get number of vertices for current level and material.
    unsigned GetCurrentNumVertices() const;
    /// Get materials.
    const Vector<SharedPtr<Material>>& GetMaterials() const;

    /// Build model from stored data.
    SharedPtr<Model> BuildModel() const;

private:
    /// Vertex elements.
    PODVector<VertexElement> vertexElements_;
    /// Vertex size.
    unsigned vertexSize_ = 0;
    /// Set to use 32-bit indices.
    bool largeIndices_ = false;

    /// Current geometry.
    unsigned currentGeometry_ = 0;
    /// Current level.
    unsigned currentLevel_ = 0;
    /// Geometry data grouped by material and levels.
    Vector<Vector<ModelGeometryBuffer>> geometry_;
    /// Materials of geometries.
    Vector<SharedPtr<Material>> materials_;
};

/// Create model from script.
SharedPtr<ModelFactory> CreateModelFromScript(ScriptFile& scriptFile, const String& entryPoint);

/// Create default quad model.
SharedPtr<Model> CreateQuadModel(Context* context);

/// Create default quad model or get it from global context variable 'DefaultRenderTargetModel'.
SharedPtr<Model> GetOrCreateQuadModel(Context* context);

/// Append one model geometries to another.
void AppendModelGeometries(Model& dest, const Model& source);

/// Add empty LOD level for each model geometry.
void AppendEmptyLOD(Model& model, float distance);

}
