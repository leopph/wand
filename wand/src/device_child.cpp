#include "wand/device_child.hpp"

#include "wand/wand.hpp"

namespace wand {
template<DeviceChild T>
DeviceChildDeleter<T>::DeviceChildDeleter(GraphicsDevice& device) :
  device_{&device} {
}


template<DeviceChild T>
auto DeviceChildDeleter<T>::operator()(T const* device_child) -> void {
  if (device_) {
    if constexpr (std::same_as<T, Buffer>) {
      device_->DestroyBuffer(device_child);
    } else if constexpr (std::same_as<T, Texture>) {
      device_->DestroyTexture(device_child);
    } else if constexpr (std::same_as<T, PipelineState>) {
      device_->DestroyPipelineState(device_child);
    } else if constexpr (std::same_as<T, CommandList>) {
      device_->DestroyCommandList(device_child);
    } else if constexpr (std::same_as<T, Fence>) {
      device_->DestroyFence(device_child);
    } else if constexpr (std::same_as<T, SwapChain>) {
      device_->DestroySwapChain(device_child);
    }
  }
}
}
