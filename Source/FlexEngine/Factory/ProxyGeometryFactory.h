#include <FlexEngine/Common.h>
#include <FlexEngine/Container/Ptr.h>
#include <FlexEngine/Factory/FatVertex.h>

namespace Urho3D
{

class BoundingBox;
class XMLElement;

}

namespace FlexEngine
{

struct OrthoCameraDescription;

/// Parameters of cylinder proxy geometry.
struct CylinderProxyParameters
{
    /// Number of vertical surfaces of proxy geometry, front and back are considered as different surfaces.
    unsigned numSurfaces_;
    /// Number of vertical segments of each surface.
    unsigned numVertSegments_;
    /// Angle of second layer measured from horizontal plane.
    float diagonalAngle_;
};

/// Generate cylinder proxy geometry and cameras.
void GenerateCylinderProxy(const BoundingBox& boundingBox, const CylinderProxyParameters& param, unsigned width, unsigned height,
    Vector<OrthoCameraDescription>& cameras, Vector<FatVertex>& vertices, Vector<FatIndex>& indices);

/// Generate proxy geometry and cameras from XML.
void GenerateProxyFromXML(const BoundingBox& boundingBox, unsigned width, unsigned height, const XMLElement& node,
    Vector<OrthoCameraDescription>& cameras, Vector<FatVertex>& vertices, Vector<FatIndex>& indices);

/// Generate proxy cameras from XML.
Vector<OrthoCameraDescription> GenerateProxyCamerasFromXML(
    const BoundingBox& boundingBox, unsigned width, unsigned height, const XMLElement& node);

}
