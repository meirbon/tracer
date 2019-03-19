#include "BVH/MBVHNode.h"
#include "BVH/MBVHTree.h"
#include "Utils/Messages.h"

namespace bvh
{
void MBVHNode::SetBounds(unsigned int nodeIdx, const vec3 &min, const vec3 &max)
{
	this->bminx[nodeIdx] = min.x;
	this->bminy[nodeIdx] = min.y;
	this->bminz[nodeIdx] = min.z;

	this->bmaxx[nodeIdx] = max.x;
	this->bmaxy[nodeIdx] = max.y;
	this->bmaxz[nodeIdx] = max.z;
}

void MBVHNode::SetBounds(unsigned int nodeIdx, const AABB &bounds)
{
	this->bminx[nodeIdx] = bounds.bmin[0];
	this->bminy[nodeIdx] = bounds.bmin[1];
	this->bminz[nodeIdx] = bounds.bmin[2];

	this->bmaxx[nodeIdx] = bounds.bmax[0];
	this->bmaxy[nodeIdx] = bounds.bmax[1];
	this->bmaxz[nodeIdx] = bounds.bmax[2];
}

unsigned int MBVHNode::IntersectDebug(core::Ray &r, const __m128 &dirx4, const __m128 &diry4, const __m128 &dirz4,
									  const __m128 &orgx4, const __m128 &orgy4, const __m128 &orgz4,
									  const MBVHNode *pool) const
{
	unsigned int depth = 1;
	union {
		__m128 tmin4;
		float tmin[4];
		uint tmini[4];
	};

	union {
		__m128 tmax4;
		float tmax[4];
	};

	const __m128 tx1 = _mm_mul_ps(_mm_sub_ps(bminx4, orgx4), dirx4);
	const __m128 tx2 = _mm_mul_ps(_mm_sub_ps(bmaxx4, orgx4), dirx4);

	tmax4 = _mm_max_ps(tx1, tx2);
	tmin4 = _mm_min_ps(tx1, tx2);

	const __m128 ty1 = _mm_mul_ps(_mm_sub_ps(bminy4, orgy4), diry4);
	const __m128 ty2 = _mm_mul_ps(_mm_sub_ps(bmaxy4, orgy4), diry4);

	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(ty1, ty2));
	tmin4 = _mm_max_ps(tmin4, _mm_min_ps(ty1, ty2));

	const __m128 tz1 = _mm_mul_ps(_mm_sub_ps(bminz4, orgz4), dirz4);
	const __m128 tz2 = _mm_mul_ps(_mm_sub_ps(bmaxz4, orgz4), dirz4);

	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(tz1, tz2));
	tmin4 = _mm_max_ps(tmin4, _mm_min_ps(tz1, tz2));

	const int result =
		_mm_movemask_ps(_mm_and_ps(_mm_cmpge_ps(tmax4, _mm_setzero_ps()),
								   _mm_and_ps(_mm_cmplt_ps(tmin4, tmax4), _mm_cmplt_ps(tmin4, _mm_set1_ps(r.t)))));

	if (result > 0)
	{
		// sort based on tmin
		tmini[0] &= 0xFFFFFFFC;
		tmini[1] &= 0xFFFFFFFC;
		tmini[2] &= 0xFFFFFFFC;
		tmini[3] &= 0xFFFFFFFC;

		tmini[0] = (tmini[0] | 0b00);
		tmini[1] = (tmini[1] | 0b01);
		tmini[2] = (tmini[2] | 0b10);
		tmini[3] = (tmini[3] | 0b11);

		if (tmin[0] > tmin[1])
		{
			std::swap(tmin[0], tmin[1]);
		}
		if (tmin[2] > tmin[3])
		{
			std::swap(tmin[2], tmin[3]);
		}
		if (tmin[0] > tmin[2])
		{
			std::swap(tmin[0], tmin[2]);
		}
		if (tmin[1] > tmin[3])
		{
			std::swap(tmin[1], tmin[3]);
		}
		if (tmin[2] > tmin[3])
		{
			std::swap(tmin[2], tmin[3]);
		}

		for (unsigned int t : tmini)
		{
			const uint idx = (t & 0b11);
			if (!((result >> idx) & 0x1))
			{ // we should not check this node
				continue;
			}

			if (this->count[idx] > -1)
			{ // leaf node
				depth += 1;
			}
			else
			{
				depth += pool[this->child[idx]].IntersectDebug(r, dirx4, diry4, dirz4, orgx4, orgy4, orgz4, pool);
			}
		}
	}

	return depth;
}

