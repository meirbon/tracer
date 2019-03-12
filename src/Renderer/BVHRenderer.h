#pragma once

#include "Primitives/SceneObjectList.h"
#include "Renderer/Camera.h"
#include "Renderer/Renderer.h"
#include "Utils/ctpl.h"

class BVHRenderer : public Renderer
{
  public:
    BVHRenderer(WorldScene *scene, Camera *camera, int width, int height);
    ~BVHRenderer();

    virtual void Render(Surface *output) override;
    virtual void Resize(Texture *newOutput) override;
    virtual void SwitchSkybox() override;

  private:
    Camera *m_Camera;
    WorldScene *m_Scene;
    ctpl::ThreadPool *m_TPool;
    std::vector<std::future<void>> *tResults = nullptr;
    int m_Width, m_Height, m_Tiles;
};