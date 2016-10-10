#include <FlexEngine/Math/PoissonRandom.h>

#include <FlexEngine/Math/MathDefs.h>

#include <random>

namespace FlexEngine
{

struct PoissonRandom::Core
{
    Core(unsigned seed) : m_generator(seed), m_distr(0.0, 1.0) {}
    std::mt19937 m_generator;
    std::uniform_real_distribution<> m_distr;
};

// Point Cloud
PointCloud2D samplePointCloud(const PointCloud2DNorm& cloud,
    const Vector2& begin, const Vector2& end,
    float scale)
{
    PointCloud2D dest;
    const Vector2 from = VectorFloor(begin / scale);
    const Vector2 to = VectorCeil(end / scale);
    for (float nx = from.x_; nx <= to.x_; ++nx)
    {
        for (float ny = from.y_; ny <= to.y_; ++ny)
        {
            const Vector2 tileBegin = Vector2(nx, ny);
            const Vector2 tileEnd = Vector2(nx + 1, ny + 1);
            const Vector2 clipBegin = VectorMax(begin / scale, VectorMin(end / scale, tileBegin));
            const Vector2 clipEnd = VectorMax(begin / scale, VectorMin(end / scale, tileEnd));
            for (const Vector2& sourcePoint : cloud)
            {
                const Vector2 point = VectorLerp(tileBegin, tileEnd, static_cast<Vector2>(sourcePoint));
                if (point.x_ < clipBegin.x_ || point.y_ < clipBegin.y_ || point.x_ > clipEnd.x_ || point.y_ > clipEnd.y_)
                    continue;

                dest.Push(point * scale);
            }
        }
    }
    return dest;
}


// Ctor
PoissonRandom::PoissonRandom(unsigned seed)
    : impl_(MakeUnique<Core>(seed))
{
}
PoissonRandom::~PoissonRandom()
{
}


// Generator
struct PoissonRandom::Cell
    : public Vector2
{
    bool isValid = false;
};
float PoissonRandom::randomFloat()
{
    return static_cast<float>(impl_->m_distr(impl_->m_generator));
}
IntVector2 PoissonRandom::imageToGrid(const Vector2& p, float cellSize)
{
    return IntVector2(static_cast<int>(p.x_ / cellSize), static_cast<int>(p.y_ / cellSize));

}
struct PoissonRandom::Grid
{
    Grid(const int w, const int h, const float cellSize)
        : m_width(w)
        , m_height(h)
        , m_cellSize(cellSize)
    {
        m_grid.Resize(m_height);

        for (auto i = m_grid.Begin(); i != m_grid.End(); i++)
        {
            i->Resize(m_width);
        }
    }
    void insert(const Vector2& p)
    {
        IntVector2 g = imageToGrid(p, m_cellSize);
        Cell& c = m_grid[g.x_][g.y_];

        c.x_ = p.x_;
        c.y_ = p.y_;
        c.isValid = true;
    }
    bool isInNeighbourhood(const Vector2& point, const float minDist, const float cellSize)
    {
        IntVector2 g = imageToGrid(point, cellSize);

        // Number of adjucent cells to look for neighbour points
        const int d = 5;

        // Scan the neighbourhood of the point in the grid
        for (int i = g.x_ - d; i < g.x_ + d; i++)
        {
            for (int j = g.y_ - d; j < g.y_ + d; j++)
            {
                // Wrap cells
                int wi = i;
                int wj = j;
                while (wi < 0) wi += m_width;
                while (wj < 0) wj += m_height;
                wi %= m_width;
                wj %= m_height;

                // Test wrapped distances
                Cell p = m_grid[wi][wj];
                const float dist = Min(Min((p - point).Length(),
                                           (p + Vector2(1, 0) - point).Length()),
                                       Min((p + Vector2(0, 1) - point).Length(),
                                           (p + Vector2(1, 1) - point).Length()));

                if (p.isValid && dist < minDist)
                    return true;
            }
        }
        return false;
    }

private:
    int m_width;
    int m_height;
    float m_cellSize;

    Vector<Vector<Cell>> m_grid;
};
Vector2 PoissonRandom::popRandom(PointCloud2DNorm& points)
{
    std::uniform_int_distribution<> dis(0, points.Size() - 1);
    const int idx = dis(impl_->m_generator);
    const Vector2 p = points[idx];
    points.Erase(points.Begin() + idx);
    return p;
}
Vector2 PoissonRandom::generateRandomPointAround(const Vector2& p, float minDist)
{
    // Start with non-uniform distribution
    const float r1 = randomFloat();
    const float r2 = randomFloat();

    // Radius should be between MinDist and 2 * MinDist
    const float radius = minDist * (r1 + 1.0f);

    // Random angle
    const float angle = 2 * 3.141592653589f * r2;

    // The new point is generated around the point (x, y)
    const float x = p.x_ + radius * cos(angle);
    const float y = p.y_ + radius * sin(angle);

    return Vector2(x, y);
}


// Output
PointCloud2DNorm PoissonRandom::generate(const float minDist, const unsigned newPointsCount, const unsigned numPoints)
{
    PointCloud2DNorm samplePoints;
    PointCloud2DNorm processList;

    // Create the grid
    const float cellSize = minDist / sqrt(2.0f);

    const int gridW = static_cast<int>(ceil(1.0f / cellSize));
    const int gridH = static_cast<int>(ceil(1.0f / cellSize));

    Grid grid(gridW, gridH, cellSize);

    Vector2 firstPoint;
    firstPoint.x_ = randomFloat();
    firstPoint.y_ = randomFloat();

    // Update containers
    processList.Push(firstPoint);
    samplePoints.Push(firstPoint);
    grid.insert(firstPoint);

    // Generate new points for each point in the queue
    while (!processList.Empty() && samplePoints.Size() < numPoints)
    {
        const Vector2 point = popRandom(processList);

        for (unsigned i = 0; i < newPointsCount; i++)
        {
            const Vector2 newPoint = generateRandomPointAround(point, minDist);

            // Test
            const bool fits = newPoint.x_ >= 0 && newPoint.y_ >= 0 && newPoint.x_ <= 1 && newPoint.y_ <= 1;

            if (fits && !grid.isInNeighbourhood(newPoint, minDist, cellSize))
            {
                processList.Push(newPoint);
                samplePoints.Push(newPoint);
                grid.insert(newPoint);
                continue;
            }
        }
    }
    return samplePoints;
}

}
