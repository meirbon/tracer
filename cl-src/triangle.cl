#ifndef TRIANGLE_CL
#define TRIANGLE_CL

bool t_intersect(float3 org, float3 dir, float *rayt, float3 p0, float3 p1, float3 p2, float epsilon);
float3 t_getBaryCoords(float3 p, float3 normal, float3 p0, float3 p1, float3 p2);
float3 t_getRandomPointOnSurface(float3 p0, float3 p1, float3 p2, float r1, float r2);
float3 t_getNormal(float3 bary, float3 n0, float3 n1, float3 n2);
float2 t_getTexCoords(float3 bary, float2 t0, float2 t1, float2 t2);
float t_getArea(float3 p0, float3 p1, float3 p2);

#define EPSILON_TRIANGLE 0.00001f

inline bool t_intersect(float3 org, float3 dir, float *rayt, float3 p0, float3 p1, float3 p2, float epsilon)
{
    const float3 edge1 = p1 - p0;
    const float3 edge2 = p2 - p0;

    const float3 h = cross(dir, edge2);

    const float a = dot(edge1, h);
    if (a > -epsilon && a < epsilon)
        return false;

    const float f = 1.f / a;
    const float3 s = org - p0;
    const float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    const float3 q = cross(s, edge1);
    const float v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    const float t = f * dot(edge2, q);

    if (t > epsilon && *rayt > t) // ray intersection
    {
        *rayt = t;
        return true;
    }

    return false;
}

inline float3 t_getBaryCoords(float3 p, float3 normal, float3 p0, float3 p1, float3 p2)
{
    const float areaABC = dot(normal, cross(p1 - p0, p2 - p0));
    const float areaPBC = dot(normal, cross(p1 - p, p2 - p));
    const float areaPCA = dot(normal, cross(p2 - p, p0 - p));

    const float alpha = areaPBC / areaABC;
    const float beta = areaPCA / areaABC;
    const float gamma = 1.0f - alpha - beta;

    return (float3)(alpha, beta, gamma);
}

inline float3 t_getRandomPointOnSurface(float3 p0, float3 p1, float3 p2, float r1, float r2)
{
    if (r1 + r2 > 1.0f)
    {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }

    return p0 + r1 * (p1 - p0) + r2 * (p2 - p0);
}

inline float3 t_getNormal(float3 bary, float3 n0, float3 n1, float3 n2)
{
    return bary.x * n0 + bary.y * n1 + bary.z * n2;
}

inline float2 t_getTexCoords(float3 bary, float2 t0, float2 t1, float2 t2)
{
    return bary.x * t0 + bary.y * t1 + bary.z * t2;
}

inline float t_getArea(float3 p0, float3 p1, float3 p2)
{
    const float a = fast_length(p0 - p1);
    const float b = fast_length(p1 - p2);
    const float c = fast_length(p2 - p0);
    const float s = (a + b + c) / 2.f;
    return sqrt(s * (s - a) * (s - b) * (s - c));
}

#endif