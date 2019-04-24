#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Core/Surface.h"
#include "Primitives/SceneObject.h"
#include "Shared.h"

enum MaterialType
{
	Undefined = 0,
	Light = 1,
	Lambert = 2,
	Specular = 3,
	Fresnel = 4
};

struct Material
{
  public:
	union {
		glm::vec3 albedo = glm::vec3(1.f); // 12
		glm::vec3 emission;
	};
	union {
		float diffuse{}; // 16
		float intensity;
	};

	glm::vec3 absorption = glm::vec3(0.0f); // 28
	float refractionIndex = 1.0f;			// 32
	int textureIdx = -1;					// 36
	MaterialType type = Undefined;			// 40
	int normalTex = -1;						// 44
	int maskTex = -1;						// 48
	int displaceTex = -1;					// 52
	float roughnessX = 1.0f;				// 56
	float roughnessY = 1.0f;				// 60
	float dummy;							// 64

	static Material lambertian(glm::vec3 albedo, int diffuseTex = -1, int normalTex = -1, int maskTex = -1,
							   int displaceTex = -1, float roughness = 1.0f)
	{
		Material mat;
		mat.albedo = albedo;
		mat.textureIdx = diffuseTex;
		mat.type = Lambert;
		mat.normalTex = normalTex;
		mat.maskTex = maskTex;
		mat.displaceTex = displaceTex;
		mat.roughnessX = roughness;
		mat.roughnessY = roughness;
		return mat;
	}

	static Material light(glm::vec3 emission)
	{
		Material mat;
		mat.refractionIndex = 1.0f;
		mat.textureIdx = -1;
		mat.emission = glm::max(emission, glm::vec3(0.01f));
		mat.absorption = glm::vec3(0.0f);
		mat.type = Light;
		return mat;
	}

	static Material specular(glm::vec3 albedo, float refractionIdx)
	{
		Material mat;
		mat.diffuse = 0.0f;
		mat.albedo = albedo;
		mat.refractionIndex = refractionIdx;
		mat.type = Specular;
		return mat;
	}

	static Material fresnel(glm::vec3 albedo, float refractIdx, glm::vec3 absorption = glm::vec3(0.0f),
							float roughness = 0.0f, int diffuseTex = -1, int normalTex = -1)
	{
		Material mat;
		mat.albedo = albedo;
		mat.refractionIndex = refractIdx;
		mat.textureIdx = diffuseTex;
		mat.normalTex = normalTex;
		mat.absorption = absorption;
		mat.diffuse = 0.0f;
		mat.type = Fresnel;
		mat.roughnessX = roughness;
		mat.roughnessY = roughness;
		return mat;
	}
};
