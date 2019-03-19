#pragma once

#include <vector>

#include "Camera.h"
#include "Primitives/SceneObjectList.h"
#include "Renderer.h"
#include "Utils/ctpl.h"

namespace core
{

class RayTracer : public Renderer
{
  public:
	RayTracer() = default;
	~RayTracer();
	void Render(Surface *output) override;

	RayTracer(prims::WorldScene *Scene, glm::vec3 backgroundColor, glm::vec3 ambientColor, uint maxRecursionDepth,
			  Camera *camera, int width, int height);
	glm::vec3 Trace(Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng);
	glm::vec3 Shade(const Ray &r, uint &depth, float refractionIndex, RandomGenerator &rng);
	glm::vec3 GetDiffuseSpecularColor(const Ray &r, const Material &mat, const glm::vec3 &hitPoint,
									  RandomGenerator &rng) const;
	glm::vec3 GetreflectionRefractionColor(const Ray &r, uint &depth, float refractionIndex,
										   const Material &mat, const glm::vec3 &hitPoint,
										   RandomGenerator &rng);

	inline void SwitchSkybox() override {}

	void Resize(gl::Texture *newOutput) override;

	inline void Reset() override { m_Samples = 0; }

  private:
	glm::vec3 backGroundColor, ambientColor;
	unsigned int maxRecursionDepth;
	prims::WorldScene *m_Scene;
	ctpl::ThreadPool *tPool = nullptr;
	Camera *m_Camera;

	int m_Tiles, m_Width, m_Height, m_LightCount;

	std::vector<std::future<void>> *tResults = nullptr;
	std::vector<RandomGenerator *> *m_Rngs = nullptr;

	glm::vec3 *m_Pixels;

	int m_Samples = 0;
};
} // namespace core