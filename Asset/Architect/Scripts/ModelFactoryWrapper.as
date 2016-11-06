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

    /// Add quad.
    void AddQuad(
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

    private ModelFactory@ factory_;
}

/// 2D quad description.
class Quad
{
    Vector3 position;
    float angle;
    Vector2 size;
    Rect uv;
    Vector2 param;
    Vector4 color;
}

/// List of quads.
class QuadList
{
    Array<Quad> quads;
    
    void AddQuad(Quad quad)
    {
        quads.Push(quad);
    }
    
    void AddQuad(Vector3 position, float angle, Vector2 size, Vector2 uv0, Vector2 uv1, Vector2 param, Vector4 color)
    {
        Quad quad;
        quad.position = position;
        quad.angle = angle;
        quad.size = size;
        quad.uv = Rect(uv0, uv1);
        quad.param = param;
        quad.color = color;
        AddQuad(quad);
    }
    
    void Deploy(ModelFactoryWrapper@ dest)
    {
        for (uint i = 0; i < quads.length; ++i)
        {
            Quad quad = quads[i];
            
            Matrix3 rotation = Quaternion(quad.angle).rotationMatrix;
            Vector3 axisY = rotation * Vector3(0, 1, 0);
            Vector3 axisX = rotation * Vector3(1, 0, 0);
            
            Vector3 posN0 = quad.position - axisX * 0.5 * quad.size.x;
            Vector3 posP0 = quad.position + axisX * 0.5 * quad.size.x;
            Vector3 posN1 = quad.position - axisX * 0.5 * quad.size.x + axisY * quad.size.y;
            Vector3 posP1 = quad.position + axisX * 0.5 * quad.size.x + axisY * quad.size.y;
            
            Vector4 texNN = Vector4(quad.uv.min.x, quad.uv.min.y, quad.param.x, quad.param.y);
            Vector4 texNP = Vector4(quad.uv.max.x, quad.uv.min.y, quad.param.x, quad.param.y);
            Vector4 texPN = Vector4(quad.uv.min.x, quad.uv.max.y, quad.param.x, quad.param.y);
            Vector4 texPP = Vector4(quad.uv.max.x, quad.uv.max.y, quad.param.x, quad.param.y);
            
            dest.AddQuad(posN0, posP0, posN1, posP1, texNN, texNP, texPN, texPP, quad.color);
        }
    }
}

/// vec2 -> vec2 random.
Vector2 StableRandom2(Vector2 uv)
{
    return Vector2(StableRandom(uv), StableRandom(uv + Vector2(1, 1)));
}

/// vec2 -> vec3 random.
Vector3 StableRandom3(Vector2 uv)
{
    return Vector3(StableRandom(uv), StableRandom(uv + Vector2(1, 1)), StableRandom(uv + Vector2(2, 2)));
}

/// vec2 -> vec4 random.
Vector4 StableRandom4(Vector2 uv)
{
    return Vector4(StableRandom(uv), StableRandom(uv + Vector2(1, 1)), StableRandom(uv + Vector2(2, 2)), StableRandom(uv + Vector2(3, 3)));
}

/// Create model from quad list.
Model@ CreateModel(ProceduralContext@ context, QuadList@ quadList)
{
    ModelFactory@ factory = context.CreateModelFactory();
    ModelFactoryWrapper model(factory);
    quadList.Deploy(model);
    return context.CreateModel(factory);
}

QuadList@ FlipVertical(QuadList@ src)
{
    QuadList@ dest = QuadList();
    
    dest.quads.Reserve(src.quads.length);
    for (uint i = 0; i < src.quads.length; ++i)
    {
        Quad quad = src.quads[i];
        quad.position.y = -quad.position.y;
        quad.angle = 180 - quad.angle;
        dest.AddQuad(quad);
    }
    
    return dest;
}

QuadList@ GenerateQuadStrips(Vector2 quadSize, Vector2 range, Vector2 stride, Vector2 noise)
{
    QuadList@ dest = QuadList();

    for (float x = -range.x/2; x <= range.x/2; x += stride.x)
    {
        for (float y = range.y/2; y >= -range.y/2; y -= stride.y)
        {
            Vector2 position = Vector2(x, y);
            position += (StableRandom2(position) - Vector2(0.5, 0.5)) * noise;
            dest.AddQuad(Vector3(position, StableRandom(position) * 0.1), 0.0, quadSize, Vector2(0, 0), Vector2(1, 1), Vector2(0, 1), Vector4());
        }
    }

    return dest;
}

QuadList@ FillRandomFactor(QuadList@ src, float seed)
{
    QuadList@ dest = QuadList();
    
    dest.quads.Reserve(src.quads.length);
    for (uint i = 0; i < src.quads.length; ++i)
    {
        Quad quad = src.quads[i];
        quad.param.y = StableRandom(Vector2(quad.position.x + seed, quad.position.y + seed));
        dest.AddQuad(quad);
    }
    
    return dest;
}

QuadList@ FillRandomNormal(QuadList@ src, float seed, Vector2 deviation)
{
    QuadList@ dest = QuadList();
    
    dest.quads.Reserve(src.quads.length);
    for (uint i = 0; i < src.quads.length; ++i)
    {
        Quad quad = src.quads[i];
        Vector2 noise = StableRandom2(Vector2(quad.position.x + seed, quad.position.y + seed)) * 2 - Vector2(1, 1);
        Vector3 normal = Vector3(noise * deviation, 1.0).Normalized();
        quad.color = Vector4(normal * 0.5 + Vector3(0.5, 0.5, 0.5), 1.0);
        dest.AddQuad(quad);
    }
    
    return dest;
}

QuadList@ FilterRoundQuadArea(QuadList@ src, Vector2 range, float radius, Vector2 fade)
{
    QuadList@ dest = QuadList();

    dest.quads.Reserve(src.quads.length);
    for (uint i = 0; i < src.quads.length; ++i)
    {
        Vector3 position = src.quads[i].position;
        Vector2 relative = VectorMax(Vector2(Abs(position.x), Abs(position.y)) - VectorMax(range / 2 - Vector2(radius, radius), Vector2(0, 0)), Vector2(0, 0));
        float distance = relative.length;
        float fadeOut = Clamp(InverseLerp(fade.x, fade.y, distance), 0.0, 1.0);
        if (StableRandom(position) >= fadeOut)
        {
            dest.AddQuad(src.quads[i]);
        }
    }
    
    return dest;
}

QuadList@ FilterNoiseGrad(QuadList@ src, Vector2 grad0, Vector2 grad1)
{
    Vector2 dir = grad1 - grad0;
    QuadList@ dest = QuadList();

    dest.quads.Reserve(src.quads.length);
    for (uint i = 0; i < src.quads.length; ++i)
    {
        Vector3 position = src.quads[i].position;
        float grad = Clamp(Vector2(position.x - grad0.x, position.y - grad0.y).ProjectOntoAxis(dir) / dir.length, 0.0, 1.0);
        if (StableRandom(position) < 1.0 - grad)
        {
            dest.AddQuad(src.quads[i]);
        }
    }
    
    return dest;
}
