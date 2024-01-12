#pragma once

#include "platform_d3d12.hpp"
#include "texture.hpp"

#include <cstdint>

namespace wand {
class TextureD3D12 final : public Texture {
public:
  TextureD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc, std::uint32_t srv_idx, std::uint32_t uav_idx, std::uint32_t rtv_idx, std::uint32_t dsv_idx);

private:
  Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc_;
  std::uint32_t srv_idx_;
  std::uint32_t uav_idx_;
  std::uint32_t rtv_idx_;
  std::uint32_t dsv_idx_;
};
}
