#pragma once

#include <glm/glm.hpp>
#include <immintrin.h>

#include "BVH/BVHNode.h"
#include "MBVHTree.h"
#include "Shared.h"
#include "StaticBVHTree.h"

namespace bvh
{
class MBVHTree;

struct MBVHHit
{
    union {
        __m128 tmin4;
        float tmin[4];
        int tmini[4];
    };
    int result;
};

class MBVHNode
{
  public:
    MBVHNode() = default;

    ~MBVHNode() = default;

    union {
        __m128 bminx4;
        float bminx[4]{};
    };
    union {
        __m128 bmaxx4;
        float bmaxx[4]{};
    };

    union {
        __m128 bminy4;
        float bminy[4]{};
    };
    union {
        __m128 bmaxy4;
        float bmaxy[4]{};
    };

    union {
        __m128 bminz4;
        float bminz[4]{};
    };
    union {
        __m128 bmaxz4;
        float bmaxz[4]{};
    };

    int child[4];
    int count[4];

    void SetBounds(unsigned int nodeIdx, const glm::vec3 &min,
                   const glm::vec3 &max);

    void SetBounds(unsigned int nodeIdx, const bvh::AABB &bounds);

    unsigned int IntersectDebug(Ray &r, const __m128 &dirX, const __m128 &dirY,
                                const __m128 &dirZ, const __m128 &orgX,
                                const __m128 &orgY, const __m128 &orgZ,
                                const bvh::MBVHNode *pool) const;

    void Intersect(Ray &r, const __m128 &dirX, const __m128 &dirY,
                   const __m128 &dirZ, const __m128 &orgX, const __m128 &orgY,
                   const __m128 &orgZ, const bvh::MBVHNode *pool,
                   const std::vector<unsigned int> &primitiveIndices,
                   const std::vector<SceneObject *> &objectList) const;

    MBVHHit Intersect(Ray &r, const __m128 &dirX, const __m128 &dirY,
                      const __m128 &dirZ, const __m128 &orgX,
                      const __m128 &orgY, const __m128 &orgZ) const;

    void MergeNodes(const bvh::BVHNode &node, const bvh::BVHNode *bvhPool,
                    bvh::MBVHTree *bvhTree);

    void MergeNodesMT(const bvh::BVHNode &node, const bvh::BVHNode *bvhPool,
                      bvh::MBVHTree *bvhTree, bool thread = true);

    void MergeNodes(const bvh::BVHNode &node,
                    const std::vector<bvh::BVHNode> &bvhPool,
                    bvh::MBVHTree *bvhTree);

    void MergeNodesMT(const bvh::BVHNode &node,
                      const std::vector<bvh::BVHNode> &bvhPool,
                      bvh::MBVHTree *bvhTree, bool thread = true);

    void GetBVHNodeInfo(const bvh::BVHNode &node, const bvh::BVHNode *pool,
                        int &numChildren);

    void GetBVHNodeInfo(const bvh::BVHNode &node,
                        const std::vector<bvh::BVHNode> &bvhPool,
                        int &numChildren);

    void SortResults(const float *tmin, int &a, int &b, int &c, int &d) const;
};
} // namespace bvh