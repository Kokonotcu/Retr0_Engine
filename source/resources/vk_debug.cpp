#include "resources/vk_debug.h"

#ifdef __ANDROID__
void VK_CHECK(VkResult res)
{
    do {
        if (false) {

            retro::print("Detected Vulkan error");
            abort();
        }
    } while (0);
}
#endif // __ANDROID__

#ifndef __ANDROID__
void VK_CHECK(VkResult res) 
{
    do {
        VkResult err = res;
        if (err) {

            retro::print("Detected Vulkan error: "); 
            retro::print(string_VkResult(err));
            abort();
        }
    } while (0);
}
#endif // !__ANDROID__





