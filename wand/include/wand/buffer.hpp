#pragma once

#include <wand/resource.hpp>

namespace wand {
struct BufferDesc {
  UINT64 size;
  UINT stride;
  bool constant_buffer;
  bool shader_resource;
  bool unordered_access;
};

class Buffer : public Resource {
public:
  [[nodiscard]]
  auto GetDesc() const -> BufferDesc const&;
  [[nodiscard]]
  auto GetConstantBuffer() const -> UINT;

private:
  Buffer(Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation, Microsoft::WRL::ComPtr<ID3D12Resource2> resource,
         std::optional<UINT> cbv, std::optional<UINT> srv, std::optional<UINT> uav, BufferDesc const& desc);

  BufferDesc desc_;
  std::optional<UINT> cbv_;

  friend GraphicsDevice;
};
}
