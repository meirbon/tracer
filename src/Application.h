#pragma once

#include <SDL2/SDL.h>

#include "BVH/GameObject.h"
#include "BVH/TopLevelBVH.h"

#include "Renderer/BVHRenderer.h"
#include "Renderer/Camera.h"
#include "Renderer/GpuTracer.h"
#include "Renderer/PathTracer.h"
#include "Renderer/RayTracer.h"
#include "Renderer/Scenes.h"
#include "Renderer/Surface.h"

#include "Materials/MaterialManager.h"

#include "Utils/Messages.h"
#include "Utils/ctpl.h"

#include "Primitives/Model.h"

#include "GL/GLTools.h"
#include "GL/Shader.h"
#include "GL/Texture.h"

enum RendererType
{
    CPU,
    GPU,
    CPU_RAYTRACER,
    GPU_RAYTRACER
};

class Application
{
  public:
    Application(SDL_Window *window, RendererType type, int width, int height,
                const char *scene = nullptr, const char *skybox = nullptr);
    ~Application();

    void Draw(float deltaTime);

    void Resize(int newWidth, int newHeight);

    void Tick(float deltaTime) noexcept;

    /**
     * x = true if scrolling right
     * y = true if scrolling up
     */
    void MouseScroll(bool x, bool y);

    void MouseUp(int button) { m_MouseKeyStatus[button] = false; }

    void MouseDown(int button) { m_MouseKeyStatus[button] = true; }

    void MouseMove(int x, int y);

    void KeyUp(int key) noexcept { m_KeyStatus[key] = false; }

    void KeyDown(int key) noexcept { m_KeyStatus[key] = true; }

    void HandleKeys(float deltaTime) noexcept;

    void SwitchMovementLocked()
    {
        if (m_MovementTimer.elapsed() >= 250.0f)
        {
            m_MovementLocked = !m_MovementLocked;
            m_MovementTimer.reset();
        }
    }

    void SwitchDynamicLocked()
    {
        if (m_DynamicTimer.elapsed() >= 250.0f)
        {
            m_DynamicLocked = !m_DynamicLocked;
            m_DynamicTimer.reset();
        }
    }

    void SwitchBVHMode()
    {
        if (m_BVHDebugTimer.elapsed() >= 250.0f)
        {
            m_BVHDebugMode = !m_BVHDebugMode;
            m_BVHDebugTimer.reset();
        }
    }

    inline int GetWidth() { return m_Width; }

    inline int GetHeight() { return m_Height; }

  private:
    RendererType m_Type;
    Renderer *m_Renderer = nullptr;

    Timer m_MovementTimer;
    Timer m_DynamicTimer;
    Timer m_DBVHBuildTimer;
    Timer m_BVHDebugTimer;
    Camera m_Camera;
    Surface *m_Screen = nullptr;
    Texture *m_OutputTexture[2] = {nullptr, nullptr};
    Shader *m_DrawShader = nullptr;
    Surface *m_Skybox = nullptr;

    int m_tIndex;
    bool m_KeyStatus[256]{};
    bool m_MouseKeyStatus[32]{};
    bool m_MovementLocked = false, m_DynamicLocked = false;
    int m_Width, m_Height;

    SceneObjectList *m_ObjectList;
    GpuTriangleList *m_GpuList;
    bvh::TopLevelBVH *m_Scene = nullptr;

    ctpl::ThreadPool *m_TPool;
    std::future<void> m_RebuildThread;

    bool m_RebuildingBVH = false;
    bool m_BVHDebugMode = false;
    BVHRenderer *m_BVHRenderer = nullptr;

  public:
    SDL_Window *m_Window;
};
