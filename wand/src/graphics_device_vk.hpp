#pragma once

#include <vulkan/vulkan_raii.hpp>

#ifdef _WIN64
#include "platform_win32.hpp"
#include "graphics_device.hpp"
#endif

namespace wand {
class GraphicsDeviceVk final : public GraphicsDevice {
#ifdef _WIN64
  using WindowHandleType = HWND;
#endif

  vk::raii::Context context_;
  vk::raii::Instance instance_{VK_NULL_HANDLE};
  WindowHandleType window_handle_;

public:
  explicit GraphicsDeviceVk(HWND window_handle);
};
}
