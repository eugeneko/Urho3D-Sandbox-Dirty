#include "ModelFactoryWrapper.as"

void Main(ModelFactory@ dest)
{
    ModelFactoryWrapper model(dest);

    model.AddRect2D(Vector3(0.5, 0, 0.5), 0.0, Vector2(1, 1), Vector2(0, 1), Vector2(1, 0), Vector2(0, 1), Vector4());
}
