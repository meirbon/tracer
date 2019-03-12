#pragma once

#include "Light.h"
#include "Materials/Material.h"
#include <glm/glm.hpp>

class LightSpot : public Light
{
  public:
    LightSpot() = default;
    LightSpot(glm::vec3 position, glm::vec3 direction, float angleInner,
              float angleOuter, unsigned int matIdx, float objRadius);
    //    glm::vec3 CalculateLight(const Ray& r, const material::Material* mat,
    //    const WorldScene* m_Scene) const override;
    ~LightSpot() override = default;
    void Intersect(Ray &r) const override;

    glm::vec3 Direction{};
    float cosAngleInner{};
    float cosAngleOuter{};
    float m_RadiusSquared{};

    bvh::AABB GetBounds() const override;
    glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction,
                                      glm::vec3 &lNormal,
                                      RandomGenerator &rng) const override;
    glm::vec3 GetNormal(const glm::vec3 &hitPoint) const override;
    glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const override;
};
