#pragma once

#include "Core/Camera.h"
#include "Core/Renderer.h"
#include "Primitives/SceneObjectList.h"
#include "Utils/ctpl.h"

namespace core
{
class Surface;
class BVHRenderer : public Renderer
{
  public:
	BVHRenderer(prims::WorldScene *scene, Camera *camera, int width, int height);
	~BVHRenderer() override;

	void Render(Surface *output) override;
	void Resize(int  width, int  height, gl::Texture * newOutput1, gl::Texture * newOutput2) override;
	void SwitchSkybox() override;

  private:
	Camera *m_Camera;
	prims::WorldScene *m_Scene;
	ctpl::ThreadPool *m_TPool;
	std::vector<std::future<void>> tResults;
	int m_Width, m_Height, m_Tiles;
};
} // namespace core