static const __m128 zerops = _mm_set1_ps(EPSILON);

void MBVHNode::Intersect(core::Ray &r, const __m128 &dirx4, const __m128 &diry4, const __m128 &dirz4,
						 const __m128 &orgx4, const __m128 &orgy4, const __m128 &orgz4, const MBVHNode *pool,
						 const std::vector<unsigned int> &primitiveIndices,
						 const std::vector<prims::SceneObject *> &objectList) const
{
	union {
		__m128 tmin4;
		float tmin[4];
		uint tmini[4];
	};

	union {
		__m128 tmax4;
		float tmax[4];
	};

	const __m128 tx1 = _mm_mul_ps(_mm_sub_ps(bminx4, orgx4), dirx4);
	const __m128 tx2 = _mm_mul_ps(_mm_sub_ps(bmaxx4, orgx4), dirx4);

	tmax4 = _mm_max_ps(tx1, tx2);
	tmin4 = _mm_min_ps(tx1, tx2);

	const __m128 ty1 = _mm_mul_ps(_mm_sub_ps(bminy4, orgy4), diry4);
	const __m128 ty2 = _mm_mul_ps(_mm_sub_ps(bmaxy4, orgy4), diry4);

	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(ty1, ty2));
	tmin4 = _mm_max_ps(tmin4, _mm_min_ps(ty1, ty2));

	const __m128 tz1 = _mm_mul_ps(_mm_sub_ps(bminz4, orgz4), dirz4);
	const __m128 tz2 = _mm_mul_ps(_mm_sub_ps(bmaxz4, orgz4), dirz4);

	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(tz1, tz2));
	tmin4 = _mm_max_ps(tmin4, _mm_min_ps(tz1, tz2));

	const int result = _mm_movemask_ps(_mm_and_ps(
		_mm_cmpge_ps(tmax4, zerops), _mm_and_ps(_mm_cmplt_ps(tmin4, tmax4), _mm_cmplt_ps(tmin4, _mm_set1_ps(r.t)))));

	if (result > 0)
	{
		// sort based on tmin
		tmini[0] &= 0xFFFFFFFC;
		tmini[1] &= 0xFFFFFFFC;
		tmini[2] &= 0xFFFFFFFC;
		tmini[3] &= 0xFFFFFFFC;

		tmini[0] = (tmini[0] | 0b00);
		tmini[1] = (tmini[1] | 0b01);
		tmini[2] = (tmini[2] | 0b10);
		tmini[3] = (tmini[3] | 0b11);

		if (tmin[0] > tmin[1])
		{
			std::swap(tmin[0], tmin[1]);
		}
		if (tmin[2] > tmin[3])
		{
			std::swap(tmin[2], tmin[3]);
		}
		if (tmin[0] > tmin[2])
		{
			std::swap(tmin[0], tmin[2]);
		}
		if (tmin[1] > tmin[3])
		{
			std::swap(tmin[1], tmin[3]);
		}
		if (tmin[2] > tmin[3])
		{
			std::swap(tmin[2], tmin[3]);
		}

		for (unsigned int t : tmini)
		{
			const uint idx = (t & 0b11);
			if (!((result >> idx) & 0x1))
			{ // we should not check this node
				continue;
			}
			if (this->count[idx] > -1)
			{ // leaf node
				for (int i = 0; i < this->count[idx]; i++)
				{
					const uint &primIdx = primitiveIndices[i + this->child[idx]];
					objectList[primIdx]->Intersect(r);
				}
			}
			else
			{
				pool[this->child[idx]].Intersect(r, dirx4, diry4, dirz4, orgx4, orgy4, orgz4, pool, primitiveIndices,
												 objectList);
			}
		}
	}
}

void MBVHNode::MergeNodes(const BVHNode &node, const BVHNode *bvhPool, MBVHTree *bvhTree)
{
	int numChildren;
	GetBVHNodeInfo(node, bvhPool, numChildren);

	for (int idx = 0; idx < numChildren; idx++)
	{
		if (this->count[idx] == -1)
		{ // not a leaf
			const BVHNode &curNode = bvhPool[this->child[idx]];
			if (curNode.IsLeaf())
			{
				this->count[idx] = curNode.GetCount();
				this->child[idx] = curNode.GetLeftFirst();
				this->SetBounds(idx, curNode.bounds);
			}
			else
			{
				const uint newIdx = bvhTree->m_FinalPtr++;
				MBVHNode &newNode = bvhTree->m_Tree[newIdx];
				this->child[idx] = newIdx; // replace BVHNode idx with MBVHNode idx
				this->count[idx] = -1;
				this->SetBounds(idx, curNode.bounds);
				newNode.MergeNodes(curNode, bvhPool, bvhTree);
			}
		}
	}

	// invalidate any remaining children
	for (int idx = numChildren; idx < 4; idx++)
	{
		this->SetBounds(idx, vec3(1e34f), vec3(-1e34f));
		this->count[idx] = 0;
	}
}

