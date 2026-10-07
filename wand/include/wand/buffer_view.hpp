#pragma once

#include <cstdint>

#include <wand/buffer.hpp>
#include <wand/device_object.hpp>
#include <wand/wandapi.hpp>


namespace wand {
enum class BufferViewUsage : std::uint32_t {
  kNone            = 0,
  kConstantBuffer  = 1 << 0,
  kShaderResource  = 1 << 1,
  kUnorderedAccess = 1 << 2,
};


struct BufferViewDesc {
  std::uint64_t offset;
  std::uint64_t size;
  std::uint32_t stride;
  BufferViewUsage usage;
};


class BufferView {
public:
  [[nodiscard]] WANDAPI
  auto GetDesc() const -> BufferViewDesc const&;

  [[nodiscard]] WANDAPI
  auto GetBuffer() const -> SharedDeviceHandle<Buffer> const&;

  [[nodiscard]] WANDAPI
  auto GetConstantBuffer() const -> UINT;

  [[nodiscard]] WANDAPI
  auto GetShaderResource() const -> UINT;

  [[nodiscard]] WANDAPI
  auto GetUnorderedAccess() const -> UINT;

private:
  BufferView(SharedDeviceHandle<Buffer> buf, std::optional<UINT> cbv, std::optional<UINT> srv,
             std::optional<UINT> uav, BufferViewDesc const& desc);

  SharedDeviceHandle<Buffer> buf_;
  std::optional<UINT> cbv_;
  std::optional<UINT> srv_;
  std::optional<UINT> uav_;

  BufferViewDesc desc_;

  friend GraphicsDevice;
};
}
