#pragma once

#include <cstdint>
#include <optional>

#include <wand/buffer.hpp>
#include <wand/buffer_region.hpp>
#include <wand/device_object.hpp>
#include <wand/wandapi.hpp>


namespace wand {
enum class AccelerationStructureType : std::uint8_t {
  kBottomLevel,
  kTopLevel,
};


struct AccelerationStructureDesc {
  BufferRegion region;
  AccelerationStructureType type;
};


class AccelerationStructure {
public:
  [[nodiscard]] WANDAPI
  auto GetDesc() const -> AccelerationStructureDesc const&;

  [[nodiscard]] WANDAPI
  auto GetBuffer() const -> SharedDeviceHandle<Buffer> const&;

  [[nodiscard]] WANDAPI
  auto GetGpuAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS;

  [[nodiscard]] WANDAPI
  auto GetRegion() const -> BufferRegion const&;

  [[nodiscard]] WANDAPI
  auto GetShaderResource() const -> UINT;

private:
  AccelerationStructure(SharedDeviceHandle<Buffer> buf, std::optional<UINT> srv,
                        AccelerationStructureDesc const& desc);

  SharedDeviceHandle<Buffer> buf_;
  AccelerationStructureDesc desc_;
  std::optional<UINT> srv_;

  friend GraphicsDevice;
};
}
