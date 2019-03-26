#include "Application.h"

#include "Shared.h"
#include "Utils/GLFWWindow.h"
#include "Utils/SDLWindow.h"
#include "Utils/Timer.h"

constexpr int SCRWIDTH = 1024;
constexpr int SCRHEIGHT = 768;

using namespace utils;

#define USE_SDL 0

int main(int argc, char *argv[])
{
	printf("Application started.\n");

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
#if USE_SDL
	auto window = utils::SDLWindow("Tracer", SCRWIDTH, SCRHEIGHT, oFullScreen);
#else
	auto window = utils::GLFWWindow("Tracer", SCRWIDTH, SCRHEIGHT, oFullScreen);
#endif
	auto app = new Application(&window, rendererType, SCRWIDTH, SCRHEIGHT, file.empty() ? nullptr : f);

	Timer t, drawTimer;
#if USE_SDL
	window.SetEventCallback([&exitApp, &app](SDL_Event event) {
		switch (event.type)
		{
		case SDL_QUIT:
			exitApp = 1;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				exitApp = 1;
			app->KeyDown(event.key.keysym.scancode);
			break;
		case SDL_KEYUP:
			app->KeyUp(event.key.keysym.scancode);
			break;
		case SDL_MOUSEMOTION:
			app->MouseMove(event.motion.xrel, event.motion.yrel);
			break;
		case SDL_MOUSEBUTTONUP:
			app->MouseUp(event.button.button);
			break;
		case SDL_MOUSEBUTTONDOWN:
			app->MouseDown(event.button.button);
			break;
		case SDL_MOUSEWHEEL:
			app->MouseScroll(event.wheel.x > 0, event.wheel.y > 0);
			break;
		default:
			break;
		}
	});
#else
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
				else
					app->MouseUp(event.key);
			}
			else
			{
				if (event.state == KEY_PRESSED)
					app->KeyDown(event.key);
				else
					app->KeyUp(event.key);
			}
			break;
		case MOUSE:
			if (event.state == KEY_PRESSED)
				app->MouseDown(event.key);
			else if (event.state == KEY_RELEASED)
				app->MouseUp(event.key);
			else if (event.state == MOUSE_MOVE)
				app->MouseMove(event.x, event.y);
			else if (event.state == MOUSE_SCROLL)
				app->MouseScroll(event.x, event.y);
			break;
		}
	});
#endif

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