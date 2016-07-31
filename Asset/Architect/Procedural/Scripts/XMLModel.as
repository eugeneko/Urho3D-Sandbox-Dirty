/// Model data in XML format.
class XMLModel
{
    /// Construct by passed root XML node.
    XMLModel(XMLElement root)
    {
        vertices_ = root.CreateChild("vertices");
        indices_ = root.CreateChild("indices");
        numVertices_ = 0;
    }
    
    /// Add vertex.
    void AddVertex(Vector3 position, Vector4 uv0, Vector4 uv1)
    {
        XMLElement vertex = vertices_.CreateChild("vertex");
        vertex.SetVector3("position", position);
        vertex.SetVector4("uv0", uv0);
        vertex.SetVector4("uv1", uv1);
        ++numVertices_;
    }

    /// Add triangle.
    void AddTriangle(uint base, uint i0, uint i1, uint i2)
    {
        XMLElement triangle = indices_.CreateChild("triangle");
        triangle.SetUInt("i0", base + i0);
        triangle.SetUInt("i1", base + i1);
        triangle.SetUInt("i2", base + i2);
    }

    /// Add rectangle.
    void AddRect(
        Vector3 posNN, Vector3 posNP, Vector3 posPN, Vector3 posPP,
        Vector4 texNN, Vector4 texNP, Vector4 texPN, Vector4 texPP,
        Vector4 param = Vector4())
    {
        uint baseVertex = numVertices_;
        AddVertex(posNN, texNN, param);
        AddVertex(posNP, texNP, param);
        AddVertex(posPN, texPN, param);
        AddVertex(posPP, texPP, param);
        AddTriangle(baseVertex, 0, 2, 3);
        AddTriangle(baseVertex, 0, 3, 1);
    }
    
    /// Add 2D rectangle.
    void AddRect2D(Vector3 position, float angle, Vector2 scale, Vector2 uv0, Vector2 uv1, Vector2 param0, Vector4 param1)
    {
        Matrix3 rotation = Quaternion(angle).rotationMatrix;
        Vector3 axisY = rotation * Vector3(0, 1, 0);
        Vector3 axisX = rotation * Vector3(1, 0, 0);
        
        Vector3 posN0 = position - axisX * 0.5 * scale.x;
        Vector3 posP0 = position + axisX * 0.5 * scale.x;
        Vector3 posN1 = position - axisX * 0.5 * scale.x + axisY * scale.y;
        Vector3 posP1 = position + axisX * 0.5 * scale.x + axisY * scale.y;
        
        Vector4 texNN = Vector4(uv0.x, uv0.y, param0.x, param0.y);
        Vector4 texNP = Vector4(uv1.x, uv0.y, param0.x, param0.y);
        Vector4 texPN = Vector4(uv0.x, uv1.y, param0.x, param0.y);
        Vector4 texPP = Vector4(uv1.x, uv1.y, param0.x, param0.y);
        
        AddRect(posN0, posP0, posN1, posP1, texNN, texNP, texPN, texPP, param1);
    }

    private XMLElement vertices_;
    private XMLElement indices_;
    private uint numVertices_;
}
