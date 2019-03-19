#include "Materials/MaterialMicrofacet.h"
#include "Materials/Microfacet.h"

inline float cos_phi(const glm::vec3 &w)
{
	if (w.z == 1.0f)
		return 0.0f;

	const float tmp = sinTheta(w);
	return tmp == 0.0f ? 0.0f : w.x / tmp;
}

inline float cos2_phi(const glm::vec3 &w)
{
	const float c = cosPhi(w);
	return c * c;
}

inline float sin_phi(const glm::vec3 &w)
{
	if (w.z == 1.0f)
		return 0.0f;

	return w.y / sinTheta(w);
}

inline float sin2_phi(const glm::vec3 &w)
{
	const float s = sin_phi(w);
	return s * s;
}

MaterialMicrofacet::MaterialMicrofacet(float alphaX, float alphaY) : m_AlphaX(alphaX), m_AlphaY(alphaY) {}

inline vec3 from_tangent_to_local(const vec3 &normal, const vec3 &tangent)
{
	vec3 t;
	if (fabs(normal.x) > 0.99f)
	{
		t = normalize(cross(normal, vec3(0, 1, 0)));
	}
	else
	{
		t = normalize(cross(normal, vec3(1, 0, 0)));
	}
	const vec3 b = cross(normal, t);
	return tangent.x * t + tangent.y * b + tangent.z * normal;
}

glm::vec3 MaterialMicrofacet::SampleNormal(const vec3 &inDirection, const vec3 &normal, RandomGenerator &rng) const
{
	glm::vec3 Nt, Nb;
	rng.createCoordinateSystem(normal, Nt, Nb);

	float r0 = rng.Rand(1.0f);
	float r1 = rng.Rand(1.0f);

	float t = powf(r0, 2.0f / ((m_AlphaX + m_AlphaY / 2.0f) + 1.0f));

	float phi = 2.0f * PI * r1;
	float sqrt_1_min_t = sqrtf(1.0f - t);

	float x = cosf(phi) * sqrt_1_min_t;
	float z = sinf(phi) * sqrt_1_min_t;
	float y = sqrtf(t);

	return rng.localToWorld(vec3(x, y, z), Nt, Nb, normal);
}

inline float distribution_for_normal(float ndoth, float alpha)
{
	const float FRAC_1_2PI = 1.0f / (2.0f * PI);
	return (alpha + 2.0f) * FRAC_1_2PI * powf(ndoth, alpha);
}

inline float geometric_term(float ndotv, float ndotl, float ndoth, float hdotv)
{
	// Physically Based Rendring, page 455
	const float frac2ndoth_hdtov = (2.0f * ndoth) / hdotv;

	return glm::min(1.0f, glm::min(frac2ndoth_hdtov * ndotv, frac2ndoth_hdtov * ndotl));
}

inline float fresnel_term(float hdotl)
{
	const float t = (1.0f - hdotl);
	return t * t * t * t * t;
}

float MaterialMicrofacet::Weight(const glm::vec3 &inDirection, const glm::vec3 &realNormal,
								 const glm::vec3 outDirection) const
{
	const vec3 halfway = normalize(inDirection + outDirection);

	const float ndotv = dot(realNormal, inDirection);
	const float ndotl = dot(realNormal, outDirection);
	const float ndoth = dot(realNormal, halfway);
	const float hdotv = dot(halfway, inDirection);
	const float hdotl = dot(halfway, outDirection);

	const float normal_distribution = distribution_for_normal(ndoth, (m_AlphaX + m_AlphaY) / 2.0f);
	const float geometric_t = geometric_term(ndotv, ndotl, ndoth, hdotv);
	const float fresnel_t = fresnel_term(hdotl);
	const float normalization = 1.0f / (4.0f * ndotl * ndotv);

	return normal_distribution * geometric_t * normalization * fresnel_t;
}

float MaterialMicrofacet::lambda(const glm::vec3 outDirection) const
{
	const float absTanThetaO = fabs(tanTheta(outDirection));
	if (isinf(absTanThetaO))
	{
		return 0.f;
	}

	const float alpha =
		sqrtf(cos2_phi(outDirection) * m_AlphaX * m_AlphaX + sin2_phi(outDirection) * m_AlphaY * m_AlphaY);
	const float alpha2Tan2Theta = (alpha * absTanThetaO) * (alpha * absTanThetaO);
	return (-1.0f + sqrtf(1.f + alpha2Tan2Theta)) / 2.0f;
}
