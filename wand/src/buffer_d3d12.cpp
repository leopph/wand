#ifdef _WIN64
#include "buffer_d3d12.hpp"
#include "device_d3d12.hpp"

#include <utility>

namespace wand {
BufferD3D12::BufferD3D12(Desc const& desc, DeviceD3D12* const device, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t const cbv_idx, std::uint32_t const srv_idx, std::uint32_t const uav_idx) :
  Buffer{desc},
  device_{device},
  alloc_{std::move(alloc)},
  cbv_idx_{cbv_idx},
  srv_idx_{srv_idx},
  uav_idx_{uav_idx} {
}

BufferD3D12::~BufferD3D12() {
  if (cbv_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseConstantBufferView(cbv_idx_);
  }
  if (srv_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseShaderResourceView(srv_idx_);
  }
  if (uav_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseUnorderedAccessView(uav_idx_);
  }
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
