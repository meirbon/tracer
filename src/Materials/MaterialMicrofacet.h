#pragma once

#include "Utils/RandomGenerator.h"
#include <glm/glm.hpp>

using namespace glm;

// using a GGX distribution
class MaterialMicrofacet
{
  public:
	MaterialMicrofacet() = default;
	~MaterialMicrofacet() = default;
	MaterialMicrofacet(float alphaX, float alphaY);

	vec3 SampleNormal(const vec3 &inDirection, const vec3 &realNormal, RandomGenerator &rng) const;
	float Weight(const glm::vec3 &inDirection, const glm::vec3 &realNormal, const glm::vec3 outDirection) const;

	inline float G(const vec3 &out, const vec3 &in) const { return G1(out) * G1(in); }

	inline float G1(const vec3 &v) const { return 1.0f / (1.0f + lambda(v)); }

  private:
	float lambda(glm::vec3 outDirection) const;
	float m_AlphaX, m_AlphaY;
};
