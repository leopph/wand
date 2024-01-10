#ifdef _WIN64
#include "buffer_d3d12.hpp"

#include <utility>

namespace wand {
BufferD3D12::BufferD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t const cbv_idx, std::uint32_t const srv_idx, std::uint32_t const uav_idx) :
  Buffer{desc},
  alloc_{std::move(alloc)},
  cbv_idx_{cbv_idx},
  srv_idx_{srv_idx},
  uav_idx_{uav_idx} {
}

auto BufferD3D12::Map() const -> void* {
  void* ret;

  if (FAILED(alloc_->GetResource()->Map(0, nullptr, &ret))) {
    return nullptr;
  }

  return ret;
}

auto BufferD3D12::Unmap() const -> void {
  alloc_->GetResource()->Unmap(0, nullptr);
}
}
#endif
