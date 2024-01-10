#include "device.hpp"

#ifdef _WIN64
#include "device_d3d12.hpp"
#endif

namespace wand {
auto Device::New(void* const window_handle, GraphicsApi const api) -> std::unique_ptr<Device> {
  switch (api) {
    using enum GraphicsApi;
#ifdef _WIN64
  case kD3D12: {
    return std::make_unique<DeviceD3D12>(static_cast<HWND>(window_handle));
  }
#endif
  }
}
}
