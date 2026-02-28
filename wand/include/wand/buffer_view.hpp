#pragma once

#include <wand/buffer.hpp>
#include <wand/device_child.hpp>

namespace wand {
struct BufferViewDesc {
  UINT64 offset;
  UINT64 size;
  UINT stride;
  bool constant_buffer;
  bool shader_resource;
  bool unordered_access;
  bool acceleration_structure;
};


class BufferView {
public:
  [[nodiscard]] auto GetDesc() const -> BufferViewDesc const&;
  [[nodiscard]] auto GetBuffer() const -> SharedDeviceChildHandle<Buffer> const&;

  [[nodiscard]] auto GetConstantBuffer() const -> UINT;
  [[nodiscard]] auto GetShaderResource() const -> UINT;
  [[nodiscard]] auto GetUnorderedAccess() const -> UINT;
  [[nodiscard]] auto GetAccelerationStructure() const -> UINT;

private:
  BufferView(SharedDeviceChildHandle<Buffer> buffer, UINT cbv, UINT srv, UINT uav, BufferViewDesc const& desc);

  SharedDeviceChildHandle<Buffer> buffer_;
  BufferViewDesc desc_;
  UINT cbv_;
  UINT srv_;
  UINT uav_;

  friend GraphicsDevice;
};
}
