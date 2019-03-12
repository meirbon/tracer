#ifndef BVH_H
#define BVH_H

typedef struct BVHNode {
    float maxX, maxY, maxZ;
    int leftFirst;
    float minX, minY, minZ;
    int count;
} BVHNode;

typedef struct BVHTraversal {
    uint nodeIdx; // Node
    float tNear; // Minimum hit time for this node.
} BVHTraversal;

float2 IntersectBVHNode(global BVHNode* node, global Ray* ray);
void IntersectBVHTree(global BVHNode* nodes, global Triangle* triangles, global uint* primitiveIndices, global Ray* ray);

inline float2 IntersectBVHNodeRay(global BVHNode* node, Ray* ray);
inline void IntersectBVHTreeRay(global BVHNode* nodes, global Triangle* triangles, global uint* primitiveIndices, Ray* ray);

inline float2 IntersectBVHNode(global BVHNode* node, global Ray* ray)
{
    float3 t1 = ((float3)(node->minX, node->minY, node->minZ) - ray->origin) / ray->direction;
    float3 t2 = ((float3)(node->maxX, node->maxY, node->maxZ) - ray->origin) / ray->direction;

    float tmin = fmin(t1.x, t2.x);
    float tmax = fmax(t1.x, t2.x);

    tmin = fmax(tmin, fmin(t1.y, t2.y));
    tmax = fmin(tmax, fmax(t1.y, t2.y));

    tmin = fmax(tmin, fmin(t1.z, t2.z));
    tmax = fmin(tmax, fmax(t1.z, t2.z));

    return (float2)(tmin, tmax);
}

inline float2 IntersectBVHNodeRay(global BVHNode* node, Ray* ray)
{
    float3 t1 = ((float3)(node->minX, node->minY, node->minZ) - ray->origin) / ray->direction;
    float3 t2 = ((float3)(node->maxX, node->maxY, node->maxZ) - ray->origin) / ray->direction;

    float tmin = fmin(t1.x, t2.x);
    float tmax = fmax(t1.x, t2.x);

    tmin = fmax(tmin, fmin(t1.y, t2.y));
    tmax = fmin(tmax, fmax(t1.y, t2.y));

    tmin = fmax(tmin, fmin(t1.z, t2.z));
    tmax = fmin(tmax, fmax(t1.z, t2.z));

    return (float2)(tmin, tmax);
}

inline void IntersectBVHTree(global BVHNode* nodes, global Triangle* triangles, global uint* primitiveIndices, global Ray* ray)
{
    struct BVHTraversal todo[64];
    int stackptr = 0;
    float2 t1NearFar, t2NearFar;

    t1NearFar = IntersectBVHNode(&nodes[0], ray);
    if (t1NearFar.y < 0.f || t1NearFar.x > t1NearFar.y) {
#if DEBUG_BVH
        ray->t = 0.f;
#endif
        return;
    }

#if DEBUG_BVH
    uint steps = 1;
#endif

    // "Push" on the root node to the working setp
    todo[stackptr].nodeIdx = 0;

    while (stackptr >= 0) {
        // Pop off the next node to work on.
        global struct BVHNode* node = &nodes[todo[stackptr].nodeIdx];
        stackptr--;

        if (todo[stackptr].tNear > ray->t) {
            continue;
        }

        if (node->count > -1) { // Leaf node
#if DEBUG_BVH
            steps++;
#endif
            for (int idx = 0; idx < node->count; idx++) {
                const int primIdx = primitiveIndices[node->leftFirst + idx];
                IntersectTriangle(ray, &triangles[primIdx]);
            }
        } else { // Not a leaf
            t1NearFar = IntersectBVHNode(&nodes[node->leftFirst], ray);
            t2NearFar = IntersectBVHNode(&nodes[node->leftFirst + 1], ray);

            int hitLeftNode = (t1NearFar.y >= 0 && t1NearFar.x < t1NearFar.y);
            int hitRightNode = (t2NearFar.y >= 0 && t2NearFar.x < t2NearFar.y);

            if (hitLeftNode == 1 && hitRightNode == 1) {
                if (t1NearFar.x < t2NearFar.x) {
                    todo[++stackptr].nodeIdx = node->leftFirst;
                    todo[stackptr].tNear = t1NearFar.x;

                    todo[++stackptr].nodeIdx = node->leftFirst + 1;
                    todo[stackptr].tNear = t2NearFar.x;
                } else {
                    todo[++stackptr].nodeIdx = node->leftFirst + 1;
                    todo[stackptr].tNear = t2NearFar.x;

                    todo[++stackptr].nodeIdx = node->leftFirst;
                    todo[stackptr].tNear = t1NearFar.x;
                }
            } else if (hitLeftNode == 1) {
                todo[++stackptr].nodeIdx = node->leftFirst;
                todo[stackptr].tNear = t1NearFar.x;
            } else if (hitRightNode == 1) {
                todo[++stackptr].nodeIdx = node->leftFirst + 1;
                todo[stackptr].tNear = t2NearFar.x;
            }
        }
    }
#if DEBUG_BVH
    ray->t = (float)steps;
#endif
}

