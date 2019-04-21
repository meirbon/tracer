#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Core/Surface.h"
#include "Primitives/SceneObject.h"
#include "Shared.h"

#define LIGHT_MASK 0x01

enum MaterialType
{
	Light = 1,
	Lambert = 2,
	Specular = 3,
	Fresnel = 4
};

struct Material
{
  public:
	Material() = default;

	Material(float diffuse, glm::vec3 colorDiffuse, float shininess, float refractionIndex = 1.0f,
			 float transparency = 0.0f, glm::vec3 absorption = glm::vec3(0.0f));

	Material(float diffuse, glm::vec3 colorDiffuse, float shininess, float refractionIndex, float transparency);

	Material(float diffuse, glm::vec3 colorDiffuse, float shininess, unsigned int textureIdx);

	Material(float diffuse, glm::vec3 colorDiffuse, float shineness, float refractionIndex, float transparency,
			 glm::vec3 absorption, unsigned int textureIdx);

	Material(float diffuse, glm::vec3 albedo, float refractionIndex, float transparency, glm::vec3 absorption);

	Material(glm::vec3 emittance, float intensity);

	~Material() = default;

	union {
		glm::vec3 colorDiffuse = glm::vec3(1.f), albedo; // 12
	};
	union {
		float diffuse{}; // 16
		float intensity;
	};

	union {
		glm::vec3 emission{}; // 28
	};

	float shininess{}; // 32

	glm::vec3 absorption{};  // 44
	float refractionIndex{}; // 48
	float transparency{};	// 52
	int textureIdx = -1;	 // 56
	unsigned int flags = 0;  // 60
	float dummy{};			 // 64;

	glm::vec3 GetDiffuseColor(const prims::SceneObject *obj, const glm::vec3 &hitPoint) const;

	glm::vec3 GetAlbedoColor(const prims::SceneObject *obj, const glm::vec3 &hitPoint) const;

	inline const glm::vec3 &GetSpecularColor() const { return this->colorDiffuse; }

	inline const float &GetDiffuse() const { return this->diffuse; }

	inline float GetSpecular() const { return 1.f - this->diffuse; }

	inline const float &GetIntensity() const { return this->intensity; }

	inline const float &GetTransparency() const { return this->transparency; }

	inline bool IsDiffuse() const { return this->diffuse > EPSILON; }

	inline bool IsReflective() const { return (1.f - this->diffuse) > EPSILON; }

	inline bool IsTransparent() const { return this->transparency > EPSILON; }

	inline const glm::vec3 &GetEmission() const { return emission; }

	inline const glm::vec3 GetIntensifiedEmission() const { return emission * intensity; }

	inline void MakeLight() { this->flags = LIGHT_MASK; }

	inline bool IsLight() const { return (this->flags & LIGHT_MASK) == LIGHT_MASK; }

	static Material lambertian(glm::vec3 albedo, int diffuseTex = -1, int normalTex = -1, int maskTex = -1,
							   int displaceTex = -1, float refractionIdx = 1.0f)
	{
		Material mat;
		mat.albedo = albedo;
		mat.refractionIndex = refractionIdx;
		mat.textureIdx = diffuseTex;
		mat.shininess = 1.0f;
		mat.flags = 2;
		// mat.normalTex = normalTex;
		// mat.maskTex = maskTex;
		// mat.displaceTex = displaceTex;
		// mat.type = Lambertian;
		return mat;
	}

	static Material light(glm::vec3 emission)
	{
		Material mat;
		mat.refractionIndex = 1.0f;
		mat.textureIdx = -1;
		mat.MakeLight();
		mat.emission = glm::max(emission, glm::vec3(0.01f));
		mat.absorption = glm::vec3(0.0f);
		mat.flags = 1;
		return mat;
	}

	static Material specular(glm::vec3 albedo, float refractionIdx)
	{
		Material mat;
		mat.diffuse = 0.0f;
		mat.albedo = albedo;
		mat.refractionIndex = refractionIdx;
		mat.shininess = 10000.0f;
		mat.flags = 3;
		return mat;
	}

	static Material fresnel(glm::vec3 albedo, float refractIdx, glm::vec3 absorption = glm::vec3(0.0f),
							float roughness = 0.0f, int diffuseTex = -1, int normalTex = -1)
	{
		Material mat;
		mat.albedo = albedo;
		mat.refractionIndex = refractIdx;
		mat.textureIdx = diffuseTex;
		mat.absorption = absorption;
		mat.shininess = glm::max(roughness, 2.0f);
		mat.diffuse = 0.0f;
		mat.flags = 4;
		return mat;
	}
};
