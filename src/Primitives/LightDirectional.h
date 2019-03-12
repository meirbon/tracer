#pragma once

#include "BVH/AABB.h"
#include "Light.h"
#include <glm/glm.hpp>

class LightDirectional : public Light
{
  public:
    LightDirectional() = default;

    explicit LightDirectional(glm::vec3 direction, unsigned int matIdx);

    //    glm::vec3 CalculateLight(const Ray& r, const Material* mat, const
    //    WorldScene* m_Scene) const override;

    ~LightDirectional() override = default;

    void Intersect(Ray &r) const override;

    glm::vec3 Direction{};

    // Inherited via Light
    bvh::AABB GetBounds() const override;

    glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction,
                                      glm::vec3 &lNormal,
                                      RandomGenerator &rng) const override;

    glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;

    glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
