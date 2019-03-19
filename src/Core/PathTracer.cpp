#include "PathTracer.h"
#include "Utils/MersenneTwister.h"
#include "Utils/Xor128.h"

#include <glm/gtc/constants.hpp>

namespace core
{

#define SAMPLE_COUNT 1
#define RUSSIAN_ROULETTE 1
#define LOOP_DEPTH 16
#define FIREFLYFILTER 1

#define TILE_HEIGHT 32
#define TILE_WIDTH 32

using namespace prims;

PathTracer::PathTracer(WorldScene *scene, int width, int height, Camera *camera, Surface *skyBox)
	: m_Scene(scene), m_Width(width), m_Height(height), m_SkyBox(skyBox), m_Camera(camera),
	  m_SkyboxEnabled(skyBox != nullptr)
{
	m_Pixels = new glm::vec3[m_Width * m_Height];
	m_Energy = new float[m_Width * m_Height];

	Reset();
	m_Tiles = (m_Width / TILE_WIDTH) * (m_Height / TILE_HEIGHT);

	auto &lights = m_Scene->GetLights();
	m_LightArea = 0.0f;
	m_LightCount = static_cast<unsigned int>(lights.size());
	if (!lights.empty())
	{
		for (SceneObject *light : lights)
		{
			m_LightArea += light->GetArea();
		}

		m_LightLotteryTickets.resize(lights.size());
		m_LightLotteryTickets[0] = lights[0]->GetArea() / m_LightArea;
		for (unsigned int i = 1; i < lights.size(); i++)
		{
			m_LightLotteryTickets[i] = m_LightLotteryTickets[i - 1] + lights[i]->GetArea() / m_LightArea;
		}
		m_LightLotteryTickets[lights.size() - 1] = 1.f;
	}

	this->tPool = new ctpl::ThreadPool(ctpl::nr_of_cores);

	for (int i = 0; i < m_Tiles; i++)
	{
		m_Rngs.push_back(new Xor128());
	}

	m_Samples = 0;
	m_Materials = MaterialManager::GetInstance();
}

PathTracer::~PathTracer()
{
	delete tPool;

	delete[] m_Pixels;
	delete[] m_Energy;
}

void PathTracer::Reset()
{
	memset(m_Pixels, 0, m_Width * m_Height * 4);
	memset(m_Energy, 0, m_Width * m_Height * 4);
	m_Samples = 0;
}

int PathTracer::GetSamples() const { return m_Samples; }

float PathTracer::GetEnergy() const
{
	const float oneThird = 1.f / 3.f;
	float totalEnergy = 0.0f;
	for (int i = 0; i < m_Width * m_Height; i++)
	{
		totalEnergy += m_Energy[i] * oneThird;
	}
	return totalEnergy;
}

float PathTracer::TotalEnergy() const
{
	const float oneThird = 1.f / 3.f;
	float totalEnergy = 0;
	for (int y = 0; y < m_Height; y++)
	{
		for (int x = 0; x < m_Width; x++)
		{
			const glm::vec3 tmp = (m_Pixels[x + y * m_Width] * oneThird);
			const float E = tmp.r + tmp.g + tmp.b;

			totalEnergy += E;
		}
	}
	return totalEnergy;
}

void PathTracer::Render(Surface *output)
{
	const float EFactor = 1.0f / float(SAMPLE_COUNT);

	m_Height = output->GetHeight();
	m_Width = output->GetWidth();

	const int vTiles = m_Height / TILE_HEIGHT;
	const int hTiles = m_Width / TILE_WIDTH;

	unsigned int idx = 0;

	for (int tile_y = 0; tile_y < vTiles; tile_y++)
	{
		for (int tile_x = 0; tile_x < hTiles; tile_x++)
		{
			RandomGenerator *rngPointer = m_Rngs.at(idx);
			idx++;
			tResults.push_back(tPool->push([tile_x, tile_y, this, output, rngPointer, EFactor](int) -> void {
				for (int y = 0; y < TILE_HEIGHT; y++)
				{
					const int pixel_y = y + tile_y * TILE_HEIGHT;
					for (int x = 0; x < TILE_WIDTH; x++)
					{
						const int pixel_x = x + tile_x * TILE_WIDTH;

						uint depth = 0;
						Ray r = m_Camera->GenerateRandomRay(float(pixel_x), float(pixel_y), *rngPointer);
						auto color = Trace(r, depth, 1.f, *rngPointer) * EFactor;

						const int idx = pixel_x + pixel_y * m_Width;

						if (m_Samples > 0)
						{
							const float factor = 1.0f / float(m_Samples + 1);
							m_Pixels[idx] = m_Pixels[idx] * float(m_Samples) * factor + color * factor;
						}
						else
						{
							m_Pixels[idx] = color;
						}

						m_Energy[idx] = color.x + color.y + color.z;
						output->Plot(pixel_x, pixel_y, m_Pixels[idx]);
					}
				}
			}));
		}
	}

	for (auto &r : tResults)
		r.get();

	tResults.clear();
	m_Samples++;
}

glm::vec3 PathTracer::Trace(Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng)
{
	glm::vec3 E = glm::vec3(0.0f);
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		glm::vec3 newSample;
		switch (m_Mode)
		{
		case (Mode::NEE):
			newSample = SampleNEE(r, rng);
			break;
		case (Mode::IS):
			newSample = SampleIS(r, rng);
			break;
		case (Mode::NEE_IS):
			newSample = SampleNEE_IS(r, rng);
			break;
		case (Mode::NEE_MIS):
			newSample = SampleNEE_MIS(r, rng);
			break;
		case (Mode::ReferenceMicrofacet):
			newSample = SampleReferenceMicrofacet(r, rng);
			break;
		case (Mode::NEEMicrofacet):
			newSample = SampleNEEMicrofacet(r, rng);
			break;
		case (Mode::Reference):
		default:
			newSample = SampleReference(r, rng);
			break;
		}

		E += newSample;
	}

#if FIREFLYFILTER
	const float lengthSqr = dot(E, E);
	if (lengthSqr > 100.0f) // length > 10
		return E / sqrtf(lengthSqr) * 10.0f;
#endif

