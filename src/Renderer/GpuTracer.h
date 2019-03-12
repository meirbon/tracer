#pragma once

#include <vector>

#include "BVH/MBVHTree.h"
#include "BVH/StaticBVHTree.h"
#include "Renderer/Camera.h"
#include "GL/OpenCL.h"
#include "GL/Buffer.h"
#include "GL/Texture.h"
#include "Renderer/PathTracer.h"
#include "Primitives/GpuTriangleList.h"

struct TextureInfo
{
    int width, height, offset, dummy;
    TextureInfo(int width, int height, int offset)
    {
        this->width = width;
        this->height = height;
        this->offset = offset;
    }
};

class SceneObjectList;
class ComputeBuffer;

class GpuTracer : public Renderer
{
  public:
    friend class Application;

    GpuTracer() = default;
    GpuTracer(GpuTriangleList *objectList, Texture *targetTexture1, Texture *targetTexture2, Camera *camera,
              Surface *skyBox = nullptr, ctpl::ThreadPool *pool = nullptr);
    ~GpuTracer();

    void Render(Surface *output) override;

    void Reset() override;

    inline void SetMode(Mode mode) override
    {
        Kernel::SyncQueue();
        m_Mode = mode;
        intersectRaysKernelRef->SetArgument(18, m_Mode);
        intersectRaysKernelOpt->SetArgument(18, m_Mode);
        intersectRaysKernelBVH->SetArgument(18, m_Mode);
        intersectRaysKernelMF->SetArgument(18, m_Mode);
    }

    inline void SwitchSkybox() override
    {
        Kernel::SyncQueue();
        this->m_SkyboxEnabled = (this->m_HasSkybox && !this->m_SkyboxEnabled);
        intersectRaysKernelRef->SetArgument(15, m_SkyboxEnabled);
        intersectRaysKernelOpt->SetArgument(15, m_SkyboxEnabled);
        intersectRaysKernelBVH->SetArgument(15, m_SkyboxEnabled);
        intersectRaysKernelMF->SetArgument(15, m_SkyboxEnabled);
    }

    void Resize(Texture *newOutput) override;

    void SetupCamera();
    void SetupSeeds(int width, int height);

    void SetupObjects();
    void SetupMaterials();

    void SetupNEEData();
    void SetupTextures();
    void SetupSkybox(Surface *skyBox);

    void SetArguments();

    inline static unsigned int UpperPowerOfTwo(unsigned int v)
    {
        return static_cast<unsigned int>(pow(2, ceil(log(v) / log(2))));
    }

    inline const char *GetModeString() const noexcept override
    {
        switch (m_Mode)
        {
        case (NEE_MIS):
            return "NEE, MIS";
        case (Reference):
            return "Reference";
        case (NEE):
        case (IS):
        case (NEE_IS):
        case (NEEMicrofacet):
        case (ReferenceMicrofacet):
        default:
            return "Reference Microfacet";
        }
    }

  private:
    Camera *m_Camera = nullptr;
    GpuTriangleList *m_ObjectList = nullptr;
    bvh::StaticBVHTree *m_BVHTree = nullptr;
    bvh::MBVHTree *m_MBVHTree = nullptr;
    Texture *outputTexture[2] = {nullptr, nullptr};
    Buffer *outputBuffer = nullptr;

    Buffer *primitiveIndicesBuffer = nullptr;
    Buffer *BVHNodeBuffer = nullptr;
    Buffer *MBVHNodeBuffer = nullptr;
    Buffer *seedBuffer = nullptr;
    Buffer *previousColorBuffer = nullptr;
    Buffer *colorBuffer = nullptr;
    Buffer *textureBuffer = nullptr;
    Buffer *textureInfoBuffer = nullptr;
    Buffer *skyDome = nullptr;
    Buffer *skyDomeInfo = nullptr;

    Buffer *raysBuffer = nullptr;
    Buffer *cameraBuffer = nullptr;
    Buffer *materialBuffer = nullptr;
    Buffer *microfacetBuffer = nullptr;
    Buffer *triangleBuffer = nullptr;

    Buffer *lightIndices = nullptr;
    Buffer *lightLotteryTickets = nullptr;
    int lightCount;
    float lightArea;

    Kernel *generateRayKernel = nullptr;
    Kernel *intersectRaysKernelRef = nullptr;
    Kernel *intersectRaysKernelMF = nullptr;
    Kernel *intersectRaysKernelBVH = nullptr;
    Kernel *intersectRaysKernelOpt = nullptr;
    Kernel *drawKernel = nullptr;

    Kernel *wIntersectKernel = nullptr;
    Kernel *wShadeKernel = nullptr;
    Kernel *wDrawKernel = nullptr;

    int tIndex = 0;
    int m_Samples = 0;
    int m_Width, m_Height;

    bool m_SkyboxEnabled;
    bool m_HasSkybox = false;
};
