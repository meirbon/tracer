#pragma once

#include <glm/glm.hpp>

#include "BVH/AABB.h"
#include "SceneObject.h"

struct Ray;

class Sphere : public SceneObject
{
  public:
    Sphere() = default;
    explicit Sphere(glm::vec3 pos, float radius, unsigned int matIdx);
    void Intersect(Ray &r) const override;
    ~Sphere() = default;

    // pos 12
    // mat 16
    float radiusSquared{}; // 20

    bvh::AABB GetBounds() const override;
    glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction,
                                      glm::vec3 &lNormal,
                                      RandomGenerator &rng) const override;
    glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
    glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
