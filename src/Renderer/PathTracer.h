#pragma once

#include "Camera.h"
#include "Materials/MaterialManager.h"
#include "Primitives/SceneObjectList.h"
#include "Renderer.h"
#include "Utils/ctpl.h"

class PathTracer : public Renderer
{
  public:
    PathTracer() = default;

    ~PathTracer() override;

    explicit PathTracer(WorldScene *scene, int width, int height,
                        Camera *camera, Surface *skyBox = nullptr);

    inline void SetMode(Mode mode) override
    {
        m_Mode = mode;
        Reset();
    }

    void Reset() override;

    int GetSamples() const override;

    void Render(Surface *output) override;

    glm::vec3 Trace(Ray &r, uint &depth, float refractionIndex,
                    RandomGenerator &rng);

    bool TraceLightRay(Ray &r, SceneObject *light)
        const; // returns whether light is obstructed

    glm::vec3 SampleNEE(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleIS(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleReference(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleNEE_IS(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleNEE_MIS(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleSkyBox(const glm::vec3 &dir) const;

    glm::vec3 SampleReferenceMicrofacet(Ray &r, RandomGenerator &rng) const;

    glm::vec3 SampleNEEMicrofacet(Ray &r, RandomGenerator &rng) const;

    glm::vec3 Refract(const bool &flipNormal, const material::Material &mat,
                      const glm::vec3 &normal, const glm::vec3 &p,
                      const float &t, Ray &r, RandomGenerator &rng) const;

    float GetEnergy() const;

    float TotalEnergy() const;

    inline void SwitchSkybox() override
    {
        this->m_SkyboxEnabled =
            (this->m_SkyBox != nullptr && !this->m_SkyboxEnabled);
    }

    SceneObject *RandomPointOnLight(float &NEEpdf, RandomGenerator &rng) const;

    inline bool RussianRoulette(glm::vec3 &throughput, const uint depth,
                                RandomGenerator &rng) const
    {
        if (depth > 3)
        {
            const float probability = std::max(
                EPSILON,
                std::max(throughput.x, std::max(throughput.y, throughput.z)));
            if (rng.Rand(1.0f) > probability || probability <= 0.f)
                return true;
            throughput /= probability;
        }
        return false;
    }

    void Resize(Texture *newOutput) override;

  private:
    WorldScene *m_Scene;
    glm::vec3 *m_Pixels;
    float *m_Energy;
    int m_Width, m_Height, m_Samples;
    std::vector<float> m_LightLotteryTickets;
    float m_LightArea;
    unsigned int m_LightCount;
    Surface *m_SkyBox = nullptr;
    bool m_SkyboxEnabled = false;
    Camera *m_Camera;
    const material::MaterialManager *m_Materials;

    int m_Tiles;

    ctpl::ThreadPool *tPool = nullptr;
    std::vector<std::future<void>> tResults{};
    std::vector<RandomGenerator *> m_Rngs{};
};
