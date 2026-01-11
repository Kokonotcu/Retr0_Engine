#include "Window.h"


bool Window::Init(const Settings& settings) 
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        retro::print("Failed to init SDL Video\n");
        return false;
    }

    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN); // Start hidden
    if (settings.isResizable) flags = (SDL_WindowFlags)(flags | SDL_WINDOW_RESIZABLE);

    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "1");

    window = SDL_CreateWindow(settings.title.c_str(), settings.width, settings.height, flags);

    if (window) 
    {
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);
        return true;
    }
    return false;
}

bool Window::CreateVulkanSurface(VkInstance instance)
{
    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
    {
        retro::print("failed to create surface: ");
        retro::print(SDL_GetError());
		return false;
    }
	return true;
}

void Window::Shutdown() 
{
    if (window) 
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

Window::Extent Window::GetWindowExtent()
{
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
	return { (uint32_t)w, (uint32_t)h };
}

bool Window::ProcessEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) 
    {
        switch (e.type) 
        {
        case SDL_EVENT_QUIT:
            return false;

        case SDL_EVENT_WILL_ENTER_BACKGROUND:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_MINIMIZED:
            isMinimized = true;
            break;

        case  SDL_EVENT_WILL_ENTER_FOREGROUND:
        case SDL_EVENT_WINDOW_RESTORED:
    	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            isMinimized = false;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            resizePending = true;
            break;
        }
    }
    return true;
}