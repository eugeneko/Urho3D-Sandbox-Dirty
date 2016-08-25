#include <FlexEngine/Factory/ProxyGeometryFactory.h>

#include <FlexEngine/Factory/GeometryUtils.h>
#include <FlexEngine/Factory/ModelFactory.h>
#include <FlexEngine/Factory/TextureFactory.h>
#include <FlexEngine/Math/MathDefs.h>
#include <FlexEngine/Resource/XMLHelpers.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

namespace
{

/// Compute diagonal height of w*w*h box with specified diagonal angle.
float ComputeDiagonalHeight(float width, float height, float skewAngle)
{
    const Vector2 axis = Vector2(Sin(skewAngle), Cos(skewAngle));
    const Vector2 size = Vector2(width, height);
    return ProjectOntoAxis(axis, size);
}

/// Expand region to meet the width/height ratio.
Vector2 ExpandRegionToMeetRatio(const Vector2& region, float ratio)
{
    Vector2 ret;
    ret.x_ = Max(region.x_, ratio * region.y_);
    ret.y_ = ret.x_ / ratio;
    return ret;
}

/// Convert float to integer texture coordinates (1D).
int ConvertTexCoordToViewport(float uv, int size)
{
    return static_cast<int>(Round(uv * size));
}

/// Convert float to integer texture coordinates (2D).
IntVector2 ConvertTexCoordToViewport(const Vector2& uv, const IntVector2& size)
{
    return IntVector2(ConvertTexCoordToViewport(uv.x_, size.x_), ConvertTexCoordToViewport(uv.y_, size.y_));
}

}

void GenerateCylinderProxy(const BoundingBox& boundingBox, const CylinderProxyParameters& param, unsigned width, unsigned height,
    Vector<OrthoCameraDescription>& cameras, PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices)
{
    // Compute extents of bounding box
    const Vector3 boxSize = boundingBox.Size();
    const float boxWidth  = Max(boxSize.x_, boxSize.z_);
    const float boxHeight = Max(boxWidth, boxSize.y_);

    // Compute dimensions
    const float boxDiagonalHeight = ComputeDiagonalHeight(boxWidth, boxHeight, param.diagonalAngle_);
    const Vector2 totalSize = Vector2(boxWidth * param.numSurfaces_, boxHeight + boxDiagonalHeight);
    const float aspectRatio = totalSize.x_ / totalSize.y_;
    const IntVector2 dimensions = IntVector2(static_cast<int>(width), static_cast<int>(height));
    const float textureScale = static_cast<float>(width) / height;
    const Vector2 fixedTotalSize = ExpandRegionToMeetRatio(totalSize, textureScale);

    // Generate vertical slices
    for (unsigned isDiagonal = 0; isDiagonal < 2; ++isDiagonal)
    {
        // Size of slice
        const float boxSliceHeight = !isDiagonal ? boxHeight : boxDiagonalHeight;
        const float boxHalfWidth = boxWidth / 2.0f;
        const float boxHalfHeight = boxSliceHeight / 2.0f;
        const float boxHalfDepth = Vector2(boxHalfWidth, boxHalfHeight).Length();

        // UV base
        const float baseV = !isDiagonal ? boxDiagonalHeight : 0.0f;

        // Rotation around X axis
        const float angleX = !isDiagonal ? 0.0f : param.diagonalAngle_;

        for (unsigned surface = 0; surface < param.numSurfaces_; ++surface)
        {
            // Rotation angle and axises
            const float angleY = 360.0f * surface / param.numSurfaces_;
            const Vector3 axisX = Vector3(Cos(angleY), 0.0f, Sin(angleY));
            const Vector3 axisFlatZ = CrossProduct(axisX, Vector3(0.0f, 1.0f, 0.0f)).Normalized();
            const Vector3 axisY = Vector3(Sin(angleX) * axisFlatZ.x_, Cos(angleX), Sin(angleX) * axisFlatZ.z_);
            const Vector3 axisZ = CrossProduct(axisX, axisY).Normalized();

            // Compute UVs
            const Vector2 textureBegin = Vector2(surface * boxWidth, baseV) / fixedTotalSize;
            const Vector2 textureEnd   = Vector2((surface + 1) * boxWidth, baseV + boxSliceHeight) / fixedTotalSize;
            const IntVector2 viewportBegin = ConvertTexCoordToViewport(textureBegin, dimensions);
            const IntVector2 viewportEnd   = ConvertTexCoordToViewport(textureEnd, dimensions);

            // Compute camera
            OrthoCameraDescription cameraDesc;
            cameraDesc.rotation_ = Quaternion(axisX, axisY, axisZ);
            cameraDesc.position_ = boundingBox.Center() - axisZ * boxHalfDepth;
            cameraDesc.farClip_  = 2.0f * boxHalfDepth;
            cameraDesc.size_     = 2.0f * Vector2(boxHalfWidth, boxHalfHeight);
            cameraDesc.viewport_ = IntRect(viewportBegin.x_, viewportBegin.y_, viewportEnd.x_, viewportEnd.y_);
            cameras.Push(cameraDesc);

            // Compute normal
            const Vector3 normal = cameraDesc.rotation_.Inverse().RotationMatrix() * Vector3(0.0f, 0.0f, -1.0f);

            // Generate geometry data
            DefaultVertex verts[4];

            verts[0].position_ = boundingBox.Center() - axisX * boxHalfWidth - axisY * boxHalfHeight;
            verts[1].position_ = boundingBox.Center() + axisX * boxHalfWidth - axisY * boxHalfHeight;
            verts[2].position_ = boundingBox.Center() - axisX * boxHalfWidth + axisY * boxHalfHeight;
            verts[3].position_ = boundingBox.Center() + axisX * boxHalfWidth + axisY * boxHalfHeight;

            verts[0].uv_[0] = Vector4(textureBegin.x_, textureEnd.y_,   0, 0);
            verts[1].uv_[0] = Vector4(textureEnd.x_,   textureEnd.y_,   0, 0);
            verts[2].uv_[0] = Vector4(textureBegin.x_, textureBegin.y_, 0, 0);
            verts[3].uv_[0] = Vector4(textureEnd.x_,   textureBegin.y_, 0, 0);

            for (DefaultVertex& v : verts)
            {
//                 v.mainAdherence_ = UnLerpClamped(boundingBox.min_.y_, boundingBox.max_.y_, v.position_.y_);
//                 v.branchAdherence_ = 0.0f;
//                 v.phase_ = 0.0f;
//                 v.edgeOscillation_ = 0.0f;

                v.normal_ = normal;
            }

            // Generate quads
            AppendQuadGridToVertices(vertices, indices, verts[0], verts[1], verts[2], verts[3], 1, param.numVertSegments_);
        }
    }
}

