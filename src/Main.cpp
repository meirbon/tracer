#include "Application.h"

#include "Shared.h"
#include "Utils/GLFWWindow.h"
#include "Utils/Timer.h"

constexpr int SCRWIDTH = 1024;
constexpr int SCRHEIGHT = 512;

int main(int argc, char *argv[])
{
	using namespace utils;
	std::cout << "Application started." << std::endl;

	bool oFullScreen = false;
	RendererType rendererType = CPU;
	std::string file;

	for (int i = 1; i < argc; i++)
	{
		const std::string str = argv[i];
		if (str == "--fullscreen" || str == "-f")
			oFullScreen = true;
		else if (str == "--gpu" || str == "-g")
			rendererType = GPU;
		else if (str == "--cpu" || str == "-c")
			rendererType = CPU;
		else
			file = str;
	}

	int exitApp = 0;
	const char *f = file.c_str();
	auto window = utils::GLFWWindow("Tracer", SCRWIDTH, SCRHEIGHT, oFullScreen);
	auto app = new Application(&window, rendererType, SCRWIDTH, SCRHEIGHT, file.empty() ? nullptr : f);

	Timer t, drawTimer;
	window.SetEventCallback([&exitApp, &app](utils::Event event) {
		switch (event.type)
		{
		case CLOSED:
			exitApp = 1;
			break;
		case KEY:
			if (event.key == GLFW_KEY_ESCAPE)
			{
				exitApp = 1;
				break;
			}

			if (event.key >= 0 && event.key <= 7)
			{
				if (event.state == KEY_PRESSED)
					app->MouseDown(event.key);
				else if (event.state == KEY_RELEASED)
					app->MouseUp(event.key);
			}
			else
			{
				if (event.state == KEY_PRESSED)
					app->KeyDown(event.key);
				else if (event.state == KEY_RELEASED)
					app->KeyUp(event.key);
			}
			break;
		case MOUSE:
			if (event.state == KEY_PRESSED)
				app->MouseDown(event.key);
			else if (event.state == KEY_RELEASED)
				app->MouseUp(event.key);
			else if (event.state == MOUSE_MOVE)
				app->MouseMove(vec2{event.x, event.y});
			else if (event.state == MOUSE_SCROLL)
				app->MouseScroll(vec2{event.x, event.y});
			break;
		}
	});

	window.SetResizeCallback([&app](int width, int height) { app->Resize(width, height); });

	while (!exitApp)
	{
		const float elapsed = t.elapsed();
		t.reset();

		window.PollEvents();

		app->Tick(elapsed);
		app->Draw(elapsed);

		window.Present();
	}

	delete app;
	return EXIT_SUCCESS;
}