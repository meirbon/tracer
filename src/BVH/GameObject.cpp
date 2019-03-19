#include "GameObject.h"

namespace bvh
{
GameObject::GameObject(prims::WorldScene *bvhTree)
{
	this->isLeaf = true;
	this->m_BVHTree = bvhTree;
	this->m_TransformationMat = mat4(1.f);
	this->m_InverseMat = mat4(1.f);
}

GameObject::GameObject(std::vector<GameObject *> *children)
{
	this->isLeaf = false;
	this->m_Children = children;
	this->m_BVHTree = nullptr;
	for (auto *obj : *children)
		obj->m_Parent = this;
}

GameObject::~GameObject() { delete m_Children; }

AABB GameObject::GetBounds() const
{
	if (this->m_BVHTree != nullptr && this->m_BVHTree->GetPrimitiveCount() > 0)
		return this->m_BVHTree->GetNodeBounds(0);

	return {};
}

void GameObject::Move(glm::vec3 movement)
{
	this->m_TransformationMat = glm::translate(this->m_TransformationMat, -movement);
	this->m_InverseMat = glm::inverse(this->m_TransformationMat);
}

void GameObject::Rotate(float angle, glm::vec3 rotationAxis)
{
	this->m_TransformationMat = glm::rotate(mat4(1.f), angle, rotationAxis) * this->m_TransformationMat;
	this->m_InverseMat = glm::inverse(this->m_TransformationMat);
}

void GameObject::UpdateDynamicBVHNode(glm::mat4 matrix)
{
	if (this->IsLeaf() && this->m_Parent != nullptr)
	{
		this->m_Parent->UpdateDynamicBVHNode(matrix);
	}
}
} // namespace bvh