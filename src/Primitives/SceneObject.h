#pragma once

struct Ray;
#include "Utils/RandomGenerator.h"
#include "BVH/AABB.h"

class SceneObject {
public:
	virtual void Intersect(Ray &r) const = 0;
	virtual bvh::AABB GetBounds() const = 0;

        virtual ~SceneObject() = default;
        ;

        glm::vec3 centroid; // 12
	unsigned int materialIdx, objIdx; // 16
	float m_Area;

	virtual glm::vec3 GetRandomPointOnSurface(const glm::vec3 &direction, glm::vec3 &lNormal, RandomGenerator& rng) const = 0;
	virtual glm::vec3 GetNormal(const glm::vec3 &hitPoint) const = 0;
	virtual glm::vec2 GetTexCoords(const glm::vec3 &hitPoint) const = 0;

	inline const glm::vec3 &GetPosition() const {
		return centroid;
	}

	inline const float &GetArea() const {
		return m_Area;
	}

};