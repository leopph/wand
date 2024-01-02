#include "graphics_device.hpp"
#include "graphics_device_d3d12.hpp"

namespace wand {
  auto GraphicsDevice::New(GraphicsApi const api) -> std::unique_ptr<GraphicsDevice> {
    switch (api) {
      using enum GraphicsApi;
#ifdef _WIN64
    case kD3D12: {
      return std::make_unique<GraphicsDeviceD3D12>();
    }
#endif
    }
  }
}
