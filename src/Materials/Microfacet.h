/*
 * Created by tatsy
 * Taken from https://github.com/tatsy/MicrofacetDistribution
 */

#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>

#include "Utils/RandomGenerator.h"

#ifndef PI
#define PI 3.14159265358979323846264338327950288419716939937510582097494459072381640628620899862803482534211706798f
#endif

inline float clamp(float value, float lower, float upper)
{
	if (value >= upper)
	{
		return upper;
	}

	if (value <= lower)
	{
		return lower;
	}

	return value;
}

inline float cosTheta(const glm::vec3 &w) { return w.z; }

inline float cos2Theta(const glm::vec3 &w)
{
	float c = cosTheta(w);
	return c * c;
}

inline float sinTheta(const glm::vec3 &w) { return sqrtf(glm::max(FLT_MIN, 1.0f - cos2Theta(w))); }

inline float tanTheta(const glm::vec3 &w)
{
	if (w.z == 0.0f)
		return 0.0f;
	return sinTheta(w) / cosTheta(w);
}

inline float cosPhi(const glm::vec3 &w)
{
	if (w.z == 1.0f)
		return 0.0f;

	const float tmp = sinTheta(w);
	return tmp == 0.0f ? 0.0f : w.x / tmp;
}

inline float cos2Phi(const glm::vec3 &w)
{
	const float c = cosPhi(w);
	return c * c;
}

inline float sinPhi(const glm::vec3 &w)
{
	if (w.z == 1.0f)
		return 0.0f;

	return w.y / sinTheta(w);
}

inline float sin2Phi(const glm::vec3 &w)
{
	const float s = sinPhi(w);
	return s * s;
}

// Rational approximation of error function
// This method is borrowed from PBRT v3
inline float mf_erf(float x)
{
	// constants
	const float a1 = 0.254829592f;
	const float a2 = -0.284496736f;
	const float a3 = 1.421413741f;
	const float a4 = -1.453152027f;
	const float a5 = 1.061405429f;
	const float p = 0.3275911f;

	// Save the sign of x
	const int sign = (x < 0) ? -1 : 1;
	x = fabs(x);

	// A&S formula 7.1.26
	const float t = 1 / (1 + p * x);
	const float y = 1.0f - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * expf(-x * x);

	return sign * y;
}

// ----------------------------------------------------------------------------
// The interface for microfacet distribution, GGX distribution by default
// ----------------------------------------------------------------------------
class Microfacet
{
  public:
	Microfacet() = default;
	Microfacet(float alphax, float alphay) : m_AlphaX{alphax}, m_AlphaY{alphay} {}

	~Microfacet() = default;

