#pragma once

#include "Utils/Window.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <functional>

namespace utils
{
class SDLWindow : public Window
{
  public:
	SDLWindow(const char *title, int width, int height, bool fullscreen = false);
	~SDLWindow();

	void SetSize(int width, int height) override;
	void SetTitle(const char *title) override;
	void PollEvents() override;
	void Clear(const glm::vec4 &color) override;
	void Present() override;

	void SwitchFullscreen() override;

	void SetEventCallback(std::function<void(SDL_Event)> callback);
	void SetResizeCallback(std::function<void(int, int)> callback);

  private:
	SDL_GLContext m_Context;
	SDL_Window *m_Window;
	std::function<void(SDL_Event)> m_OnEventCallback = [](SDL_Event) {};
	std::function<void(int, int)> m_OnResizeCallback = [](int, int) {};
};
} // namespace utils