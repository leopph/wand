#ifdef _WIN64
#include "texture_d3d12.hpp"

#include <utility>

namespace wand {
TextureD3D12::TextureD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t const srv_idx, std::uint32_t const uav_idx, std::uint32_t const rtv_idx, std::uint32_t const dsv_idx) :
  Texture{desc}, alloc_{std::move(alloc)},
  srv_idx_{srv_idx},
  uav_idx_{uav_idx},
  rtv_idx_{rtv_idx},
  dsv_idx_{dsv_idx} {
}
}
#endif