void GeneratePlainProxy(const BoundingBox& boundingBox, unsigned width, unsigned height,
    Vector<OrthoCameraDescription>& cameras, PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices)
{
    const Vector3 center = boundingBox.Center();
    const Vector3 size = boundingBox.Size();

    OrthoCameraDescription cameraDesc;
    cameraDesc.position_ = Vector3(center.x_, center.y_, boundingBox.min_.z_);
    cameraDesc.farClip_ = size.z_;
    cameraDesc.size_ = Vector2(size.x_, size.y_);
    cameraDesc.viewport_ = IntRect(0, 0, width, height);
    cameras.Push(cameraDesc);

    // #TODO Add geometry generation
}

void GenerateProxyFromXML(const BoundingBox& boundingBox, unsigned width, unsigned height, const XMLElement& node,
    Vector<OrthoCameraDescription>& cameras, PODVector<DefaultVertex>& vertices, PODVector<unsigned>& indices)
{
    const String type = node.GetAttribute("type");
    if (type.Compare("CylinderProxy", false) == 0)
    {
        CylinderProxyParameters params;
        params.numSurfaces_ = node.GetUInt("numSurfaces");
        params.numVertSegments_ = node.GetUInt("numVertSegments");
        params.diagonalAngle_ = node.GetFloat("diagonalAngle");
        GenerateCylinderProxy(boundingBox, params, width, height, cameras, vertices, indices);
    }
    else if (type.Compare("BoundingBox", false) == 0)
    {
        BoundingBox newBoundingBox = boundingBox;
        if (node.HasAttribute("min"))
        {
            newBoundingBox.min_ = node.GetVector3("min");
        }
        if (node.HasAttribute("max"))
        {
            newBoundingBox.max_ = node.GetVector3("max");
        }
        GeneratePlainProxy(newBoundingBox, width, height, cameras, vertices, indices);
    }
    else
    {
        URHO3D_LOGERRORF("Proxy has unknown pattern '%s'", type.CString());
    }
}

Vector<OrthoCameraDescription> GenerateProxyCamerasFromXML(
    const BoundingBox& boundingBox, unsigned width, unsigned height, const XMLElement& node)
{
    Vector<OrthoCameraDescription> cameras;
    PODVector<DefaultVertex> vertices;
    PODVector<unsigned> indices;
    GenerateProxyFromXML(boundingBox, width, height, node, cameras, vertices, indices);
    return cameras;
}

}
