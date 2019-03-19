#pragma once

#include <glm/glm.hpp>

#include "BVH/AABB.h"
#include "Primitives/SceneObject.h"

namespace prims
{
class Torus : public SceneObject
{
  public:
	Torus() = default;
	Torus(glm::vec3 center, glm::vec3 vAxis, glm::vec3 hAxis, float innerRadius, float outerRadius,
		  unsigned int matIdx);
	~Torus() = default;

	void Intersect(core::Ray &r) const override;

	glm::vec3 m_Axis{}, m_HorizontalAxis;
	float m_InnerRadius{}, m_OuterRadius{};
	float m_TorusPlaneOffset{};
	bvh::AABB GetBounds() const override;
	glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal,
									  RandomGenerator &rng) const override;
	glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
	glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
} // namespace prims