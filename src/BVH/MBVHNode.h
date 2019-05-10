#pragma once

#include <glm/glm.hpp>
#include <immintrin.h>

#include "BVH/BVHNode.h"
#include "MBVHTree.h"
#include "Shared.h"
#include "StaticBVHTree.h"

struct MBVHHit
{
	union {
		vec4 tmin4;
		float tmin[4];
		int tmini[4];
	};
	bvec4 result;
};

namespace bvh
{
class MBVHTree;

class MBVHNode
{
  public:
	MBVHNode() = default;

	~MBVHNode() = default;

	union {
		__m128 bminx4;
		float bminx[4]{};
		glm::vec4 minx;
	};
	union {
		__m128 bmaxx4;
		float bmaxx[4]{};
		glm::vec4 maxx;
	};

	union {
		__m128 bminy4;
		float bminy[4]{};
		glm::vec4 miny;
	};
	union {
		__m128 bmaxy4;
		float bmaxy[4]{};
		glm::vec4 maxy;
	};

	union {
		__m128 bminz4;
		float bminz[4]{};
		glm::vec4 minz;
	};
	union {
		__m128 bmaxz4;
		float bmaxz[4]{};
		glm::vec4 maxz;
	};

	int child[4];
	int count[4];

	void SetBounds(unsigned int nodeIdx, const glm::vec3 &min, const glm::vec3 &max);

	void SetBounds(unsigned int nodeIdx, const bvh::AABB &bounds);

	unsigned int IntersectDebug(core::Ray &r, const glm::vec3 &dirInverse, const bvh::MBVHNode *pool) const;

	void Intersect(core::Ray &r, const glm::vec3 &dirInverse, const MBVHNode *pool,
				   const std::vector<unsigned int> &primitiveIndices,
				   const std::vector<prims::SceneObject *> &objectList) const;

	MBVHHit Intersect(core::Ray &r, const glm::vec3 &dirInverse) const;

	void MergeNodes(const bvh::BVHNode &node, const bvh::BVHNode *bvhPool, bvh::MBVHTree *bvhTree);

	void MergeNodesMT(const bvh::BVHNode &node, const bvh::BVHNode *bvhPool, bvh::MBVHTree *bvhTree,
					  bool thread = true);

	void MergeNodes(const bvh::BVHNode &node, const std::vector<bvh::BVHNode> &bvhPool, bvh::MBVHTree *bvhTree);

	void MergeNodesMT(const bvh::BVHNode &node, const std::vector<bvh::BVHNode> &bvhPool, bvh::MBVHTree *bvhTree,
					  bool thread = true);

	void GetBVHNodeInfo(const bvh::BVHNode &node, const bvh::BVHNode *pool, int &numChildren);

	void GetBVHNodeInfo(const bvh::BVHNode &node, const std::vector<bvh::BVHNode> &bvhPool, int &numChildren);

	void SortResults(const float *tmin, int &a, int &b, int &c, int &d) const;
};
} // namespace bvh