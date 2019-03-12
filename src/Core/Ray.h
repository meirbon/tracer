#pragma once

#include <glm/glm.hpp>

#include "Primitives/SceneObject.h"
#include "Utils/Template.h"

struct Ray
{
  public:
    Ray() = default;

    Ray(const glm::vec3 &a, const glm::vec3 &b);

    inline bool IsValid() const { return this->obj != nullptr; }

    inline void Reset()
    {
        this->t = 1e34f;
        this->obj = nullptr;
    }

    glm::vec3 GetHitpoint() const;

    Ray Reflect(const glm::vec3 &point, const glm::vec3 &normal) const;

    Ray Reflect(const glm::vec3 &normal) const;

    Ray DiffuseReflection(const glm::vec3 &point, const glm::vec3 &normal,
                          RandomGenerator &rng) const;

    Ray DiffuseReflection(const glm::vec3 &normal, RandomGenerator &rng) const;

    Ray CosineWeightedDiffuseReflection(const glm::vec3 &origin,
                                        const glm::vec3 &normal,
                                        RandomGenerator &rng) const;

    glm::vec3 TransformToTangent(const glm::vec3 &normal,
                                 glm::vec3 vector) const;

    union {
        struct
        {
            glm::vec3 direction;
            float t;
        };
        __m128 m_Direction4;
    };

    union {
        struct
        {
            glm::vec3 origin;
            float wO;
        };
        __m128 m_Origin4;
    };

    const SceneObject *obj;
    glm::vec3 normal;
};
