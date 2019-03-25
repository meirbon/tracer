#include "Utils/Window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>

namespace utils
{
enum EventType
{
	KEY = 0,
	MOUSE = 1,
	CLOSED = 2
};

enum State
{
	KEY_PRESSED = 0,
	KEY_RELEASED = 1,
	MOUSE_SCROLL = 2,
	MOUSE_MOVE = 3,
};

struct Event
{
	EventType type;
	int key;
	State state;
	float x, y;
};
class GLFWWindow : public Window
{
  public:
	GLFWWindow(const char *title, int width, int height, bool fullscreen = false, bool lockMouse = false);
	~GLFWWindow();

	void SetSize(int width, int height) override;
	void SetTitle(const char *title) override;
	void PollEvents() override;
	void Clear(const glm::vec4 &color) override;
	void Present() override;

	void SetEventCallback(std::function<void(Event event)> callback);
	void SetResizeCallback(std::function<void(int, int)> callback);

	static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void InputCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void ErrorCallback(int code, const char *error);
	static void MouseCallback(GLFWwindow *window, double xPos, double yPos);
	static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
	static void MouseScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

	GLFWmonitor *m_Monitor = nullptr;
	std::function<void(Event event)> m_OnEventCallback = [](Event) {};
	std::function<void(int, int)> m_OnResizeCallback = [](int, int) {};

	float m_LastX, m_LastY;
	bool m_FirstMouse = true;

  private:
	GLFWwindow *m_Window;
};
} // namespace utils