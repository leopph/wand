#pragma once

#include <concepts>
#include <type_traits>

#include <wand/wandapi.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
class Buffer;
class Texture;
class PipelineState;
class RtStateObject;
class CommandList;
class Fence;
class SwapChain;
class BufferView;

template<typename T>concept DeviceObject =
  std::same_as<std::remove_const_t<T>, Buffer> ||
  std::same_as<std::remove_const_t<T>, Texture> ||
  std::same_as<std::remove_const_t<T>, PipelineState> ||
  std::same_as<std::remove_const_t<T>, RtStateObject> ||
  std::same_as<std::remove_const_t<T>, CommandList> ||
  std::same_as<std::remove_const_t<T>, Fence> ||
  std::same_as<std::remove_const_t<T>, SwapChain> ||
  std::same_as<std::remove_const_t<T>, BufferView>;

template<DeviceObject T>
class DeviceObjectDeleter;

template<DeviceObject T>
using UniqueDeviceHandle = std::unique_ptr<T, DeviceObjectDeleter<T>>;

template<DeviceObject T>
using SharedDeviceHandle = std::shared_ptr<T>;

class GraphicsDevice;


template<DeviceObject T>
class DeviceObjectDeleter {
public:
  DeviceObjectDeleter() = default;
  explicit WANDAPI DeviceObjectDeleter(GraphicsDevice& device);
  WANDAPI auto operator()(T const* device_child) -> void;

private:
  GraphicsDevice* device_;
};


extern template class DeviceObjectDeleter<Buffer>;
extern template class DeviceObjectDeleter<Texture>;
extern template class DeviceObjectDeleter<PipelineState>;
extern template class DeviceObjectDeleter<RtStateObject>;
extern template class DeviceObjectDeleter<CommandList>;
extern template class DeviceObjectDeleter<Fence>;
extern template class DeviceObjectDeleter<SwapChain>;
extern template class DeviceObjectDeleter<BufferView>;
}