	return E;
}

glm::vec3 PathTracer::SampleNEE(Ray &r, RandomGenerator &rng) const
{
	vec3 E = vec3(0.0f);
	vec3 throughput = vec3(1.0f);
	bool specular = true;

	const float PDF = 1.0f / (2.0f * PI);
	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			// Sample skybox
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const glm::vec3 p = r.GetHitpoint();
		const auto mat = m_Materials->GetMaterial(r.obj->materialIdx);

		if (mat.IsLight())
		{
			if (specular)
			{
				E += throughput * mat.GetEmission();
			}
			break;
		}

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		const bool isDiffuse = mat.IsDiffuse();
		const bool isReflective = mat.IsReflective();
		const bool isTransparent = mat.IsTransparent();
		specular = true;

		if (isTransparent)
		{
			throughput *= albedoColor * Refract(flipNormal, mat, normal, p, r.t, r, rng);
			continue;
		}

		if (isDiffuse && isReflective)
		{
			if (rng.Rand(1.0f) < mat.GetSpecular())
			{
				throughput *= albedoColor;
				r = r.Reflect(p, normal);
				continue;
			}
		}
		else if (isReflective)
		{
			throughput = throughput * albedoColor;
			r = r.Reflect(p, normal);
			continue;
		}

		r = r.DiffuseReflection(p, normal, rng);
		const vec3 BRDF = albedoColor / glm::pi<float>();

		// NEE
		float NEEpdf;
		SceneObject *light = RandomPointOnLight(NEEpdf, rng);
		if (light != nullptr)
		{ // if there are no lights
			specular = false;
			const vec3 Direction = normalize(p - light->GetPosition());
			vec3 lightNormal;
			const vec3 pointOnLight = light->GetRandomPointOnSurface(Direction, lightNormal, rng);

			vec3 L = pointOnLight - p;
			const float squaredDistance = dot(L, L);
			const float distance = sqrtf(squaredDistance);
			L /= distance;

			const float NdotL = dot(normal, L);
			const float LNdotL = dot(lightNormal, -L);

			if (NdotL > 0.f && LNdotL > 0.f)
			{
				Ray lightRay = Ray(p + EPSILON * L, L);
				if (!m_Scene->TraceShadowRay(lightRay, distance - EPSILON))
				{
					const float SolidAngle = LNdotL * (light->m_Area / squaredDistance);
					const auto m = m_Materials->GetMaterial(light->materialIdx);
					const vec3 Ld = BRDF * m.GetEmission() * SolidAngle * NdotL / NEEpdf;

					E += throughput * Ld;
				}
			}
		}
		// NEE

		throughput *= BRDF * dot(normal, r.direction) / PDF;
#if RUSSIAN_ROULETTE
		if (RussianRoulette(throughput, depth, rng))
		{
			break;
		}
#endif
	}

	return E;
}

