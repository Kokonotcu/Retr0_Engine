#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <string>

#include "tools/printer.h"

class Window 
{
public:
    struct Settings 
    {
        std::string title = "Retr0 Engine";
        int width = 800;
        int height = 600;
        bool isResizable = true;
    };
    struct Extent 
    {
        uint32_t width;
        uint32_t height;
	};

    bool Init(const Settings& settings);
	bool CreateVulkanSurface(VkInstance instance);
    void Shutdown();

    // Returns false if the app should quit
    bool ProcessEvents();

    // Helper to get size for Vulkan (handles minimization checks internally)
    Extent GetWindowExtent();
	VkSurfaceKHR GetVulkanSurface() const { return surface; }
    bool IsMinimized() const { return isMinimized; }
    bool IsResizePending() const { return resizePending; }
    void ResizeHandled() { resizePending = false; }

private:
    SDL_Window* window{ nullptr };
    VkSurfaceKHR surface;
    bool isMinimized{ false };
    bool resizePending{ false };
};