void MBVHNode::MergeNodesMT(const BVHNode &node, const BVHNode *bvhPool, MBVHTree *bvhTree, bool thread)
{
	int numChildren;
	GetBVHNodeInfo(node, bvhPool, numChildren);

	int threadCount = 0;
	std::vector<std::future<void>> threads{};

	// invalidate any remaining children
	for (int idx = numChildren; idx < 4; idx++)
	{
		this->SetBounds(idx, vec3(1e34f), vec3(-1e34f));
		this->count[idx] = 0;
	}

	for (int idx = 0; idx < numChildren; idx++)
	{
		if (this->count[idx] == -1)
		{ // not a leaf
			const BVHNode *curNode = &bvhPool[this->child[idx]];

			if (curNode->IsLeaf())
			{
				this->count[idx] = curNode->GetCount();
				this->child[idx] = curNode->GetLeftFirst();
				this->SetBounds(idx, curNode->bounds);
				continue;
			}

			bvhTree->m_PoolPtrMutex.lock();
			const auto newIdx = bvhTree->m_FinalPtr++;
			bvhTree->m_PoolPtrMutex.unlock();

			MBVHNode *newNode = &bvhTree->m_Tree[newIdx];
			this->child[idx] = newIdx; // replace BVHNode idx with MBVHNode idx
			this->count[idx] = -1;
			this->SetBounds(idx, curNode->bounds);

			if (bvhTree->m_ThreadLimitReached || !thread)
			{
				newNode->MergeNodesMT(*curNode, bvhPool, bvhTree, !thread);
			}
			else
			{
				bvhTree->m_ThreadMutex.lock();
				bvhTree->m_BuildingThreads++;
				if (bvhTree->m_BuildingThreads > ctpl::nr_of_cores)
					bvhTree->m_ThreadLimitReached = true;
				bvhTree->m_ThreadMutex.unlock();

				threadCount++;
				threads.push_back(bvhTree->m_OriginalTree->m_ThreadPool->push(
					[newNode, curNode, bvhPool, bvhTree](int) { newNode->MergeNodesMT(*curNode, bvhPool, bvhTree); }));
			}
		}
	}

	for (int i = 0; i < threadCount; i++)
	{
		threads[i].get();
	}
}

void MBVHNode::MergeNodes(const bvh::BVHNode &node, const std::vector<bvh::BVHNode> &bvhPool, bvh::MBVHTree *bvhTree)
{
	int numChildren;
	GetBVHNodeInfo(node, bvhPool, numChildren);

	for (int idx = 0; idx < numChildren; idx++)
	{
		if (this->count[idx] == -1)
		{ // not a leaf
			const BVHNode &curNode = bvhPool[this->child[idx]];
			if (curNode.IsLeaf())
			{
				this->count[idx] = curNode.GetCount();
				this->child[idx] = curNode.GetLeftFirst();
				this->SetBounds(idx, curNode.bounds);
			}
			else
			{
				const uint newIdx = bvhTree->m_FinalPtr++;
				MBVHNode &newNode = bvhTree->m_Tree[newIdx];
				this->child[idx] = newIdx; // replace BVHNode idx with MBVHNode idx
				this->count[idx] = -1;
				this->SetBounds(idx, curNode.bounds);
				newNode.MergeNodes(curNode, bvhPool, bvhTree);
			}
		}
	}

	// invalidate any remaining children
	for (int idx = numChildren; idx < 4; idx++)
	{
		this->SetBounds(idx, vec3(1e34f), vec3(-1e34f));
		this->count[idx] = 0;
	}
}