glm::vec3 PathTracer::SampleIS(Ray &r, RandomGenerator &rng) const
{
	vec3 E = vec3(0.0f);
	vec3 throughput = vec3(1.0f);

	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			// sample skybox
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const auto mat = m_Materials->GetMaterial(r.obj->materialIdx);

		if (mat.IsLight())
		{
			E += throughput * mat.GetEmission();
			break;
		}

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		const bool isDiffuse = mat.IsDiffuse();
		const bool isReflective = mat.IsReflective();
		const bool isTransparent = mat.IsTransparent();

		if (isTransparent)
		{
			throughput *= albedoColor * Refract(flipNormal, mat, normal, p, r.t, r, rng);
			continue;
		}

		if (isDiffuse && isReflective)
		{
			if (rng.Rand(1.0f) < mat.GetSpecular())
			{
				throughput *= albedoColor;
				r = r.Reflect(p, normal);
				continue;
			}
		}
		else if (isReflective)
		{
			throughput *= albedoColor;
			r = r.Reflect(p, normal);
			continue;
		}

		r = r.CosineWeightedDiffuseReflection(p, normal, rng);
		const float PDF = dot(normal, r.direction) * glm::one_over_pi<float>();
		const glm::vec3 BRDF = albedoColor * glm::one_over_pi<float>();
		throughput *= BRDF * dot(normal, r.direction) / PDF;

#if RUSSIAN_ROULETTE
		if (RussianRoulette(throughput, depth, rng))
		{
			break;
		}
#endif
	}

	return E;
}

glm::vec3 PathTracer::SampleNEE_IS(Ray &r, RandomGenerator &rng) const
{
	vec3 E = vec3(0.0f);
	vec3 throughput = vec3(1.0f);
	bool specular = true;

	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			// sample skybox
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const auto mat = m_Materials->GetMaterial(r.obj->materialIdx);

		if (mat.IsLight())
		{
			if (specular)
			{
				E += throughput * mat.GetEmission();
			}
			break;
		}

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		const bool isDiffuse = mat.IsDiffuse();
		const bool isReflective = mat.IsReflective();
		const bool isTransparent = mat.IsTransparent();
		specular = true;

		if (isTransparent)
		{
			throughput *= albedoColor * Refract(flipNormal, mat, normal, p, r.t, r, rng);
			continue;
		}

		if (isDiffuse && isReflective)
		{
			if (rng.Rand(1.0f) < mat.GetSpecular())
			{
				throughput *= albedoColor;
				r = r.Reflect(p, normal);
				continue;
			}
		}
		else if (isReflective)
		{
			throughput *= albedoColor;
			r = r.Reflect(p, normal);
			continue;
		}

		r = r.CosineWeightedDiffuseReflection(p, normal, rng);
		const float PDF = dot(normal, r.direction) * glm::one_over_pi<float>();
		const glm::vec3 BRDF = albedoColor * glm::one_over_pi<float>();

		// NEE
		float NEEpdf;
		SceneObject *light = RandomPointOnLight(NEEpdf, rng);
		if (light != nullptr)
		{ // if there are no lights
			specular = false;
			const vec3 Direction = normalize(p - light->GetPosition());
			vec3 lightNormal;
			const vec3 pointOnLight = light->GetRandomPointOnSurface(Direction, lightNormal, rng);

			vec3 L = pointOnLight - p;
			const float squaredDistance = dot(L, L);
			const float lDistance = sqrtf(squaredDistance);
			L /= lDistance;

			const float NdotL = dot(normal, L);
			const float LNdotL = dot(lightNormal, -L);

			if (NdotL > 0.f && LNdotL > 0.f)
			{
				Ray lightRay = Ray(p + EPSILON * L, L);
				if (!m_Scene->TraceShadowRay(lightRay, lDistance - EPSILON))
				{
					const float SolidAngle = LNdotL * light->m_Area / squaredDistance;
					const auto m = m_Materials->GetMaterial(light->materialIdx);
					const vec3 Ld = BRDF * m.GetEmission() * SolidAngle * NdotL / NEEpdf;

					E += throughput * Ld;
				}
			}
		}
		// NEE

		throughput *= BRDF * dot(normal, r.direction) / PDF;
#if RUSSIAN_ROULETTE
		if (RussianRoulette(throughput, depth, rng))
		{
			break;
		}
#endif
	}

	return E;
}

