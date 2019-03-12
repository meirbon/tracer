#ifndef TRIANGLE_H
#define TRIANGLE_H

typedef struct Triangle {
    float3 p0; // 16
    float3 p1; // 32
    float3 p2; // 48
    float3 n0; // 64
    float3 n1; // 80
    float3 n2; // 96
    float3 normal; // 112
    uint mat_idx; // 116
    uint hit_idx; // 120
    float m_Area; // 124
    float dummy; // 128
    float t0x, t0y; // 136
    float t1x, t1y; // 144
    float t2x, t2y; // 152
    float d156, d160; // 160
} Triangle;

float3 GetBaryCentricCoordinatesTriangle(float3 hitPoint, global Triangle* triangle);
float3 GetBaryCentricCoordinatesTriangleLocal(float3 hitPoint, Triangle triangle);

float3 GetTriangleNormal(float3 hitPoint, global Triangle* triangle);
float3 GetTriangleNormalLocal(float3 hitPoint, Triangle triangle);

float3 RandomPointOnTriangle(global Triangle* triangle, uint* seed);
float3 RandomPointOnTriangleLocal(Triangle triangle, uint* seed);

void IntersectTriangle(global Ray* ray, global Triangle* triangle);
void IntersectTriangleRay(Ray* ray, global Triangle* triangle);

inline float3 GetBaryCentricCoordinatesTriangle(float3 hitPoint, global Triangle* triangle)
{
    float3 n = (triangle->n0 + triangle->n1 + triangle->n2) / 3.0f;
    const float areaABC = dot(n, cross(triangle->p1 - triangle->p0, triangle->p2 - triangle->p0));
    const float areaPBC = dot(n, cross(triangle->p1 - hitPoint, triangle->p2 - hitPoint));
    const float areaPCA = dot(n, cross(triangle->p2 - hitPoint, triangle->p0 - hitPoint));

    const float alpha = areaPBC / areaABC;
    const float beta = areaPCA / areaABC;
    const float gamma = 1.f - alpha - beta;

    return (float3)(alpha, beta, gamma);
}

inline float3 GetBaryCentricCoordinatesTriangleLocal(float3 hitPoint, Triangle triangle)
{
    float3 n = (triangle.n0 + triangle.n1 + triangle.n2) / 3.0f;
    const float areaABC = dot(n, cross(triangle.p1 - triangle.p0, triangle.p2 - triangle.p0));
    const float areaPBC = dot(n, cross(triangle.p1 - hitPoint, triangle.p2 - hitPoint));
    const float areaPCA = dot(n, cross(triangle.p2 - hitPoint, triangle.p0 - hitPoint));

    const float alpha = areaPBC / areaABC;
    const float beta = areaPCA / areaABC;
    const float gamma = 1.f - alpha - beta;

    return (float3)(alpha, beta, gamma);
}

inline float3 GetTriangleNormal(float3 hitPoint, global Triangle* triangle)
{
    const float3 bary = GetBaryCentricCoordinatesTriangle(hitPoint, triangle);
    return normalize(bary.x * triangle->n0 + bary.y * triangle->n1 + bary.z * triangle->n2);
}

inline float3 GetTriangleNormalLocal(float3 hitPoint, Triangle triangle)
{
    const float3 bary = GetBaryCentricCoordinatesTriangleLocal(hitPoint, triangle);
    return normalize(bary.x * triangle.n0 + bary.y * triangle.n1 + bary.z * triangle.n2);
}

inline float3 RandomPointOnTriangle(global Triangle* triangle, uint* seed)
{
    float r1, r2;
    r1 = RandomFloat(seed);
    do {
        r2 = RandomFloat(seed);
    } while (r1 == r2);

    if (r1 + r2 >= 1.f) {
        r1 = 1.f - r1;
        r2 = 1.f - r2;
    }

    return triangle->p0 + r1 * (triangle->p1 - triangle->p0) + r2 * (triangle->p2 - triangle->p0);
}

inline float3 RandomPointOnTriangleLocal(Triangle triangle, uint* seed)
{
    float r1, r2;
    r1 = RandomFloat(seed);
    do {
        r2 = RandomFloat(seed);
    } while (r1 == r2);

    if (r1 + r2 >= 1.f) {
        r1 = 1.f - r1;
        r2 = 1.f - r2;
    }

    return triangle.p0 + r1 * (triangle.p1 - triangle.p0) + r2 * (triangle.p2 - triangle.p0);
}

void IntersectTriangle(global Ray* ray, global Triangle* triangle)
{
    const float3 edge1 = triangle->p1 - triangle->p0;
    const float3 edge2 = triangle->p2 - triangle->p0;

    const float3 h = cross(ray->direction, edge2);

    const float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return; // This ray is parallel to this triangle.

    const float f = 1.f / a;
    const float3 s = ray->origin - triangle->p0;
    const float u = f * dot(s, h);
    if (u < 0.f || u > 1.f)
        return;

    const float3 q = cross(s, edge1);
    const float v = f * dot(ray->direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return;

    // At this stage we can compute t to find out where the intersection point is on the line.
    const float t = f * dot(edge2, q);

    if (t > EPSILON && t < ray->t) // ray intersection
    {
        // ray intersection
        ray->t = t;
        ray->hit_idx = triangle->hit_idx;
    }
}

void IntersectTriangleRay(Ray* ray, global Triangle* triangle)
{
    const float3 edge1 = triangle->p1 - triangle->p0;
    const float3 edge2 = triangle->p2 - triangle->p0;

    const float3 h = cross(ray->direction, edge2);

    const float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return; // This ray is parallel to this triangle.

    const float f = 1.f / a;
    const float3 s = ray->origin - triangle->p0;
    const float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return;

    const float3 q = cross(s, edge1);
    const float v = f * dot(ray->direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return;

    // At this stage we can compute t to find out where the intersection point is on the line.
    const float t = f * dot(edge2, q);

    if (t > EPSILON && t < ray->t) // ray intersection
    {
        // ray intersection
        ray->t = t;
        ray->hit_idx = triangle->hit_idx;
    }
}

#endif