void MBVHNode::MergeNodesMT(const bvh::BVHNode &node, const std::vector<bvh::BVHNode> &bvhPool, MBVHTree *bvhTree,
							bool thread)
{
	int numChildren;
	GetBVHNodeInfo(node, bvhPool, numChildren);

	int threadCount = 0;
	std::vector<std::future<void>> threads{};

	// Invalidate any remaining children
	for (int idx = numChildren; idx < 4; idx++)
	{
		this->SetBounds(idx, vec3(1e34f), vec3(-1e34f));
		this->count[idx] = 0;
	}

	for (int idx = 0; idx < numChildren; idx++)
	{
		if (this->count[idx] == -1)
		{ // not a leaf
			const BVHNode *curNode = &bvhPool[this->child[idx]];

			if (curNode->IsLeaf())
			{
				this->count[idx] = curNode->GetCount();
				this->child[idx] = curNode->GetLeftFirst();
				this->SetBounds(idx, curNode->bounds);
				continue;
			}

			bvhTree->m_PoolPtrMutex.lock();
			const auto newIdx = bvhTree->m_FinalPtr++;
			bvhTree->m_PoolPtrMutex.unlock();

			MBVHNode *newNode = &bvhTree->m_Tree[newIdx];
			this->child[idx] = newIdx; // replace BVHNode idx with MBVHNode idx
			this->count[idx] = -1;
			this->SetBounds(idx, curNode->bounds);

			if (bvhTree->m_ThreadLimitReached || !thread)
			{
				newNode->MergeNodesMT(*curNode, bvhPool, bvhTree, !thread);
			}
			else
			{
				bvhTree->m_ThreadMutex.lock();
				bvhTree->m_BuildingThreads++;
				if (bvhTree->m_BuildingThreads > ctpl::nr_of_cores)
					bvhTree->m_ThreadLimitReached = true;
				bvhTree->m_ThreadMutex.unlock();

				threadCount++;
				threads.push_back(bvhTree->m_OriginalTree->m_ThreadPool->push(
					[newNode, curNode, bvhPool, bvhTree](int) { newNode->MergeNodesMT(*curNode, bvhPool, bvhTree); }));
			}
		}
	}

	for (int i = 0; i < threadCount; i++)
	{
		threads[i].get();
	}
}

void MBVHNode::GetBVHNodeInfo(const BVHNode &node, const BVHNode *pool, int &numChildren)
{
	// Starting values
	child[0] = child[1] = child[2] = child[3] = -1;
	count[0] = count[1] = count[2] = count[3] = -1;
	numChildren = 0;

	if (node.IsLeaf())
	{
		utils::WarningMessage(__FILE__, __LINE__, "This node shouldn't be a leaf.", "MBVHNode");
		return;
	}

	const BVHNode &leftNode = pool[node.GetLeftFirst()];
	const BVHNode &rightNode = pool[node.GetLeftFirst() + 1];

	if (leftNode.IsLeaf())
	{
		// node only has a single child
		const int idx = numChildren++;
		SetBounds(idx, leftNode.bounds);
		child[idx] = leftNode.GetLeftFirst();
		count[idx] = leftNode.GetCount();
	}
	else
	{
		// Node has 2 children
		const int idx1 = numChildren++;
		const int idx2 = numChildren++;
		child[idx1] = leftNode.GetLeftFirst();
		child[idx2] = leftNode.GetLeftFirst() + 1;
	}

	if (rightNode.IsLeaf())
	{
		// Node only has a single child
		const int idx = numChildren++;
		SetBounds(idx, rightNode.bounds);
		child[idx] = rightNode.GetLeftFirst();
		count[idx] = rightNode.GetCount();
	}
	else
	{
		// Node has 2 children
		const int idx1 = numChildren++;
		const int idx2 = numChildren++;
		SetBounds(idx1, pool[rightNode.GetLeftFirst()].bounds);
		SetBounds(idx2, pool[rightNode.GetLeftFirst() + 1].bounds);
		child[idx1] = rightNode.GetLeftFirst();
		child[idx2] = rightNode.GetLeftFirst() + 1;
	}
}

void MBVHNode::GetBVHNodeInfo(const BVHNode &node, const std::vector<bvh::BVHNode> &pool, int &numChildren)
{
	// starting values
	child[0] = child[1] = child[2] = child[3] = -1;
	count[0] = count[1] = count[2] = count[3] = -1;
	numChildren = 0;

	if (node.IsLeaf())
	{
		utils::WarningMessage(__FILE__, __LINE__, "This node shouldn't be a leaf.", "MBVHNode");
		return;
	}

	const BVHNode &leftNode = pool[node.GetLeftFirst()];
	const BVHNode &rightNode = pool[node.GetLeftFirst() + 1];

	if (leftNode.IsLeaf())
	{
		// node only has a single child
		const int idx = numChildren++;
		SetBounds(idx, leftNode.bounds);
		child[idx] = leftNode.GetLeftFirst();
		count[idx] = leftNode.GetCount();
	}
	else
	{
		// Node has 2 children
		const int idx1 = numChildren++;
		const int idx2 = numChildren++;
		child[idx1] = leftNode.GetLeftFirst();
		child[idx2] = leftNode.GetLeftFirst() + 1;
	}

	if (rightNode.IsLeaf())
	{
		// Node only has a single child
		const int idx = numChildren++;
		SetBounds(idx, rightNode.bounds);
		child[idx] = rightNode.GetLeftFirst();
		count[idx] = rightNode.GetCount();
	}
	else
	{
		// Node has 2 children
		const int idx1 = numChildren++;
		const int idx2 = numChildren++;
		SetBounds(idx1, pool[rightNode.GetLeftFirst()].bounds);
		SetBounds(idx2, pool[rightNode.GetLeftFirst() + 1].bounds);
		child[idx1] = rightNode.GetLeftFirst();
		child[idx2] = rightNode.GetLeftFirst() + 1;
	}
}