glm::vec3 PathTracer::SampleNEE_MIS(Ray &r, RandomGenerator &rng) const
{
	vec3 E = vec3(0.0f);
	vec3 throughput = vec3(1.0f);
	bool specular = true;
	vec3 BRDF{}, normal{}, tUpdate = vec3(1.0f);
	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			// Sample skybox
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c * tUpdate;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const auto mat = m_Materials->GetMaterial(r.obj->materialIdx);

		if (mat.IsLight())
		{
			if (depth <= 0)
			{
				E += throughput * mat.GetEmission();
			}
			else if (specular)
			{
				E += throughput * mat.GetEmission() * tUpdate;
			}
			else
			{
				const auto *light = r.obj;
				const vec3 &L = r.direction;
				const float squaredDistance = r.t * r.t;

				vec3 lightNormal = r.normal;
				if (dot(lightNormal, L) > 0.0f)
					lightNormal *= -1.0f;

				const float NdotL = dot(normal, L);
				const float LNdotL = dot(lightNormal, -L);
				const float InversePDFnee = m_LightArea / light->m_Area;
				const float SolidAngle = LNdotL * light->m_Area / squaredDistance;
				const float lightPDF = 1.0f / SolidAngle;
				const float brdfPDF = NdotL / PI;
				const vec3 Ld = BRDF * mat.GetEmission() * NdotL * InversePDFnee;

				if (lightPDF > 0.0f && brdfPDF > 0.0f)
				{
					const float w1 = brdfPDF / (brdfPDF + lightPDF);
					const float w2 = lightPDF / (brdfPDF + lightPDF);
					E += throughput * Ld / (w1 * brdfPDF + w2 * lightPDF);
				}
			}
			break;
		}

		throughput *= tUpdate;
		tUpdate = vec3(1.0f);

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		BRDF = albedoColor * glm::one_over_pi<float>();

		const bool isDiffuse = mat.IsDiffuse();
		const bool isReflective = mat.IsReflective();
		const bool isTransparent = mat.IsTransparent();

		if (isTransparent)
		{
			tUpdate *= albedoColor * Refract(flipNormal, mat, normal, p, r.t, r, rng);
			specular = true;
			continue;
		}

		if (isDiffuse && isReflective)
		{
			if (rng.Rand(1.0f) < mat.GetSpecular())
			{
				tUpdate *= albedoColor;
				r = r.Reflect(p, normal);
				specular = true;
				continue;
			}
		}
		else if (isReflective)
		{
			tUpdate *= albedoColor;
			r = r.Reflect(p, normal);
			specular = true;
			continue;
		}

		// NEE
		specular = false;
		float NEEpdf;
		SceneObject *light = RandomPointOnLight(NEEpdf, rng);
		if (light != nullptr)
		{ // if there are no lights
			const vec3 Direction = normalize(p - light->GetPosition());
			vec3 lightNormal;
			const vec3 pointOnLight = light->GetRandomPointOnSurface(Direction, lightNormal, rng);

			vec3 L = pointOnLight - p;
			const float squaredDistance = dot(L, L);
			const float distance = sqrtf(squaredDistance);
			L /= distance;

			const float NdotL = dot(normal, L);
			const float LNdotL = dot(lightNormal, -L);

			if (NdotL > 0.f && LNdotL > 0.f)
			{
				Ray lightRay = Ray(p + EPSILON * L, L);
				if (!m_Scene->TraceShadowRay(lightRay, distance - EPSILON))
				{
					const float SolidAngle = LNdotL * light->m_Area / squaredDistance;
					const auto m = m_Materials->GetMaterial(light->materialIdx);
					const vec3 Ld = BRDF * m.GetEmission() * NdotL / NEEpdf;

					const float bPDF = NdotL / PI;
					const float lightPDF = 1.0f / SolidAngle;
					if (lightPDF > 0.0f && bPDF > 0.0f)
					{
						const float w1 = bPDF / (bPDF + lightPDF);
						const float w2 = lightPDF / (bPDF + lightPDF);
						E += throughput * Ld / (w1 * bPDF + w2 * lightPDF);
					}
				}
			}
		}
		// NEE

		r = r.CosineWeightedDiffuseReflection(p, normal, rng);
		const float NdotR = dot(normal, r.direction);
		const float PDF = NdotR / PI;
		if (PDF <= 0.0f)
			break;
		tUpdate *= BRDF * NdotR / PDF;
#if RUSSIAN_ROULETTE
		if (RussianRoulette(tUpdate, depth, rng))
		{
			break;
		}
#endif
	}
	return E;
}

