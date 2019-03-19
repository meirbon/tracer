#include "Renderer/BVHRenderer.h"
#include <iostream>

#define TILE_HEIGHT 32
#define TILE_WIDTH 32

BVHRenderer::BVHRenderer(WorldScene *scene, Camera *camera, int width, int height)
    : m_Camera(camera), m_Scene(scene), m_Width(width), m_Height(height)
{
    m_TPool = new ctpl::ThreadPool(12);
    m_Tiles = (m_Width / TILE_WIDTH) * (m_Height / TILE_HEIGHT);
}

BVHRenderer::~BVHRenderer()
{
    delete m_TPool;
}

void BVHRenderer::Render(Surface *output)
{
    m_Width = output->GetWidth();
    m_Height = output->GetHeight();

    const int vTiles = m_Height / TILE_HEIGHT;
    const int hTiles = m_Width / TILE_WIDTH;

    for (int i = 0; i < ctpl::nr_of_cores; i++)
    {
        tResults.push_back(m_TPool->push([i, this, output](int) {
            for (int y = i; y < m_Height; y += ctpl::nr_of_cores)
            {
                for (int x = 0; x < m_Width; x++)
                {
                    Ray r = m_Camera->GenerateRay(float(x), float(y));

                    const unsigned int depth = m_Scene->TraceDebug(r);
                    uint col = 0;

                    if (depth > 0)
                    {
                        const auto f_depth = float(depth);
                        const vec3 color = {f_depth / 64.0f, 1.0f - (f_depth / 64.0f), 0.0f};
                        col = Vec3ToInt(color);
                    }

                    output->Plot(x, y, col);
                }
            }
        }));
    }

    for (auto &tResult : tResults)
    {
        tResult.get();
    }

    tResults.clear();
}

void BVHRenderer::Resize(Texture *newOutput) {}

void BVHRenderer::SwitchSkybox() {}