#pragma once

#include <concepts>
#include <type_traits>

#include <wand/platforms/d3d12.hpp>

namespace wand {
class Buffer;
class Texture;
class PipelineState;
class CommandList;
class Fence;
class SwapChain;

template<typename T>concept DeviceChild = std::same_as<std::remove_const_t<T>, Buffer> || std::same_as<
  std::remove_const_t<T>, Texture> || std::same_as<
  std::remove_const_t<T>, PipelineState> || std::same_as<
  std::remove_const_t<T>, CommandList> || std::same_as<
  std::remove_const_t<T>, Fence> || std::same_as<
  std::remove_const_t<T>, SwapChain>;

template<DeviceChild T>
class DeviceChildDeleter;

template<DeviceChild T>
using UniqueDeviceChildHandle = std::unique_ptr<T, DeviceChildDeleter<T>>;

template<DeviceChild T>
using SharedDeviceChildHandle = std::shared_ptr<T>;

class GraphicsDevice;

template<DeviceChild T>
class DeviceChildDeleter {
public:
  DeviceChildDeleter() = default;
  explicit DeviceChildDeleter(GraphicsDevice& device);
  auto operator()(T const* device_child) -> void;

private:
  GraphicsDevice* device_;
};

extern template DeviceChildDeleter<Buffer>;
extern template DeviceChildDeleter<Texture>;
extern template DeviceChildDeleter<PipelineState>;
extern template DeviceChildDeleter<CommandList>;
extern template DeviceChildDeleter<Fence>;
extern template DeviceChildDeleter<SwapChain>;
}
