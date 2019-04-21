#include "MBVHTree.h"
#include "MBVHNode.h"

#include <glm/ext.hpp>

#define PRINT_BUILD_TIME 1
#define THREADING 0

#if PRINT_BUILD_TIME
#include <Utils/Timer.h>
#endif

namespace bvh
{
MBVHTree::MBVHTree(StaticBVHTree *orgTree)
{
	this->m_ObjectList = orgTree->m_ObjectList;
	this->m_PrimitiveIndices = orgTree->m_PrimitiveIndices;
	this->m_OriginalTree = orgTree;
	ConstructBVH();
}

void MBVHTree::Traverse(core::Ray &r) const
{
	if (m_CanUseBVH)
	{
		const vec3 invDir = 1.f / r.direction;
		const __m128 dirx4 = _mm_set1_ps(invDir.x);
		const __m128 diry4 = _mm_set1_ps(invDir.y);
		const __m128 dirz4 = _mm_set1_ps(invDir.z);

		const __m128 orgx4 = _mm_set1_ps(r.origin.x);
		const __m128 orgy4 = _mm_set1_ps(r.origin.y);
		const __m128 orgz4 = _mm_set1_ps(r.origin.z);

		m_Tree[0].Intersect(r, dirx4, diry4, dirz4, orgx4, orgy4, orgz4, m_Tree.data(), m_PrimitiveIndices,
							m_ObjectList->GetObjects());
	}
}

unsigned int MBVHTree::TraverseDebug(core::Ray &r) const
{
	if (m_CanUseBVH)
	{
		const vec3 invDir = 1.f / r.direction;
		const __m128 dirx4 = _mm_set1_ps(invDir.x);
		const __m128 diry4 = _mm_set1_ps(invDir.y);
		const __m128 dirz4 = _mm_set1_ps(invDir.z);

		const __m128 orgx4 = _mm_set1_ps(r.origin.x);
		const __m128 orgy4 = _mm_set1_ps(r.origin.y);
		const __m128 orgz4 = _mm_set1_ps(r.origin.z);

		return m_Tree[0].IntersectDebug(r, dirx4, diry4, dirz4, orgx4, orgy4, orgz4, m_Tree.data());
	}

	return 0;
}

void MBVHTree::ConstructBVH()
{
	const uint primCount = m_OriginalTree->m_AABBs.size();
	if (primCount <= 0)
		return;

	m_Tree.clear();
	m_Tree.resize(primCount * 2);
#if PRINT_BUILD_TIME
	utils::Timer t{};
#endif
	m_FinalPtr = 1;
	MBVHNode &mRootNode = m_Tree[0];
	BVHNode &curNode = m_OriginalTree->GetNode(0);
#if THREADING // threading is not actually faster here
	// Only use threading when we have a large number of primitives,
	// otherwise threading is actually slower
	if (m_OriginalTree->m_ThreadPool != nullptr && m_OriginalTree->GetPrimitiveCount() >= 250000)
	{
		m_ThreadLimitReached = false;
		m_BuildingThreads = 0;
		mRootNode.MergeNodesMT(curNode, this->m_OriginalTree->m_BVHPool, this);
	}
	else
	{
		mRootNode.MergeNodes(curNode, this->m_OriginalTree->m_BVHPool, this);
	}
#else
	mRootNode.MergeNodes(curNode, this->m_OriginalTree->m_BVHPool, this);
#endif
	m_Bounds = m_OriginalTree->GetNode(0).bounds;

#if PRINT_BUILD_TIME
	std::cout << "Building MBVH took: " << t.elapsed() << " ms." << std::endl;
#endif
	m_CanUseBVH = true;
}

static const __m128 QuadOne = _mm_set1_ps(1.f);

void MBVHTree::TraceRay(core::Ray &r) const
{
	const vec3 dir_inv = 1.0f / r.direction;
	const vec3 bMin = {m_Bounds.bmin[0], m_Bounds.bmin[1], m_Bounds.bmin[2]};
	const vec3 bMax = {m_Bounds.bmax[0], m_Bounds.bmax[1], m_Bounds.bmax[2]};

	const vec3 t1 = bMin - r.origin * dir_inv;
	const vec3 t2 = bMax - r.origin * dir_inv;

	const vec3 tMax = glm::max(t1, t2);
	const vec3 tMin = glm::min(t1, t2);

	const float tmax = min(tMax.x, min(tMax.y, tMax.z));
	const float tmin = max(tMin.x, max(tMin.y, tMin.z));

	if (tmax >= 0 && tmin < tmax)
	{
#if USE_STACK
		TraverseWithStack(r);
#else
		Traverse(r);
#endif
		if (r.IsValid())
		{
			r.normal = r.obj->GetNormal(r.GetHitpoint());
		}
	}
}

bool MBVHTree::TraceShadowRay(core::Ray &r, float tMax) const
{
	const vec3 dir_inv = 1.0f / r.direction;
	const vec3 bMin = {m_Bounds.bmin[0], m_Bounds.bmin[1], m_Bounds.bmin[2]};
	const vec3 bMax = {m_Bounds.bmax[0], m_Bounds.bmax[1], m_Bounds.bmax[2]};

	const vec3 t1 = bMin - r.origin * dir_inv;
	const vec3 t2 = bMax - r.origin * dir_inv;

	const vec3 tMa = glm::max(t1, t2);
	const vec3 tMi = glm::min(t1, t2);

	const float tmax = min(tMa.x, min(tMa.y, tMa.z));
	const float tmin = max(tMi.x, max(tMi.y, tMi.z));

	if (tmax >= 0 && tmin < tmax)
	{
#if USE_STACK
		TraverseWithStack(r);
#else
		Traverse(r);
#endif
	}

	return r.t < tMax;
}

const std::vector<prims::SceneObject *> &MBVHTree::GetLights() const { return m_ObjectList->GetLights(); }

AABB MBVHTree::GetNodeBounds(uint index)
{
	MBVHNode &curNode = m_Tree[index];
	const float maxx =
		glm::max(curNode.bmaxx[0], glm::max(curNode.bmaxx[1], glm::max(curNode.bmaxx[2], curNode.bmaxx[3])));
	const float maxy =
		glm::max(curNode.bmaxy[0], glm::max(curNode.bmaxy[1], glm::max(curNode.bmaxy[2], curNode.bmaxy[3])));
	const float maxz =
		glm::max(curNode.bmaxz[0], glm::max(curNode.bmaxz[1], glm::max(curNode.bmaxz[2], curNode.bmaxz[3])));

	const float minx =
		glm::min(curNode.bminx[0], glm::min(curNode.bminx[1], glm::min(curNode.bminx[2], curNode.bminx[3])));
	const float miny =
		glm::min(curNode.bminy[0], glm::min(curNode.bminy[1], glm::min(curNode.bminy[2], curNode.bminy[3])));
	const float minz =
		glm::min(curNode.bminz[0], glm::min(curNode.bminz[1], glm::min(curNode.bminz[2], curNode.bminz[3])));

	return AABB(vec3(minx, miny, minz), vec3(maxx, maxy, maxz));
}

uint MBVHTree::GetPrimitiveCount() { return this->m_OriginalTree->GetPrimitiveCount(); }

unsigned int MBVHTree::TraceDebug(core::Ray &r) const
{
	unsigned int depth = 0;

	const __m128 dirInversed = _mm_div_ps(QuadOne, _mm_load_ps(glm::value_ptr(r.direction)));
	const __m128 t1 = _mm_mul_ps(_mm_sub_ps(m_Bounds.bmin4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);
	const __m128 t2 = _mm_mul_ps(_mm_sub_ps(m_Bounds.bmax4, _mm_load_ps(glm::value_ptr(r.origin))), dirInversed);

	union {
		__m128 f4;
		float f[4];
	} qvmax, qvmin;

	qvmax.f4 = _mm_max_ps(t1, t2);
	qvmin.f4 = _mm_min_ps(t1, t2);

	const float tmax = glm::min(qvmax.f[0], glm::min(qvmax.f[1], qvmax.f[2]));
	const float tmin = glm::max(qvmin.f[0], glm::max(qvmin.f[1], qvmin.f[2]));

	if (tmax >= 0.0f && tmin < tmax)
	{
		depth += TraverseDebug(r);
	}

	return depth;
}

void MBVHTree::TraverseWithStack(core::Ray &r) const
{
	MBVHTraversal todo[64];
	MBVHHit mHit;
	int stackptr = 0;

	const vec3 invDir = 1.f / r.direction;
	const __m128 dirx4 = _mm_set1_ps(invDir.x);
	const __m128 diry4 = _mm_set1_ps(invDir.y);
	const __m128 dirz4 = _mm_set1_ps(invDir.z);

	const __m128 orgx4 = _mm_set1_ps(r.origin.x);
	const __m128 orgy4 = _mm_set1_ps(r.origin.y);
	const __m128 orgz4 = _mm_set1_ps(r.origin.z);

	todo[0].leftFirst = 0;
	todo[0].count = -1;
	const std::vector<prims::SceneObject *> &objects = m_ObjectList->GetObjects();

	while (stackptr >= 0)
	{
		const MBVHTraversal &mTodo = todo[stackptr];
		stackptr--;
		if (mTodo.count > -1)
		{ // leaf node
			for (int i = 0; i < mTodo.count; i++)
			{
				const int &primIdx = m_PrimitiveIndices[mTodo.leftFirst + i];
				objects[primIdx]->Intersect(r);
			}
		}
		else
		{
			const MBVHNode &n = this->m_Tree[mTodo.leftFirst];
			mHit = m_Tree[mTodo.leftFirst].Intersect(r, dirx4, diry4, dirz4, orgx4, orgy4, orgz4);
			if (mHit.result > 0)
			{
				for (int i = 3; i >= 0; i--)
				{ // reversed order, we want to check best nodes first
					const unsigned int idx = (mHit.tmini[i] & 0b11);
					if (((mHit.result >> idx) & 0b1) == 1)
					{
						stackptr++;
						todo[stackptr].leftFirst = n.child[idx];
						todo[stackptr].count = n.count[idx];
					}
				}
			}
		}
	}
}
} // namespace bvh