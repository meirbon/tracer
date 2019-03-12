#include "Renderer/BVHRenderer.h"

#define TILE_HEIGHT 32
#define TILE_WIDTH 32

BVHRenderer::BVHRenderer(WorldScene *scene, Camera *camera, int width,
                         int height)
    : m_Scene(scene), m_Camera(camera), m_Width(width), m_Height(height)
{
    m_TPool = new ctpl::ThreadPool(12);
    m_Tiles = (m_Width / TILE_WIDTH) * (m_Height / TILE_HEIGHT);
    tResults = new std::vector<std::future<void>>(m_Tiles);
}

BVHRenderer::~BVHRenderer()
{
    delete m_TPool;
    delete tResults;
}

void BVHRenderer::Render(Surface *output)
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
            tResults->at(tile_y * hTiles + tile_x) = m_TPool->push(
                [tile_x, tile_y, this, output](int threadId) -> void {
                    for (int y = 0; y < TILE_HEIGHT; y++)
                    {
                        const int pixel_y = y + tile_y * TILE_HEIGHT;
                        for (int x = 0; x < TILE_WIDTH; x++)
                        {
                            const int pixel_x = x + tile_x * TILE_WIDTH;
                            Ray r = m_Camera->GenerateRay(float(pixel_x),
                                                          float(pixel_y));

                            const unsigned int depth = m_Scene->TraceDebug(r);
                            if (depth == 0)
                            {
                                output->Plot(pixel_x, pixel_y, 0x00);
                            }
                            else
                            {
                                const float f_depth = float(depth);
                                glm::vec3 color = {f_depth / 64.0f,
                                                   1.0f - (f_depth / 64.0f),
                                                   0.0f};
                                output->Plot(pixel_x, pixel_y,
                                             Vec3ToInt(color));
                            }
                        }
                    }
                });
        }
    }

    for (auto &r : *tResults)
    {
        r.get();
    }
}

void BVHRenderer::Resize(Texture *newOutput) {}

void BVHRenderer::SwitchSkybox() {}