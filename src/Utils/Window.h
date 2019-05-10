#pragma once

#include <glm/glm.hpp>
#include <iostream>

namespace utils
{
class Window
{
  public:
	virtual ~Window() = default;

	virtual int GetWidth() = 0;
	virtual int GetHeight() = 0;

	virtual void SetSize(int width, int height) = 0;
	virtual void SetTitle(const char *title) = 0;
	virtual void PollEvents() = 0;
	virtual void Clear(const glm::vec4 &color) = 0;
	virtual void Present() = 0;
	virtual void SwitchFullscreen() = 0;
};
} // namespace utils