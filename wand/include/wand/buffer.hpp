#pragma once

#include <wand/resource.hpp>

namespace wand {
struct BufferDesc {
  UINT64 size;
  bool allow_shader_resource;
  bool allow_unordered_access;
  bool allow_acceleration_structure;
};


class Buffer : public Resource {
public:
  [[nodiscard]]
  auto GetDesc() const -> BufferDesc const&;

private:
  Buffer(Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation, Microsoft::WRL::ComPtr<ID3D12Resource2> resource,
         BufferDesc const& desc);

  BufferDesc desc_;

  friend GraphicsDevice;
};
}
