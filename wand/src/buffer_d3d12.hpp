#pragma once

#include "platform_d3d12.hpp"
#include "buffer.hpp"

#include <cstdint>

namespace wand {
class BufferD3D12 final : public Buffer {
  Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc_;
  std::uint32_t cbv_idx_;
  std::uint32_t srv_idx_;
  std::uint32_t uav_idx_;

public:
  BufferD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t cbv_idx, std::uint32_t srv_idx, std::uint32_t uav_idx);
  [[nodiscard]] auto Map() const -> void* override;
  auto Unmap() const -> void override;
};
}
