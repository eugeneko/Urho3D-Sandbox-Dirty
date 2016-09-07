/// Model factory wrapper.
class ModelFactoryWrapper
{
    /// Construct by factory.
    ModelFactoryWrapper(ModelFactory@ factory)
    {
        factory_ = factory;
    }
    
    /// Add vertex.
    void AddVertex(Vector3 position, Vector4 uv0, Vector4 uv1)
    {
        DefaultVertex vertex;
        vertex.position = position;
        vertex.uv[0] = uv0;
        vertex.uv[1] = uv1;
        factory_.PushVertex(vertex);
    }

    /// Add triangle.
    void AddTriangle(uint base, uint i0, uint i1, uint i2)
    {
        factory_.PushIndex(base + i0);
        factory_.PushIndex(base + i1);
        factory_.PushIndex(base + i2);
    }

    /// Add rectangle.
    void AddRect(
        Vector3 posNN, Vector3 posNP, Vector3 posPN, Vector3 posPP,
        Vector4 texNN, Vector4 texNP, Vector4 texPN, Vector4 texPP,
        Vector4 param = Vector4())
    {
        uint baseVertex = factory_.GetNumVerticesInBucket();
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

    private ModelFactory@ factory_;
}
