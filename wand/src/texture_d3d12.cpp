#ifdef _WIN64
#include "texture_d3d12.hpp"
#include "device_d3d12.hpp"

#include <utility>

namespace wand {
TextureD3D12::TextureD3D12(Desc const& desc, DeviceD3D12* const device, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t const srv_idx, std::uint32_t const uav_idx, std::uint32_t const rtv_idx, std::uint32_t const dsv_idx) :
  Texture{desc},
  device_{device},
  alloc_{std::move(alloc)},
  srv_idx_{srv_idx},
  uav_idx_{uav_idx},
  rtv_idx_{rtv_idx},
  dsv_idx_{dsv_idx} {
}

TextureD3D12::~TextureD3D12() {
  if (srv_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseShaderResourceView(srv_idx_);
  }
  if (uav_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseUnorderedAccessView(uav_idx_);
  }
  if (rtv_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseRenderTargetView(rtv_idx_);
  }
  if (dsv_idx_ != DeviceD3D12::invalid_descriptor_idx_) {
    device_->ReleaseDepthStencilView(dsv_idx_);
  }
}
}
#endif
