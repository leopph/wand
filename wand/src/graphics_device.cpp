#include "graphics_device.hpp"

#ifdef _WIN64
#include "graphics_device_d3d12.hpp"
#endif

namespace wand {
  auto GraphicsDevice::New(void* const window_handle, GraphicsApi const api) -> std::unique_ptr<GraphicsDevice> {
    switch (api) {
      using enum GraphicsApi;
#ifdef _WIN64
    case kD3D12: {
      return std::make_unique<GraphicsDeviceD3D12>(static_cast<HWND>(window_handle));
    }
#endif
    }
  }
}
