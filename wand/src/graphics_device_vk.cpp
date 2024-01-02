#include "graphics_device_vk.hpp"

namespace wand {
GraphicsDeviceVk::GraphicsDeviceVk(HWND const window_handle) :
  window_handle_{window_handle} {
  vk::ApplicationInfo constexpr app_info{"Wand", 1, "Wand", 1, VK_MAKE_API_VERSION(0, 1, 0, 0)};
  vk::InstanceCreateInfo const instance_create_info{{}, &app_info};
  instance_ = vk::raii::Instance{context_, instance_create_info};
}

auto GraphicsDeviceVk::CreateBuffer(Buffer::Desc const& desc) -> std::unique_ptr<Buffer> {
  return nullptr; // TODO
}

auto GraphicsDeviceVk::CreateTexture(Texture::Desc const& desc) -> std::unique_ptr<Texture> {
  return nullptr; // TODO
}
}
