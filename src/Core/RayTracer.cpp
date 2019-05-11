#include "RayTracer.h"
#include "Materials/MaterialManager.h"
#include "Utils/MersenneTwister.h"
#include "Utils/Xor128.h"

using namespace gl;

namespace core
{

RayTracer::RayTracer(prims::WorldScene *Scene, glm::vec3 backGroundColor, glm::vec3 ambientColor,
					 uint maxRecursionDepth, Camera *camera, int width, int height, Surface *skybox)
	: m_Width(width), m_Height(height), m_SkyBox(skybox)
{
	this->m_Scene = Scene;
	this->backGroundColor = backGroundColor;
	this->ambientColor = ambientColor;
	this->maxRecursionDepth = maxRecursionDepth;
	this->m_Camera = camera;
	this->tPool = new ctpl::ThreadPool(ctpl::nr_of_cores);
	this->m_Rngs = new std::vector<RandomGenerator *>();
	this->m_LightCount = static_cast<int>(m_Scene->GetLights().size());
	this->m_Pixels.resize(m_Width * m_Height);

	for (int i = 0; i < tPool->size(); i++)
		this->m_Rngs->push_back(new Xor128());
}

void RayTracer::Render(Surface *output)
{
	m_Width = output->getWidth();
	m_Height = output->getHeight();

	int nr_of_threads = tPool->size();
	for (int i = 0; i < nr_of_threads; i++)
	{
		RandomGenerator *rngPointer = m_Rngs->at(i);

		tResults.push_back(tPool->push([this, output, i, nr_of_threads, rngPointer](int) -> void {
			for (int y = i; y < m_Height; y += nr_of_threads)
			{
				for (int x = 0; x < m_Width; x++)
				{
					uint depth = 0;
					Ray r = m_Camera->generateRandomRay(float(x), float(y), *rngPointer);
					const glm::vec3 color = Trace(r, depth, 1.f, *rngPointer);

					const int pidx = x + y * m_Width;

					if (m_Samples > 0)
						m_Pixels[pidx] = (m_Pixels[pidx] * float(m_Samples) + color) / (float(m_Samples + 1));
					else
						m_Pixels[pidx] = color;
					output->Plot(x, y, m_Pixels[pidx]);
				}
			}
		}));
	}

	for (auto &r : tResults)
		r.get();

	tResults.clear();
	m_Samples++;
}

glm::vec3 RayTracer::Trace(Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng)
{
	m_Scene->TraceRay(r);

	if (!r.IsValid())
	{
		if (!m_SkyBox)
			return backGroundColor;
		const float u =
			min(1.0f, max(0.0f, (1.0f + atan2f(r.direction.x, -r.direction.z) * one_over_pi<float>()) / 2.0f));
		const float v = min(1.0f, max(0.0f, acosf(r.direction.y) * one_over_pi<float>()));
		return m_SkyBox->getColorAt(u, 1.0f - v);
	}

	return Shade(r, depth, refractionIndex, rng);
}

glm::vec3 RayTracer::Shade(const Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng)
{
	if (depth >= maxRecursionDepth)
		return glm::vec3(0.f);

	const vec3 p = r.GetHitpoint();
	const auto mat = MaterialManager::GetInstance()->GetMaterial(r.obj->materialIdx);

	if (mat.type == Light)
		return mat.emission;

	vec3 color;
	if (mat.type == Lambert)
		return GetDiffuseSpecularColor(r, mat, p, rng);
	else
		return GetreflectionRefractionColor(r, depth, refractionIndex, mat, p, rng);
}

glm::vec3 RayTracer::GetDiffuseSpecularColor(const Ray &r, const Material &mat, const glm::vec3 &hitPoint,
											 RandomGenerator &rng) const
{
	if (m_LightCount < 0)
		return {};

	glm::vec3 color = mat.albedo;
	if (mat.textureIdx >= 0)
		color = MaterialManager::GetInstance()->GetTexture(mat.textureIdx).getColorAt(r.obj->GetTexCoords(r.GetHitpoint()));

	glm::vec3 ret = ambientColor * color;

	const auto l1 = (unsigned int)(rng.Rand() * (m_LightCount - 1));
	const auto l2 = (unsigned int)(rng.Rand() * (m_LightCount - 1));

	const prims::SceneObject *lights[2] = {m_Scene->GetLights().at(l1), m_Scene->GetLights().at(l2)};

	for (const auto &light : lights)
	{
		const auto p = r.GetHitpoint();

		vec3 lNormal;
		const vec3 lightPos = light->GetRandomPointOnSurface(glm::normalize(light->centroid - p), lNormal, rng);
		const vec3 towardsLight = lightPos - p;
		const float towardsLightDot = glm::dot(towardsLight, towardsLight);
		const float distToLight = sqrtf(towardsLightDot);
		const vec3 towardsLightNorm = towardsLight / distToLight;

		auto normal = r.normal;
		const float dotRayNormal = glm::dot(r.direction, normal);
		if (dotRayNormal >= 0.0f)
			normal = -normal;

		const auto NdotL = glm::dot(normal, towardsLightNorm);
		if (NdotL < 0.0f)
			continue;

		Ray shadowRay = {p + EPSILON * towardsLightNorm, towardsLightNorm};
		if (!m_Scene->TraceShadowRay(shadowRay, distToLight - 2.0f * EPSILON))
		{
			const float LNdotL = glm::dot(lNormal, -towardsLightNorm);
			if (LNdotL > 0.0f)
			{
				const auto &lightMat = MaterialManager::GetInstance()->GetMaterial(light->materialIdx);
				const float SolidAngle = LNdotL * light->m_Area / towardsLightDot;

				ret = color * lightMat.emission * SolidAngle * NdotL * float(m_LightCount);
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

	if (mat.type == Fresnel)
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

void RayTracer::Resize(int width, int height, gl::Texture *newOutput1, gl::Texture *)
{
	m_Width = width;
	m_Height = height;
	m_Camera->setDimensions({m_Width, m_Height});

	m_Pixels.clear();
	m_Pixels.resize(m_Width * m_Height);
	Reset();

	for (auto *rng : *m_Rngs)
		delete rng;
	m_Rngs->clear();
	for (int i = 0; i < tPool->size(); i++)
		m_Rngs->push_back(new Xor128());
}

RayTracer::~RayTracer()
{
	delete tPool;
	for (auto *rng : *m_Rngs)
		delete rng;
	delete m_Rngs;
}
} // namespace core