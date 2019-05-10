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

unsigned int MBVHNode::IntersectDebug(core::Ray &r, const glm::vec3 &dirInverse, const MBVHNode *pool) const
{
	unsigned int depth = 1;
	MBVHHit hit{};

	vec4 t1 = (minx - r.origin.x) * dirInverse.x;
	vec4 t2 = (maxx - r.origin.x) * dirInverse.x;

	hit.tmin4 = glm::min(t1, t2);
	vec4 tmax = glm::max(t1, t2);

	t1 = (miny - r.origin.y) * dirInverse.y;
	t2 = (maxy - r.origin.y) * dirInverse.y;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	t1 = (minz - r.origin.z) * dirInverse.z;
	t2 = (maxz - r.origin.z) * dirInverse.z;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	hit.result = greaterThan(tmax, vec4(0.0f)) && lessThanEqual(hit.tmin4, tmax) && lessThan(hit.tmin4, vec4(r.t));

	if (glm::any(hit.result))
	{
		hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
		hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
		hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
		hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

		if (hit.tmin[0] > hit.tmin[1])
			std::swap(hit.tmin[0], hit.tmin[1]);
		if (hit.tmin[2] > hit.tmin[3])
			std::swap(hit.tmin[2], hit.tmin[3]);
		if (hit.tmin[0] > hit.tmin[2])
			std::swap(hit.tmin[0], hit.tmin[2]);
		if (hit.tmin[1] > hit.tmin[3])
			std::swap(hit.tmin[1], hit.tmin[3]);
		if (hit.tmin[2] > hit.tmin[3])
			std::swap(hit.tmin[2], hit.tmin[3]);

		for (unsigned int t : hit.tmini)
		{
			const uint idx = (t & 0b11);
			if (!hit.result[idx])
				continue;

			if (this->count[idx] > -1)
				depth += 1;
			else
				depth += pool[this->child[idx]].IntersectDebug(r, dirInverse, pool);
		}
	}

	return depth;
}

void MBVHNode::Intersect(core::Ray &r, const glm::vec3 &dirInverse, const MBVHNode *pool,
						 const std::vector<unsigned int> &primitiveIndices,
						 const std::vector<prims::SceneObject *> &objectList) const
{
	MBVHHit hit{};

	vec4 t1 = (minx - r.origin.x) * dirInverse.x;
	vec4 t2 = (maxx - r.origin.x) * dirInverse.x;

	hit.tmin4 = glm::min(t1, t2);
	vec4 tmax = glm::max(t1, t2);

	t1 = (miny - r.origin.y) * dirInverse.y;
	t2 = (maxy - r.origin.y) * dirInverse.y;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	t1 = (minz - r.origin.z) * dirInverse.z;
	t2 = (maxz - r.origin.z) * dirInverse.z;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	hit.result = greaterThan(tmax, vec4(0.0f)) && lessThanEqual(hit.tmin4, tmax) && lessThan(hit.tmin4, vec4(r.t));

	if (glm::any(hit.result))
	{
		hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
		hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
		hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
		hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

		if (hit.tmin[0] > hit.tmin[1])
			std::swap(hit.tmin[0], hit.tmin[1]);
		if (hit.tmin[2] > hit.tmin[3])
			std::swap(hit.tmin[2], hit.tmin[3]);
		if (hit.tmin[0] > hit.tmin[2])
			std::swap(hit.tmin[0], hit.tmin[2]);
		if (hit.tmin[1] > hit.tmin[3])
			std::swap(hit.tmin[1], hit.tmin[3]);
		if (hit.tmin[2] > hit.tmin[3])
			std::swap(hit.tmin[2], hit.tmin[3]);

		for (unsigned int t : hit.tmini)
		{
			const uint idx = (t & 0b11);
			if (!hit.result[idx])
				continue;

			if (this->count[idx] > -1) // leaf node
			{
				for (int i = 0; i < this->count[idx]; i++)
				{
					const uint &primIdx = primitiveIndices[i + this->child[idx]];
					objectList[primIdx]->Intersect(r);
				}
			}
			else
			{
				pool[this->child[idx]].Intersect(r, dirInverse, pool, primitiveIndices, objectList);
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

MBVHHit MBVHNode::Intersect(core::Ray &r, const glm::vec3 &dirInverse) const
{
	MBVHHit hit{};

	vec4 t1 = (minx - r.origin.x) * dirInverse.x;
	vec4 t2 = (maxx - r.origin.x) * dirInverse.x;

	hit.tmin4 = glm::min(t1, t2);
	vec4 tmax = glm::max(t1, t2);

	t1 = (miny - r.origin.y) * dirInverse.y;
	t2 = (maxy - r.origin.y) * dirInverse.y;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	t1 = (minz - r.origin.z) * dirInverse.z;
	t2 = (maxz - r.origin.z) * dirInverse.z;

	hit.tmin4 = glm::max(hit.tmin4, glm::min(t1, t2));
	tmax = glm::min(tmax, glm::max(t1, t2));

	hit.tmini[0] = ((hit.tmini[0] & 0xFFFFFFFC) | 0b00);
	hit.tmini[1] = ((hit.tmini[1] & 0xFFFFFFFC) | 0b01);
	hit.tmini[2] = ((hit.tmini[2] & 0xFFFFFFFC) | 0b10);
	hit.tmini[3] = ((hit.tmini[3] & 0xFFFFFFFC) | 0b11);

	hit.result = greaterThan(tmax, vec4(0.0f)) && lessThanEqual(hit.tmin4, tmax) && lessThan(hit.tmin4, vec4(r.t));

	if (glm::any(hit.result) > 0)
	{
		if (hit.tmin[0] > hit.tmin[1])
		{
			std::swap(hit.tmin[0], hit.tmin[1]);
		}
		if (hit.tmin[2] > hit.tmin[3])
		{
			std::swap(hit.tmin[2], hit.tmin[3]);
		}
		if (hit.tmin[0] > hit.tmin[2])
		{
			std::swap(hit.tmin[0], hit.tmin[2]);
		}
		if (hit.tmin[1] > hit.tmin[3])
		{
			std::swap(hit.tmin[1], hit.tmin[3]);
		}
		if (hit.tmin[2] > hit.tmin[3])
		{
			std::swap(hit.tmin[2], hit.tmin[3]);
		}
	}

	return hit;
}
}; // namespace bvh