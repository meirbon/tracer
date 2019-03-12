#pragma once

#include <Utils/ctpl.h>
#include <atomic>
#include <immintrin.h>
#include <thread>
#include <vector>

#include "BVH/AABB.h"
#include "GameObjectNode.h"

class SceneObject;
struct Ray;

namespace bvh
{
class StaticBVHTree;
class TopLevelBVH;

enum BVHType
{
    CENTRAL_SPLIT = 0,
    SAH = 1,
    SAH_BINNING = 2
};

struct BVHNode
{
  public:
    AABB bounds;

    BVHNode();

    BVHNode(int leftFirst, int count, AABB bounds);

    ~BVHNode() = default;

    inline bool IsLeaf() const noexcept { return bounds.count > -1; }

    bool Intersect(const Ray &r) const;

    bool IntersectSIMD(const Ray &r) const;

    bool Intersect(const Ray &r, float &tNear, float &tFar) const;

    bool IntersectSIMD(const Ray &r, float &tNear, float &tFar) const;

    inline void SetCount(int value) noexcept { bounds.count = value; }

    inline void SetLeftFirst(unsigned int value) noexcept
    {
        bounds.leftFirst = value;
    }

    inline int GetCount() const noexcept { return bounds.count; }

    inline int GetLeftFirst() const noexcept { return bounds.leftFirst; }

    unsigned int TraverseDebug(Ray &r, const bvh::StaticBVHTree *bvhTree) const;

    unsigned int TraverseDebug(Ray &r,
                               const std::vector<BVHNode> &bvhTree) const;

    unsigned int
    TraverseDebug(Ray &r, const std::vector<bvh::GameObjectNode> &objectList,
                  const std::vector<bvh::BVHNode> &bvhTree,
                  const std::vector<unsigned int> &primIndices) const;

    void Traverse(Ray &r, const std::vector<SceneObject *> &objectList,
                  const bvh::StaticBVHTree *bvhTree) const;

    void Traverse(Ray &r, const std::vector<SceneObject *> &objectList,
                  const std::vector<BVHNode> &bvhTree,
                  const std::vector<unsigned int> &primIndices) const;

    int Traverse(Ray &r, const std::vector<bvh::GameObjectNode> &objectList,
                 const std::vector<bvh::BVHNode> &bvhTree,
                 const std::vector<unsigned int> &primIndices) const;

    int TraverseShadow(Ray &r,
                       const std::vector<bvh::GameObjectNode> &objectList,
                       const std::vector<bvh::BVHNode> &bvhTree,
                       const std::vector<unsigned int> &primIndices) const;

    void Subdivide(const std::vector<AABB> &aabbs, bvh::StaticBVHTree *bvhTree,
                   unsigned int depth);

    void SubdivideMT(const std::vector<AABB> &aabbs,
                     bvh::StaticBVHTree *bvhTree, unsigned int depth);

    bool Partition(const std::vector<AABB> &aabbs, bvh::StaticBVHTree *bvhTree,
                   int &left, int &right);

    void Subdivide(const std::vector<AABB> &aabbs,
                   std::vector<BVHNode> &bvhTree,
                   std::vector<unsigned int> &primIndices, unsigned int depth);

    void SubdivideMT(const std::vector<AABB> *aabbs,
                     std::vector<BVHNode> *bvhTree,
                     std::vector<unsigned int> *primIndices,
                     ctpl::ThreadPool *tPool, std::mutex *threadMutex,
                     std::mutex *partitionMutex, unsigned int *threadCount,
                     unsigned int depth);

    bool Partition(const std::vector<AABB> *aabbs,
                   std::vector<BVHNode> *bvhTree,
                   std::vector<unsigned int> *primIndices,
                   std::mutex *partitionMutex, int &left, int &right);

    bool Partition(const std::vector<AABB> *aabbs,
                   std::vector<BVHNode> *bvhTree,
                   std::vector<unsigned int> *primIndices, int &left,
                   int &right);

    void CalculateBounds(const std::vector<AABB> &aabbs,
                         const std::vector<unsigned int> &primitiveIndices);

    void CalculateBounds(int leftChildIndex, const std::vector<BVHNode> &pool);

    void CalculateBounds(const std::vector<GameObjectNode> &objectList,
                         const std::vector<unsigned int> &primIndices);
};
} // namespace bvh