	// Sample microfacet normal
	inline glm::vec3 sampleWm(RandomGenerator &rng) const
	{
		const float U1 = rng.Rand(1.0f);
		const float U2 = rng.Rand(1.0f);

		float tan2Theta, phi;
		if (m_AlphaX == m_AlphaY)
		{
			// Isotropic case
			const float alpha = m_AlphaX;
			tan2Theta = alpha * alpha * U1 / (1.0f - U1);
			phi = 2.0f * PI * U2;
		}
		else
		{
			// Anisotropic case
			phi = atanf(m_AlphaY / m_AlphaX * tanf(2.0f * PI * U2 + 0.5f * PI));
			if (U2 > 0.5)
			{
				phi += PI;
			}

			const float sinPhi = sinf(phi);
			const float cosPhi = cosf(phi);
			const float alphax2 = m_AlphaX * m_AlphaX;
			const float alphay2 = m_AlphaY * m_AlphaY;
			const float alpha2 = 1.0f / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
			tan2Theta = U1 / (1.0f - U1) * alpha2;
		}

		// Compute normal direction from angle information sampled above
		const float cosTheta = 1.0f / sqrtf(1.0f + tan2Theta);
		const float sinTheta = sqrtf(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		glm::vec3 wm = glm::vec3(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);

		if (wm.z < 0.0)
		{
			wm = -wm;
		}

		return wm;
	}

	// The lambda function, which appears in the slope-space sampling
	inline float lambda(const glm::vec3 &wo) const
	{
		const float absTanThetaO = fabs(tanTheta(wo));
		if (std::isinf(absTanThetaO))
			return 0.f;

		const float alpha = sqrtf(cos2Phi(wo) * m_AlphaX * m_AlphaX + sin2Phi(wo) * m_AlphaY * m_AlphaY);
		const float alpha2Tan2Theta = (alpha * absTanThetaO) * (alpha * absTanThetaO);
		return (-1.0f + sqrtf(1.f + alpha2Tan2Theta)) / 2;
	}

	// Smith's masking function
	inline float G1(const glm::vec3 &wo) const { return 1.0f / (1.0f + lambda(wo)); }

	// Smith's masking-shadowing function
	inline float G(const glm::vec3 &wo, const glm::vec3 &wi) const { return G1(wo) * G1(wi); }

	// Weighting function
	inline float weight(const glm::vec3 &wo, const glm::vec3 &wi, const glm::vec3 &wm) const
	{
		return fabsf(glm::dot(wi, wm)) * G(wo, wi) / std::max(1.0e-8f, fabsf(cosTheta(wi) * cosTheta(wm)));
	}

  public:
	// Private parameters
	float m_AlphaX, m_AlphaY;
};

//
////
///----------------------------------------------------------------------------
//// Beckmann distribution
////
///----------------------------------------------------------------------------
// class BeckmannDistribution : public Microfacet {
// public:
//	BeckmannDistribution(float alphax, float alphay)
//		: Microfacet{ alphax, alphay }
//	{
//	}
//
//	glm::vec3 sampleWm(const glm::vec3& wo) const override
//	{
//		const float U1 = Rand(1.0f);
//		const float U2 = Rand(1.0f);
//
//		// Sample half-glm::vec3tor without taking normal visibility
// into
// consideration 		float tan2Theta, phi; 		if (m_AlphaX ==
// m_AlphaY) {
//			// Isotropic case
//			// Following smapling formula can be found in [Walter et
// al. 2007]
//			// "Microfacet Models for Refraction through Rough
// Surfaces" 			float alpha = m_AlphaX; 			float
// logSample = std::log(1.0f - U1); 			tan2Theta = -alpha * alpha * logSample; 			phi
//= 2.0f * PI * U2; 		} else {
//			// Anisotropic case
//			// Following sampling strategy is analytically derived
// from
//			// P.15 of [Heitz et al. 2014]
//			// "Understanding the Masking-Shadowing Function in
// Microfacet-Based BRDFs" 			float logSample = std::log(1.0f - U1);
// phi = std::atan(m_AlphaY / m_AlphaX * std::tan(2.0f * PI * U2 + 0.5 * PI));
// if (U2 > 0.5) { 				phi += PI;
//			}
//
//			float sinPhi = std::sin(phi);
//			float cosPhi = std::cos(phi);
//			float alphax2 = m_AlphaX * m_AlphaX;
//			float alphay2 = m_AlphaY * m_AlphaY;
//			tan2Theta = -logSample / (cosPhi * cosPhi / alphax2 +
// sinPhi
//* sinPhi / alphay2);
//		}
//
//		// Compute normal direction from angle information sampled above
//		float cosTheta = 1.0f / std::sqrt(1.0f + tan2Theta);
//		float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta *
// cosTheta)); 		glm::vec3 wm(sinTheta * std::cos(phi), sinTheta *
// std::sin(phi), cosTheta);
//
//		if (wm.z < 0.0) {
//			wm = -wm;
//		}
//
//		return wm;
//	}
//
//	float lambda(const glm::vec3& wo) const override
//	{
//		float cosThetaO = cosTheta(wo);
//		float sinThetaO = sinTheta(wo);
//		float absTanThetaO = std::abs(tanTheta(wo));
//		if (std::isinf(absTanThetaO))
//			return 0.f;
//
//		float alpha = std::sqrt(cosThetaO * cosThetaO * m_AlphaX *
// m_AlphaX + sinThetaO * sinThetaO * m_AlphaY * m_AlphaY); 		float a
// = 1.0f /
//(alpha * absTanThetaO); 		return (mf_erf(a) - 1.0f) / 2.0f +
// std::exp(-a * a) / (2.0f * a * std::sqrt(PI) + 1.0e-6f);
//	}
//};
