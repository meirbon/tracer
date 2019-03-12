typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

// do not change order of these
// clang-format off
#include "../src/Shared.h"
#include "randomnumbers.cl"
#include "material.cl"
#include "ray.cl"
#include "camera.cl"
#include "triangle.cl"
#include "sphere.cl"
#include "bvh.cl"
#include "mbvh.cl"
#include "microfacet.cl"
// clang-format on

void TraceBVH(global Ray* ray, global Triangle* triangles, global BVHNode* nodes, global uint* primitiveIndices);

void TraceBVHRay(Ray* ray, global Triangle* triangles, global BVHNode* nodes, global uint* primitiveIndices);

void Trace(global Ray* ray, global Triangle* triangles, global BVHNode* node, global MBVHNode* mNodes, global uint* primitiveIndices);

void TraceRay(Ray* ray, global Triangle* triangles, global BVHNode* node, global MBVHNode* mNodes, global uint* primitiveIndices);

#include "path-tracer.cl"

inline void TraceBVH(global Ray* ray, global Triangle* triangles, global BVHNode* nodes, global uint* primitiveIndices)
{
    IntersectBVHTree(nodes, triangles, primitiveIndices, ray);
}

inline void TraceBVHRay(Ray* ray, global Triangle* triangles, global BVHNode* nodes, global uint* primitiveIndices)
{
    IntersectBVHTreeRay(nodes, triangles, primitiveIndices, ray);
}

inline void Trace(global Ray* ray, global Triangle* triangles, global BVHNode* node, global MBVHNode* mNodes, global uint* primitiveIndices)
{
#if MBVH
    IntersectMBVHTree(mNodes, triangles, primitiveIndices, ray);
#else
    TraceBVH(ray, triangles, node, primitiveIndices);
#endif
}

inline void TraceRay(Ray* ray, global Triangle* triangles, global BVHNode* node, global MBVHNode* mNodes, global uint* primitiveIndices)
{
#if MBVH
    IntersectMBVHTreeRay(mNodes, triangles, primitiveIndices, ray);
#else
    TraceBVHRay(ray, triangles, node, primitiveIndices);
#endif
}