glm::vec3 PathTracer::SampleSkyBox(const glm::vec3 &dir) const
{
	if (!this->m_SkyboxEnabled)
	{
		return vec3(0.0f);
	}
	const float u = glm::min(1.0f, glm::max(0.0f, (1.0f + atan2f(dir.x, -dir.z) / PI) / 2.0f));
	const float v = glm::min(1.0f, glm::max(0.0f, acosf(dir.y) / PI));
	return this->m_SkyBox->GetColorAt(u, 1.0f - v);
}

glm::vec3 PathTracer::SampleReference(Ray &r, RandomGenerator &rng) const
{
	vec3 E = vec3(0.0f);
	vec3 throughput = vec3(1.0f);
	const float PDF = 1.0f / (2.0f * PI); // constant PDF

	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			// sample skybox
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const auto mat = m_Materials->GetMaterial(r.obj->materialIdx);

		if (mat.IsLight())
		{
			E += throughput * mat.GetEmission();
			break;
		}

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		const bool isTransparent = mat.IsTransparent();

		if (isTransparent)
		{
			throughput *= albedoColor * Refract(flipNormal, mat, normal, p, r.t, r, rng);
			continue;
		}

		const bool isDiffuse = mat.IsDiffuse();
		const bool isReflective = mat.IsReflective();

		if (isDiffuse && isReflective)
		{
			if (rng.Rand(1.0f) < mat.GetSpecular())
			{
				throughput *= albedoColor;
				r = r.Reflect(p, normal);
				continue;
			}
		}
		else if (isReflective)
		{
			throughput *= albedoColor;
			r = r.Reflect(p, normal);
			continue;
		}

		r = r.DiffuseReflection(p, normal, rng);
		const glm::vec3 BRDF = albedoColor * glm::one_over_pi<float>();
		throughput *= BRDF * dot(normal, r.direction) / PDF;
	}

	return E;
}

inline glm::vec3 PathTracer::Refract(const bool &flipNormal, const Material &mat, const glm::vec3 &normal,
									 const glm::vec3 &p, const float &t, Ray &r, RandomGenerator &rng) const
{
	float n1, n2;
	glm::vec3 throughputUpdate = vec3(1.0f);
	if (flipNormal)
		n1 = mat.refractionIndex, n2 = 1.0f;
	else
		n1 = 1.0f, n2 = mat.refractionIndex;

	const float n = n1 / n2;
	const float cosTheta = dot(normal, -r.direction);
	const float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);
	if (k > 0.0f)
	{
		const float a = n1 - n2;
		const float b = n1 + n2;
		const float R0 = (a * a) / (b * b);
		const float c = 1.0f - cosTheta;
		const float Fr = R0 + (1.0f - R0) * (c * c * c * c * c);

		if (rng.Rand(1.0f) > Fr)
		{
			if (!flipNormal)
			{
				throughputUpdate =
					glm::vec3(exp(-mat.absorption.r * t), exp(-mat.absorption.g * t), exp(-mat.absorption.b * t));
			}

			const vec3 dir = normalize(n * r.direction + normal * (n * cosTheta - sqrtf(k)));
			r = Ray(p + EPSILON * dir, dir);
			return throughputUpdate;
		}
	}

	r = r.Reflect(p, normal);
	return throughputUpdate;
}

bool PathTracer::TraceLightRay(Ray &r, SceneObject *light) const
{
	m_Scene->TraceRay(r);
	return !r.IsValid() || r.obj == light;
}

SceneObject *PathTracer::RandomPointOnLight(float &NEEpdf, RandomGenerator &rng) const
{
	const std::vector<SceneObject *> &lights = m_Scene->GetLights();
	if (!m_LightCount)
	{
		return nullptr;
	}

	const float randomf = rng.Rand(1.f);
	int winningIdx = 0;
	float previous = 0;

	for (uint i = 0; i < m_LightCount; i++) // get light corresponding with winning ticket
	{
		if (m_LightLotteryTickets[i] > previous && randomf <= m_LightLotteryTickets[i])
		{
			winningIdx = i;
			break;
		}
		previous = m_LightLotteryTickets[i];
	}

	NEEpdf = lights[winningIdx]->m_Area / m_LightArea;
	return lights.at(winningIdx);
}

