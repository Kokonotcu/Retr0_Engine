#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <tools/printer.h>

#ifndef __ANDROID__
#include <vulkan/vk_enum_string_helper.h>

#endif // __ANDROID__


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

void VK_CHECK(VkResult);



