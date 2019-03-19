#pragma once

#include <glm/glm.hpp>

#include "Materials/Material.h"
#include "Primitives/SceneObject.h"

namespace prims
{

class LightPoint : public SceneObject
{
  public:
	LightPoint() = default;
	LightPoint(glm::vec3 pos, float radius, unsigned int matIndex);
	//    glm::vec3 CalculateLight(const Ray& r, const material::Material* mat,
	//    const WorldScene* m_Scene) const override;
	void Intersect(core::Ray &r) const override;

	~LightPoint() override = default;

	float m_RadiusSquared{};

	bvh::AABB GetBounds() const override;
	glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal,
									  RandomGenerator &rng) const override;
	glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
	glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
} // namespace prims