inline void IntersectBVHTreeRay(global BVHNode* nodes, global Triangle* triangles, global uint* primitiveIndices, Ray* ray)
{
    struct BVHTraversal todo[64];
    int stackptr = 0;
    float2 t1NearFar, t2NearFar;

    t1NearFar = IntersectBVHNodeRay(&nodes[0], ray);
    if (t1NearFar.y < 0.f || t1NearFar.x > t1NearFar.y) {
#if DEBUG_BVH
        ray->t = 0.f;
#endif
        return;
    }

#if DEBUG_BVH
    uint steps = 1;
#endif

    // "Push" on the root node to the working setp
    todo[stackptr].nodeIdx = 0;

    while (stackptr >= 0) {
        // Pop off the next node to work on.
        global struct BVHNode* node = &nodes[todo[stackptr].nodeIdx];
        stackptr--;

        if (todo[stackptr].tNear > ray->t) {
            continue;
        }

        if (node->count > -1) { // Leaf node
#if DEBUG_BVH
            steps++;
#endif
            for (int idx = 0; idx < node->count; idx++) {
                const int primIdx = primitiveIndices[node->leftFirst + idx];
                IntersectTriangleRay(ray, &triangles[primIdx]);
            }
        } else { // Not a leaf
            t1NearFar = IntersectBVHNodeRay(&nodes[node->leftFirst], ray);
            t2NearFar = IntersectBVHNodeRay(&nodes[node->leftFirst + 1], ray);

            int hitLeftNode = (t1NearFar.y >= 0 && t1NearFar.x < t1NearFar.y);
            int hitRightNode = (t2NearFar.y >= 0 && t2NearFar.x < t2NearFar.y);

            if (hitLeftNode == 1 && hitRightNode == 1) {
                if (t1NearFar.x < t2NearFar.x) {
                    todo[++stackptr].nodeIdx = node->leftFirst;
                    todo[stackptr].tNear = t1NearFar.x;

                    todo[++stackptr].nodeIdx = node->leftFirst + 1;
                    todo[stackptr].tNear = t2NearFar.x;
                } else {
                    todo[++stackptr].nodeIdx = node->leftFirst + 1;
                    todo[stackptr].tNear = t2NearFar.x;

                    todo[++stackptr].nodeIdx = node->leftFirst;
                    todo[stackptr].tNear = t1NearFar.x;
                }
            } else if (hitLeftNode == 1) {
                todo[++stackptr].nodeIdx = node->leftFirst;
                todo[stackptr].tNear = t1NearFar.x;
            } else if (hitRightNode == 1) {
                todo[++stackptr].nodeIdx = node->leftFirst + 1;
                todo[stackptr].tNear = t2NearFar.x;
            }
        }
    }
#if DEBUG_BVH
    ray->t = (float)steps;
#endif
}

#endif