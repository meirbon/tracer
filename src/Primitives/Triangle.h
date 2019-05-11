#pragma once

#include <glm/glm.hpp>

#include "Primitives/SceneObject.h"

using namespace glm;

namespace prims
{
class Triangle : public SceneObject
{
  public:
	Triangle() = default;
	Triangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex);
	Triangle(vec3 p0, vec3 p1, vec3 p2, uint matIndex, vec2 t0, vec2 t1, vec2 t2);
	Triangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2, uint matIndex);
	Triangle(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2, uint matIndex, vec2 t0, vec2 t1, vec2 t2);
	~Triangle() override = default;

	vec3 p0{}, p1{}, p2{}; // 36
	vec3 n0{}, n1{}, n2{}; // 36
	vec2 t0{}, t1{}, t2{}; // 24
	vec3 normal{};		   // 12

	void Intersect(core::Ray &r) const override;
	const vec3 GetBarycentricCoordinatesAt(const vec3 &p) const;
	bvh::AABB GetBounds() const override;

	vec3 GetRandomPointOnSurface(const vec3 &direction, vec3 &lNormal, RandomGenerator &rng) const override;
	//    glm::vec3 CalculateLight(const Ray& r, const material::Material* mat,
	//    const WorldScene* m_Scene) const override;
	vec3 GetNormal(const vec3 &hitPoint) const override;
	vec2 GetTexCoords(const vec3 &hitPoint) const override;

	float CalculateArea() const;
};
}