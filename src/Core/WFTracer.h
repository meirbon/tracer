#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "BVH/MBVHTree.h"
#include "BVH/StaticBVHTree.h"
#include "CL/Buffer.h"
#include "CL/OpenCL.h"
#include "Core/Camera.h"
#include "Core/PathTracer.h"
#include "GL/Texture.h"
#include "Primitives/TriangleList.h"
#include "Utils/Memory.h"

namespace core
{
class WFTracer : public Renderer
{
  public:
	WFTracer() = default;
	WFTracer(TriangleList *tList, gl::Texture *t1, gl::Texture *t2, Camera *camera, Surface *skybox = nullptr,
			 ctpl::ThreadPool *pool = nullptr);
	~WFTracer() override;

	void Render(Surface *output) override;
	void Reset() override;

	inline void SetMode(Mode mode) override
	{
		cl::Kernel::SyncQueue();
		m_Mode = mode;
	}

	inline void SwitchSkybox() override
	{
		cl::Kernel::SyncQueue();
		this->m_SkyboxEnabled = (this->m_HasSkybox && !this->m_SkyboxEnabled);
	}

	void Resize(gl::Texture *newOutput) override;

	void setupCamera();
	void setupObjects();
	void setupMaterials();
	void setupNEEData();
	void setupTextures();
	void setupSkybox(Surface *skybox);
	void setArguments();

  private:
	Camera *m_Camera;
	bool m_SkyboxEnabled{};
	bool m_HasSkybox = false;
	int frame, samples, width, height;

	bvh::StaticBVHTree *m_BVHTree = nullptr;
	bvh::MBVHTree *m_MBVHTree = nullptr;
	gl::Texture *outputTexture[2] = {nullptr, nullptr};
	cl::Buffer *outputBuffer = nullptr;

	cl::Buffer *primitiveIndicesBuffer = nullptr;
	cl::Buffer *MBVHNodeBuffer = nullptr;
	cl::Buffer *colorBuffer = nullptr;
	cl::Buffer *textureBuffer = nullptr;
	cl::Buffer *textureInfoBuffer = nullptr;
	cl::Buffer *skyDome = nullptr;
	cl::Buffer *skyDomeInfo = nullptr;

	cl::Buffer *raysBuffer = nullptr;
	cl::Buffer *cameraBuffer = nullptr;
	cl::Buffer *materialBuffer = nullptr;
	cl::Buffer *microfacetBuffer = nullptr;

	cl::Buffer *verticesBuffer = nullptr;
	cl::Buffer *normalBuffer = nullptr;
	cl::Buffer *texCoordBuffer = nullptr;
	cl::Buffer *cnBuffer = nullptr;
	cl::Buffer *indicesBuffer = nullptr;

	cl::Buffer *lightIndices = nullptr;
	cl::Buffer *lightLotteryTickets = nullptr;
	int lightCount{};
	float lightArea{};

	cl::Kernel *wGenerateRayKernel = nullptr;
	cl::Kernel *wIntersectKernel = nullptr;
	cl::Kernel *wShadeKernel = nullptr;
	cl::Kernel *wDrawKernel = nullptr;

	TriangleList *m_TList;
};
} // namespace core
