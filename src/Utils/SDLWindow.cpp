#include "SDLWindow.h"
#include "Utils/Window.h"

utils::SDLWindow::SDLWindow(const char *title, int width, int height, bool fullscreen) : Window(title, width, height)
{
	SDL_Init(SDL_INIT_VIDEO);
	auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	m_Window = SDL_CreateWindow("Path tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	m_Context = SDL_GL_CreateContext(m_Window);
	SDL_GL_MakeCurrent(m_Window, m_Context);

	glewExperimental = (GLboolean) true;
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		std::cout << "Could not init GLEW: " << glewGetErrorString(error) << std::endl;
		exit(EXIT_FAILURE);
	}

	glViewport(0, 0, width, height);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	SDL_GL_SetSwapInterval(0);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_CaptureMouse(SDL_FALSE);

	SDL_PumpEvents();
	SDL_SetWindowSize(m_Window, width, height);
}

utils::SDLWindow::~SDLWindow()
{
	SDL_GL_DeleteContext(m_Context);
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void utils::SDLWindow::SetSize(int width, int height)
{
	m_Width = width;
	m_Height = height;
	SDL_SetWindowSize(m_Window, width, height);
	glViewport(0, 0, width, height);
}

void utils::SDLWindow::SetTitle(const char *title)
{
	m_Title = title;
	SDL_SetWindowTitle(m_Window, title);
	m_Title = title;
}

void utils::SDLWindow::SetEventCallback(std::function<void(SDL_Event)> callback) { m_OnEventCallback = callback; }

void utils::SDLWindow::SetResizeCallback(std::function<void(int, int)> callback) { m_OnResizeCallback = callback; }

void utils::SDLWindow::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_WINDOWEVENT)
		{
			const auto &width = event.window.data1;
			const auto &height = event.window.data2;

			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && (m_Width != width || m_Height != height))
			{
				SetSize(width, height);
				m_OnResizeCallback(m_Width, m_Height);
			}
		}
		else
		{
			m_OnEventCallback(event);
		}
	}
}

void utils::SDLWindow::Clear(const glm::vec4 &color) { glClearColor(color.x, color.y, color.z, color.w); }

void utils::SDLWindow::Present() { SDL_GL_SwapWindow(m_Window); }