void PathTracer::Resize(gl::Texture *newOutput)
{
	m_Width = newOutput->GetWidth();
	m_Height = newOutput->GetHeight();

	delete[] m_Pixels;
	delete[] m_Energy;

	m_Pixels = new glm::vec3[m_Width * m_Height];
	m_Energy = new float[m_Width * m_Height];
	Reset();

	m_Tiles = (m_Width / TILE_WIDTH) * (m_Height / TILE_HEIGHT);

	m_Rngs.clear();
	for (int i = 0; i < m_Tiles; i++)
	{
		m_Rngs.push_back(new MersenneTwister());
	}
}

glm::vec3 PathTracer::SampleReferenceMicrofacet(Ray &r, RandomGenerator &rng) const
{
	glm::vec3 E = vec3(0.0f);
	glm::vec3 throughput = vec3(1.0f);
	glm::vec3 u{}, v{}, w{};							   // For the transformation from world to local and back
	glm::vec3 wiLocal{}, wmLocal{}, wm{}, woLocal{}, wo{}; // The in and outgoing path vectors

	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const uint matIdx = r.obj->materialIdx;
		const auto mat = m_Materials->GetMaterial(matIdx);
		const auto mf = m_Materials->GetMicrofacet(matIdx);

		if (mat.IsLight())
		{
			E += throughput * mat.GetEmission();
			break;
		}

		const glm::vec3 albedoColor = mat.GetAlbedoColor(r.obj, p);
		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		wiLocal = rng.worldToLocalMicro(normal, r.direction, u, v, w);
		wmLocal = mf.sampleWm(rng);
		wm = rng.localToWorldMicro(wmLocal, u, v, w);

		if (mat.IsTransparent())
		{
			float n1, n2;
			glm::vec3 absorption = vec3(1.0f);
			bool doAbsorption = false;
			if (flipNormal)
			{
				n1 = mat.refractionIndex;
				n2 = 1.0f;
			}
			else
			{
				n1 = 1.0f;
				n2 = mat.refractionIndex;
				doAbsorption = true;
			}

			const float n = n1 / n2;
			const float cosTheta = dot(wm, -r.direction);
			const float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);

			if (k > 0.0f)
			{
				const float a = n1 - n2;
				const float b = n1 + n2;
				const float R0 = (a * a) / (b * b);
				const float c = 1.0f - cosTheta;
				const float Fr = R0 + (1.0f - R0) * (c * c * c * c * c);

				if (rng.Rand(1.0f) > Fr)
				{
					if (doAbsorption)
					{
						absorption = glm::vec3(exp(-mat.absorption.r * r.t), exp(-mat.absorption.g * r.t),
											   exp(-mat.absorption.b * r.t));
					}

					throughput *= absorption;
					woLocal = glm::normalize(n * -wiLocal + wmLocal * (n * cosTheta - sqrtf(k)));
				}
				else
				{
					woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal;
				}
			}
			else
			{
				woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal;
			}

			const float weight = mf.weight(woLocal, wiLocal, wmLocal);
			wo = rng.localToWorldMicro(woLocal, u, v, w);
			throughput *= albedoColor * weight;
			r = Ray(p + EPSILON * wo, wo);
		}
		else
		{
			woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal; // mirror wi in wm to get wo
			const float weight = mf.weight(woLocal, wiLocal, wmLocal);
			wo = rng.localToWorldMicro(woLocal, u, v, w);
			throughput *= albedoColor * weight;
			r = Ray(p + EPSILON * wo, wo);
		}
	}

	return E;
}

