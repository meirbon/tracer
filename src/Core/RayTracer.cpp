#include "RayTracer.h"
#include "Materials/MaterialManager.h"
#include "Utils/MersenneTwister.h"
#include "Utils/Xor128.h"

using namespace gl;

namespace core
{

#define TILE_HEIGHT 32
#define TILE_WIDTH 32

RayTracer::RayTracer(prims::WorldScene *Scene, glm::vec3 backGroundColor, glm::vec3 ambientColor,
					 uint maxRecursionDepth, Camera *camera, int width, int height)
	: m_Width(width), m_Height(height)
{
	this->m_Scene = Scene;
	this->backGroundColor = backGroundColor;
	this->ambientColor = ambientColor;
	this->maxRecursionDepth = maxRecursionDepth;
	this->m_Camera = camera;
	this->tPool = new ctpl::ThreadPool(ctpl::nr_of_cores);
	this->m_Rngs = new std::vector<RandomGenerator *>();
	this->m_Tiles = (m_Width / TILE_WIDTH) * (m_Height / TILE_HEIGHT);
	this->tResults = new std::vector<std::future<void>>(m_Tiles);
	this->m_LightCount = static_cast<int>(m_Scene->GetLights().size());
	this->m_Pixels = new vec3[m_Width * m_Height];

	for (int i = 0; i < m_Tiles; i++)
	{
		this->m_Rngs->push_back(new Xor128());
	}
}

void RayTracer::Render(Surface *output)
{
	const int width = output->GetWidth();
	const int height = output->GetHeight();

	const int vTiles = m_Width / TILE_HEIGHT;
	const int hTiles = m_Height / TILE_WIDTH;

	unsigned int idx = 0;

	for (int tile_y = 0; tile_y < vTiles; tile_y++)
	{
		for (int tile_x = 0; tile_x < hTiles; tile_x++)
		{
			RandomGenerator *rngPointer = m_Rngs->at(idx);
			idx++;

			tResults->at(tile_y * hTiles + tile_x) =
				tPool->push([tile_x, tile_y, this, output, rngPointer](int threadId) -> void {
					for (int y = 0; y < TILE_HEIGHT; y++)
					{
						const int pixel_y = y + tile_y * TILE_HEIGHT;
						for (int x = 0; x < TILE_WIDTH; x++)
						{
							const int pixel_x = x + tile_x * TILE_WIDTH;
							uint depth = 0;
							Ray r = m_Camera->GenerateRay(float(pixel_x), float(pixel_y));
							const glm::vec3 color = Trace(r, depth, 1.f, *rngPointer);

							const int pidx = pixel_x + pixel_y * m_Width;

							if (m_Samples > 0)
							{
								const float factor = 1.0f / float(m_Samples + 1);
								m_Pixels[pidx] = m_Pixels[pidx] * float(m_Samples) * factor + color * factor;
							}
							else
							{
								m_Pixels[pidx] = color;
							}
							output->Plot(pixel_x, pixel_y, m_Pixels[pidx]);
						}
					}
				});
		}
	}

	for (auto &r : *tResults)
	{
		r.get();
	}

	m_Samples++;
}

glm::vec3 RayTracer::Trace(Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng)
{
	m_Scene->TraceRay(r);

	if (!r.IsValid())
	{
		return backGroundColor;
	}

	return Shade(r, depth, refractionIndex, rng);
}

glm::vec3 RayTracer::Shade(const Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng)
{
	if (depth >= maxRecursionDepth)
	{
		return glm::vec3(0.f);
	}

	const vec3 p = r.GetHitpoint();
	const auto mat = MaterialManager::GetInstance()->GetMaterial(r.obj->materialIdx);

	if (mat.IsLight())
	{
		return mat.emission;
	}

	const glm::vec3 diffuseColor =
		mat.IsDiffuse() ? GetDiffuseSpecularColor(r, mat, p, rng) * mat.GetDiffuse() : glm::vec3(0.0f);
	const glm::vec3 reflectionRefractionColor =
		mat.IsReflective() ? GetreflectionRefractionColor(r, depth, refractionIndex, mat, p, rng) * mat.GetSpecular()
						   : glm::vec3(0.0f);

	return diffuseColor + reflectionRefractionColor;
}

glm::vec3 RayTracer::GetDiffuseSpecularColor(const Ray &r, const Material &mat, const glm::vec3 &hitPoint,
											 RandomGenerator &rng) const
{
	if (m_LightCount < 0)
		return {};

	glm::vec3 ret = ambientColor * mat.GetDiffuseColor(r.obj, hitPoint);

	unsigned int l1 = (unsigned int)(rng.Rand() * m_LightCount);
	unsigned int l2 = (unsigned int)(rng.Rand() * m_LightCount);
	unsigned int l3 = (unsigned int)(rng.Rand() * m_LightCount);

	const prims::SceneObject *lights[3] = {m_Scene->GetLights().at(l1), m_Scene->GetLights().at(l2),
										   m_Scene->GetLights().at(l3)};

	for (auto *light : lights)
	{
		glm::vec3 lightContrib = glm::vec3(0.0f);
		const auto p = r.GetHitpoint();
		const auto towardsLight = light->centroid - p;
		const float towardsLightDot = glm::dot(towardsLight, towardsLight);
		const float distToLight = sqrtf(towardsLightDot);
		const vec3 towardsLightNorm = towardsLight / distToLight;

		auto normal = r.normal;
		const float dotRayNormal = glm::dot(r.direction, normal);
		if (dotRayNormal > 0.0f)
			normal = -normal;

		const auto NdotL = glm::dot(normal, towardsLightNorm);
		if (NdotL <= 0.0f)
			continue;

		Ray shadowRay = {p + EPSILON * towardsLightNorm, towardsLightNorm};
		if (!m_Scene->TraceShadowRay(shadowRay, distToLight - 2.0f * EPSILON))
		{
			glm::vec3 lNormal;
			const glm::vec3 pointOnLight = light->GetRandomPointOnSurface(towardsLightNorm, lNormal, rng);
			const float LNdotL = glm::dot(lNormal, -towardsLightNorm);
			if (LNdotL > 0.0f)
			{
				const auto mat = MaterialManager::GetInstance()->GetMaterial(r.obj->materialIdx);
				const auto lightMat = MaterialManager::GetInstance()->GetMaterial(light->materialIdx);
				const float SolidAngle = LNdotL * (light->m_Area / towardsLightDot);

				ret = mat.GetAlbedoColor(r.obj, p) * lightMat.GetEmission() * SolidAngle * NdotL * float(m_LightCount);
			}
		}
	}

	return ret;
}

glm::vec3 RayTracer::GetreflectionRefractionColor(const core::Ray &r, uint &depth, float refractionIndex,
												  const Material &mat, const glm::vec3 &hitPoint, RandomGenerator &rng)
{
	depth++;
	const vec3 normal = r.obj->GetNormal(hitPoint);

	const bool flipNormal = dot(r.direction, normal) > 0;
	core::Ray reflectionRay = r.Reflect(hitPoint, flipNormal ? -normal : normal);
	const glm::vec3 reflectiveColor = Trace(reflectionRay, depth, refractionIndex, rng);

	glm::vec3 refractionCol = glm::vec3(0.f, 0.f, 0.f);
	float FractionReflection = 1.f;
	float FractionTransmission = 0.f;

	if (mat.IsTransparent())
	{
		float n1, n2;
		bool DoAbsorption = false;
		if (flipNormal) // positive means we hit it from the inside
		{
			n1 = mat.refractionIndex;
			n2 = refractionIndex;
		}
		else
		{
			n1 = refractionIndex;
			n2 = mat.refractionIndex;
			DoAbsorption = true;
		}
		float n = n1 / n2;
		float cosTheta = dot(normal, -r.direction);
		float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);
		if (k >= 0.f)
		{
			float R0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));
			float minCosTheta = 1.f - cosTheta;
			FractionReflection =
				R0 + (1.f - R0) * (minCosTheta * minCosTheta * minCosTheta * minCosTheta * minCosTheta);
			FractionTransmission = 1.f - FractionReflection;
			vec3 newDir = n * r.direction + normal * (n * cosTheta - sqrtf(k));
			auto refractionRay = Ray(hitPoint + EPSILON * newDir, newDir);
			refractionCol = Trace(refractionRay, depth, n2, rng);
			if (DoAbsorption)
			{
				refractionCol.r *= exp(-mat.absorption.r * r.t);
				refractionCol.g *= exp(-mat.absorption.g * r.t);
				refractionCol.b *= exp(-mat.absorption.b * r.t);
			}
		}
	}
	return reflectiveColor * FractionReflection + refractionCol * FractionTransmission;
}

void RayTracer::Resize(gl::Texture *newOutput) {}

RayTracer::~RayTracer()
{
	delete tPool;
	delete tResults;
	delete m_Rngs;
	delete[] m_Pixels;
}
} // namespace core