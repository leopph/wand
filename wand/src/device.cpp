#include "device.hpp"

#ifdef _WIN64
#include "device_d3d12.hpp"
#endif

namespace wand {
auto Device::New(void* const window_handle) -> std::unique_ptr<Device> {
    return std::make_unique<DeviceD3D12>(static_cast<HWND>(window_handle));
}
}
