#include <FlexEngine/Graphics/GrassPatch.h>

#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Math/PoissonRandom.h>
#include <FlexEngine/Math/StandardRandom.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/WorkQueue.h>
// #include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Material.h>
// #include <Urho3D/Graphics/Model.h>
// #include <Urho3D/Graphics/DebugRenderer.h>
// #include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
// #include <Urho3D/Resource/ResourceCache.h>
// #include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

namespace
{

/// Distance between points in sample.
static const float samplePointsDistance = 0.05f;
/// Number of points in sample (upper bound).
static const unsigned samplePointsLimit = 10000;
/// Max number of iterations for sample generation.
static const unsigned samplePointsMaxIterations = 30;
/// Factor of extra buffer allocation.
static const float allocationFactor = 1.1f;

}

GrassPatch::GrassPatch(Context* context)
    : Drawable(context, DRAWABLE_GEOMETRY)
    , geometry_(MakeShared<Geometry>(context_))
    , vertexBuffer_(MakeShared<VertexBuffer>(context_))
    , indexBuffer_(MakeShared<IndexBuffer>(context_))
    , instanceData_(1, 1, 0, 0)
{
    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);

    batches_.Resize(1);
    batches_[0].geometry_ = geometry_;
    batches_[0].geometryType_ = GEOM_STATIC;
    batches_[0].numWorldTransforms_ = 1;
    batches_[0].instancingData_ = &instanceData_;
}

GrassPatch::~GrassPatch()
{

}

void GrassPatch::RegisterObject(Context* context)
{
    context->RegisterFactory<GrassPatch>(FLEXENGINE_CATEGORY);
}

void GrassPatch::ApplyAttributes()
{
}

void GrassPatch::SetPattern(float scale, const PODVector<Vector2>& pattern)
{
    pattern_ = &pattern;
    patternScale_ = scale;
}

void GrassPatch::SetRange(const Vector3& origin, const Rect& localRange)
{
    localRange_ = localRange;
    origin_ = origin;
}

void GrassPatch::SetMaterial(Material* material)
{
    batches_[0].material_ = material;
}

void GrassPatch::UpdateBatches(const FrameInfo& frame)
{
    batches_[0].distance_ = 0.0f;
    batches_[0].worldTransform_ = node_ ? &node_->GetWorldTransform() : (const Matrix3x4*)0;
}

void GrassPatch::SetWorkItem(SharedPtr<WorkItem> item)
{
    workItem_ = item;
}

SharedPtr<WorkItem> GrassPatch::GetWorkItem() const
{
    return workItem_;
}

void GrassPatch::UpdatePatch(Terrain& terrain)
{
    // Sample points
    // #TODO Unhardcode
    PODVector<Vector2> points = samplePointCloud(*pattern_, localRange_.min_, localRange_.max_, patternScale_);
    const unsigned numBillboards = points.Size();
    PODVector<Vector3> positions(numBillboards);
    PODVector<Vector3> normals(numBillboards);
    PODVector<Quaternion> rotations(numBillboards);
    StandardRandom generator(0);
    for (unsigned i = 0; i < numBillboards; ++i)
    {
        positions[i].x_ = points[i].x_;
        positions[i].z_ = points[i].y_;
        positions[i].y_ = terrain.GetHeight(positions[i] + origin_);
        normals[i] = terrain.GetNormal(positions[i] + origin_);
        rotations[i] = Quaternion(Vector3::UP, normals[i]) * Quaternion(generator.FloatFrom01() * 360, Vector3::UP);
        positions[i] = positions[i] + origin_ - node_->GetPosition();
    }

    // Allocate buffers if needed
    if (vertexData_.Size() < numBillboards * 4 * 8)
        vertexData_.Resize(numBillboards * 4 * 8);

    if (indexData_.Size() < numBillboards * 6)
        indexData_.Resize(numBillboards * 6);

    // Update index data
    unsigned short* indexPtr = indexData_.Buffer();
    unsigned vertexIndex = 0;
    for (unsigned i = 0; i < numBillboards; ++i)
    {
        indexPtr[0] = (unsigned short)vertexIndex;
        indexPtr[1] = (unsigned short)(vertexIndex + 1);
        indexPtr[2] = (unsigned short)(vertexIndex + 2);
        indexPtr[3] = (unsigned short)(vertexIndex + 2);
        indexPtr[4] = (unsigned short)(vertexIndex + 3);
        indexPtr[5] = (unsigned short)vertexIndex;

        indexPtr += 6;
        vertexIndex += 4;
    }

    // Update vertex data
    boundingBox_.Clear();
    float* vertexPtr = vertexData_.Buffer();
    for (unsigned i = 0; i < numBillboards; ++i)
    {
        const Matrix3 rotationMatrix = rotations[i].RotationMatrix();
        const Vector3 xAxis = GetBasisX(rotationMatrix);
        const Vector3 yAxis = GetBasisY(rotationMatrix);
        static const Vector2 uvs[4] = { Vector2::UP, Vector2::ONE, Vector2::RIGHT, Vector2::ZERO };
        for (unsigned j = 0; j < 4; ++j)
        {
            const Vector3 pos = positions[i] + xAxis * (uvs[j].x_ - 0.5f) + yAxis * (1.0f - uvs[j].y_);
            boundingBox_.Merge(pos);
            vertexPtr[0] = pos.x_;
            vertexPtr[1] = pos.y_;
            vertexPtr[2] = pos.z_;
            vertexPtr[3] = normals[i].x_;
            vertexPtr[4] = normals[i].y_;
            vertexPtr[5] = normals[i].z_;
            vertexPtr[6] = uvs[j].x_;
            vertexPtr[7] = uvs[j].y_;
            vertexPtr += 8;
        }
    }
    OnMarkedDirty(node_);
}

void GrassPatch::FinishUpdatePatch()
{
    if (vertexBuffer_->GetVertexCount() < vertexData_.Size() / 8)
        vertexBuffer_->SetSize(static_cast<unsigned>(vertexData_.Size() / 8),
            MASK_POSITION | MASK_NORMAL /*| MASK_COLOR*/ | MASK_TEXCOORD1 /*| MASK_TEXCOORD2*/, true);

    vertexBuffer_->SetData(vertexData_.Buffer());

    if (indexBuffer_->GetIndexCount() < indexData_.Size())
        indexBuffer_->SetSize(static_cast<unsigned>(indexData_.Size()), false, true);

    indexBuffer_->SetData(indexData_.Buffer());

    geometry_->SetDrawRange(TRIANGLE_LIST, 0, indexData_.Size(), false);
}

void GrassPatch::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

}
