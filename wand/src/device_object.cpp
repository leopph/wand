#include "wand/device_object.hpp"

#include "wand/graphics_device.hpp"


namespace wand {
template<DeviceObject T>
DeviceObjectDeleter<T>::DeviceObjectDeleter(GraphicsDevice& device) :
  device_{&device} {}


template<DeviceObject T>
auto DeviceObjectDeleter<T>::operator()(T const* device_child) -> void {
  if (device_) {
    if constexpr (std::same_as<T, Buffer>) {
      device_->DestroyBuffer(device_child);
    } else if constexpr (std::same_as<T, Texture>) {
      device_->DestroyTexture(device_child);
    } else if constexpr (std::same_as<T, PipelineState>) {
      device_->DestroyPipelineState(device_child);
    } else if constexpr (std::same_as<T, RtStateObject>) {
      device_->DestroyRtStateObject(device_child);
    } else if constexpr (std::same_as<T, CommandList>) {
      device_->DestroyCommandList(device_child);
    } else if constexpr (std::same_as<T, Fence>) {
      device_->DestroyFence(device_child);
    } else if constexpr (std::same_as<T, SwapChain>) {
      device_->DestroySwapChain(device_child);
    } else if constexpr (std::same_as<T, BufferView>) {
      device_->DestroyBufferView(device_child);
    }
  }
}


template class DeviceObjectDeleter<Buffer>;
template class DeviceObjectDeleter<Texture>;
template class DeviceObjectDeleter<PipelineState>;
template class DeviceObjectDeleter<RtStateObject>;
template class DeviceObjectDeleter<CommandList>;
template class DeviceObjectDeleter<Fence>;
template class DeviceObjectDeleter<SwapChain>;
template class DeviceObjectDeleter<BufferView>;
}