// https://stackoverflow.com/questions/25070577/sort-4-numbers-without-array
void MBVHNode::SortResults(const float *tmin, int &a, int &b, int &c, int &d) const
{
	if (tmin[a] > tmin[b])
	{
		std::swap(a, b);
	}
	if (tmin[c] > tmin[d])
	{
		std::swap(c, d);
	}
	if (tmin[a] > tmin[c])
	{
		std::swap(a, c);
	}
	if (tmin[b] > tmin[d])
	{
		std::swap(b, d);
	}
	if (tmin[b] > tmin[c])
	{
		std::swap(b, c);
	}
}

MBVHHit MBVHNode::Intersect(core::Ray &r, const __m128 &dirX, const __m128 &dirY, const __m128 &dirZ,
							const __m128 &orgX, const __m128 &orgY, const __m128 &orgZ) const
{
	MBVHHit mHit;

	const __m128 tx1 = _mm_mul_ps(_mm_sub_ps(bminx4, orgX), dirX);
	const __m128 tx2 = _mm_mul_ps(_mm_sub_ps(bmaxx4, orgX), dirX);

	__m128 tmax4 = _mm_max_ps(tx1, tx2);
	mHit.tmin4 = _mm_min_ps(tx1, tx2);

	const __m128 ty1 = _mm_mul_ps(_mm_sub_ps(bminy4, orgY), dirY);
	const __m128 ty2 = _mm_mul_ps(_mm_sub_ps(bmaxy4, orgY), dirY);

	mHit.tmin4 = _mm_max_ps(mHit.tmin4, _mm_min_ps(ty1, ty2));
	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(ty1, ty2));

	const __m128 tz1 = _mm_mul_ps(_mm_sub_ps(bminz4, orgZ), dirZ);
	const __m128 tz2 = _mm_mul_ps(_mm_sub_ps(bmaxz4, orgZ), dirZ);

	tmax4 = _mm_min_ps(tmax4, _mm_max_ps(tz1, tz2));
	mHit.tmin4 = _mm_max_ps(mHit.tmin4, _mm_min_ps(tz1, tz2));

	mHit.result = _mm_movemask_ps(
		_mm_and_ps(_mm_cmpge_ps(tmax4, zerops),
				   _mm_and_ps(_mm_cmple_ps(mHit.tmin4, tmax4), _mm_cmplt_ps(mHit.tmin4, _mm_set1_ps(r.t)))));

	if (mHit.result > 0)
	{
		// sort based on tmin
		mHit.tmini[0] &= 0xFFFFFFFC;
		mHit.tmini[1] &= 0xFFFFFFFC;
		mHit.tmini[2] &= 0xFFFFFFFC;
		mHit.tmini[3] &= 0xFFFFFFFC;

		mHit.tmini[0] = (mHit.tmini[0] | 0b00);
		mHit.tmini[1] = (mHit.tmini[1] | 0b01);
		mHit.tmini[2] = (mHit.tmini[2] | 0b10);
		mHit.tmini[3] = (mHit.tmini[3] | 0b11);

		if (mHit.tmin[0] > mHit.tmin[1])
		{
			std::swap(mHit.tmin[0], mHit.tmin[1]);
		}
		if (mHit.tmin[2] > mHit.tmin[3])
		{
			std::swap(mHit.tmin[2], mHit.tmin[3]);
		}
		if (mHit.tmin[0] > mHit.tmin[2])
		{
			std::swap(mHit.tmin[0], mHit.tmin[2]);
		}
		if (mHit.tmin[1] > mHit.tmin[3])
		{
			std::swap(mHit.tmin[1], mHit.tmin[3]);
		}
		if (mHit.tmin[2] > mHit.tmin[3])
		{
			std::swap(mHit.tmin[2], mHit.tmin[3]);
		}
	}

	return mHit;
}
}; // namespace bvh