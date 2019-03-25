#pragma once

#include "BVH/GameObject.h"
#include "BVH/TopLevelBVH.h"
#include <GLFW/glfw3.h>

#include "Core/BVHRenderer.h"
#include "Core/Camera.h"
#include "Core/GpuTracer.h"
#include "Core/PathTracer.h"
#include "Core/RayTracer.h"
#include "Core/Scenes.h"
#include "Core/Surface.h"

#include "Materials/MaterialManager.h"

#include "Utils/Messages.h"
#include "Utils/SDLWindow.h"
#include "Utils/Timer.h"
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
	Application(utils::Window *window, RendererType type, int width, int height, const char *scene = nullptr,
				const char *skybox = nullptr);
	~Application();

	void Draw(float deltaTime);

	void Resize(int newWidth, int newHeight);

	void Tick(float deltaTime) noexcept;

	/**
	 * x = true if scrolling right
	 * y = true if scrolling up
	 */
	void MouseScroll(bool x, bool y);

	void MouseScroll(float x, float y);

	void MouseUp(int button) { m_MouseKeyStatus[button] = false; }

	void MouseDown(int button) { m_MouseKeyStatus[button] = true; }

	void MouseMove(int x, int y);

	void MouseMove(float x, float y);

	void KeyUp(int key) noexcept
	{
		if (key >= 0)
			m_KeyStatus[key] = false;
	}

	void KeyDown(int key) noexcept
	{
		if (key >= 0)
			m_KeyStatus[key] = true;
	}

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
	core::Renderer *m_Renderer = nullptr;

	utils::Timer m_MovementTimer;
	utils::Timer m_DynamicTimer;
	utils::Timer m_DBVHBuildTimer;
	utils::Timer m_BVHDebugTimer;
	core::Camera m_Camera;
	core::Surface *m_Screen = nullptr;
	gl::Texture *m_OutputTexture[2] = {nullptr, nullptr};
	gl::Shader *m_DrawShader = nullptr;
	core::Surface *m_Skybox = nullptr;

	int m_tIndex;
	bool m_KeyStatus[512]{};
	bool m_MouseKeyStatus[32]{};
	bool m_MovementLocked = false, m_DynamicLocked = false;
	int m_Width, m_Height;

	prims::SceneObjectList *m_ObjectList;
	prims::GpuTriangleList *m_GpuList;
	bvh::TopLevelBVH *m_Scene = nullptr;

	ctpl::ThreadPool *m_TPool;
	std::future<void> m_RebuildThread;

	bool m_RebuildingBVH = false;
	bool m_BVHDebugMode = false;
	core::BVHRenderer *m_BVHRenderer = nullptr;

	utils::Window *m_Window;
};
