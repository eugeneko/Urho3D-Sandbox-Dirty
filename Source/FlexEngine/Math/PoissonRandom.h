#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Ptr.h>

namespace FlexEngine
{

/// @brief Point Cloud 2D
using PointCloud2D = PODVector<Vector2>;
/// @brief Point Cloud 2D (normalized)
using PointCloud2DNorm = PODVector<Vector2>;
/// @brief Sample Point Cloud
/// @pre @a cloud should be normalized to [0, 1] range
PointCloud2D samplePointCloud(const PointCloud2DNorm& cloud,
    const Vector2& begin, const Vector2& end,
    float scale);
/// @brief Poisson random generator
class PoissonRandom
{
public:
    /// @brief Ctor
    PoissonRandom(unsigned seed);
    /// @brief Dtor
    ~PoissonRandom();
    /// @brief Generate
    PointCloud2DNorm generate(float minDist, unsigned newPointsCount, unsigned numPoints);
private:
    struct Cell;
    struct Grid;
    float randomFloat();
    static IntVector2 imageToGrid(const Vector2& p, float cellSize);
    Vector2 popRandom(PointCloud2DNorm& points);
    Vector2 generateRandomPointAround(const Vector2& p, float minDist);
private:
    struct Core;
    UniquePtr<Core> impl_;
};

}

