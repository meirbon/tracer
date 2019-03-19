#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "BVH/StaticBVHTree.h"

namespace bvh
{
class GameObjectNode;
class TopLevelBVH;

class GameObject
{
	friend class GameObjectNode;
	friend class TopLevelBVH;

  public:
	GameObject() = default;
	explicit GameObject(prims::WorldScene *bvhTree);
	explicit GameObject(std::vector<GameObject *> *children);
	~GameObject();

	bvh::AABB GetBounds() const;

	void Move(glm::vec3 movement);
	void Rotate(float angle, glm::vec3 rotationAxis);

	void UpdateDynamicBVHNode(glm::mat4 matrix);

	inline bool IsLeaf() const { return isLeaf; }

	inline std::vector<GameObject *> &GetChildren() { return *m_Children; }

	inline GameObject *AddChild(prims::WorldScene *childBVH)
	{
		auto *child = new GameObject(childBVH);
		m_Children->push_back(child);
		return child;
	}

  private:
	bool isLeaf = false;
	std::vector<GameObject *> *m_Children = nullptr;
	prims::WorldScene *m_BVHTree{};
	glm::mat4 m_TransformationMat = glm::mat4(1.f);
	glm::mat4 m_InverseMat = glm::mat4(1.f);

	bvh::GameObjectNode *m_DynamicBVHNode = nullptr;
	bvh::GameObject *m_Parent = nullptr;
};
} // namespace bvh