glm::vec3 PathTracer::SampleNEEMicrofacet(Ray &r, RandomGenerator &rng) const
{
	glm::vec3 E = vec3(0.0f);
	glm::vec3 throughput = vec3(1.0f);
	glm::vec3 u{}, v{}, w{};									 // For the transformation from world to local and back
	glm::vec3 wiLocal{}, wmLocal{}, wm{}, woLocal{}, wo{}, wi{}; // The in and outgoing path vectors

	for (uint depth = 0; depth < LOOP_DEPTH; depth++)
	{
		m_Scene->TraceRay(r);
		if (!r.IsValid())
		{
			if (m_SkyBox != nullptr)
			{
				const vec3 c = this->SampleSkyBox(r.direction);
				E += throughput * c;
			}
			break;
		}

		const vec3 p = r.GetHitpoint();
		const uint matIdx = r.obj->materialIdx;
		const auto mat = m_Materials->GetMaterial(matIdx);
		const auto mf = m_Materials->GetMicrofacet(matIdx);

		if (mat.IsLight())
		{
			E += throughput * mat.GetEmission();
			break;
		}

		vec3 normal = r.normal;
		const bool flipNormal = dot(normal, r.direction) > 0.0f;
		if (flipNormal)
			normal *= -1.0f;

		wiLocal = rng.worldToLocalMicro(normal, r.direction, u, v, w);
		wmLocal = mf.sampleWm(rng);
		wm = rng.localToWorldMicro(wmLocal, u, v, w);
		wi = -r.direction;

		if (mat.IsTransparent())
		{
			float n1, n2;
			glm::vec3 absorption = vec3(1.0f);
			bool doAbsorption = false;
			if (flipNormal)
			{
				n1 = mat.refractionIndex;
				n2 = 1.0f;
			}
			else
			{
				n1 = 1.0f;
				n2 = mat.refractionIndex;
				doAbsorption = true;
			}

			const float n = n1 / n2;
			const float cosTheta = dot(wm, -r.direction);
			const float k = 1.0f - (n * n) * (1.0f - cosTheta * cosTheta);

			if (k > 0.0f)
			{
				const float a = n1 - n2;
				const float b = n1 + n2;
				const float R0 = (a * a) / (b * b);
				const float c = 1.0f - cosTheta;
				const float Fr = R0 + (1.0f - R0) * (c * c * c * c * c);

				if (rng.Rand(1.0f) > Fr)
				{
					if (doAbsorption)
					{
						absorption = {exp(-mat.absorption.r * r.t), exp(-mat.absorption.g * r.t),
									  exp(-mat.absorption.b * r.t)};
					}

					throughput *= absorption;
					woLocal = glm::normalize(n * -wiLocal + wmLocal * (n * cosTheta - sqrtf(k)));
				}
				else
				{
					woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal;
				}
			}
			else
			{
				woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal;
			}

			wo = rng.localToWorldMicro(woLocal, u, v, w);
			const float weight = mf.weight(woLocal, wiLocal, wmLocal);

			throughput *= mat.GetAlbedoColor(r.obj, p) * weight;
			r = Ray(p + EPSILON * wo, wo);
		}
		else
		{
			woLocal = wmLocal * 2.0f * glm::dot(wmLocal, wiLocal) - wiLocal;
			const float weight = mf.weight(woLocal, wiLocal, wmLocal);
			wo = rng.localToWorldMicro(woLocal, u, v, w);

			// NEE
			float NEEpdf;
			SceneObject *light = RandomPointOnLight(NEEpdf, rng);
			if (light != nullptr)
			{ // if there are no lights
				const vec3 Direction = glm::normalize(p - light->GetPosition());
				vec3 lightNormal;
				const vec3 pointOnLight = light->GetRandomPointOnSurface(Direction, lightNormal, rng);

				vec3 L = pointOnLight - p;
				const float squaredDistance = dot(L, L);
				const float lDistance = sqrtf(squaredDistance);
				L /= lDistance;

				const float NdotL = dot(wm, L);
				const float LNdotL = dot(lightNormal, -L);

				if (NdotL > 0.f && LNdotL > 0.f)
				{
					Ray lightRay = Ray(p + EPSILON * L, L);
					if (!m_Scene->TraceShadowRay(r, lDistance))
					{
						const float SolidAngle = LNdotL * light->m_Area / squaredDistance;
						const auto lightMat = m_Materials->GetMaterial(light->materialIdx);
						const float neeWeight = 1.0f - mf.weight(L, wi, wm);
						if (neeWeight > 0.0f)
						{
							const vec3 Ld = mat.GetAlbedoColor(r.obj, p) * weight * lightMat.GetEmission() *
											SolidAngle * NdotL / NEEpdf;
							E += throughput * Ld * neeWeight;
						}
					}
				}
			}
			// NEE

			throughput *= mat.GetAlbedoColor(r.obj, p) * weight;
			r = Ray(p + EPSILON * wo, wo);
		}
	}

	return E;
}
} // namespace core