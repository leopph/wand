#include "wand/texture.hpp"

#include <cmath>

using Microsoft::WRL::ComPtr;

namespace wand {
auto GetActualMipLevels(TextureDesc const& desc) -> UINT {
  return desc.mip_levels == 0
           ? static_cast<UINT16>(std::ceil(std::max(std::log2(desc.width), std::log2(desc.height)) + 1))
           : desc.mip_levels;
}

auto Texture::GetDesc() const -> TextureDesc const& {
  return desc_;
}


auto Texture::Map(UINT const subresource) const -> void* {
  return InternalMap(subresource, nullptr);
}


auto Texture::Unmap(UINT const subresource) const -> void {
  InternalUnmap(subresource, nullptr);
}


auto Texture::GetDepthStencilView(UINT const mip_index) const -> UINT {
  return dsvs_.at(mip_index);
}


auto Texture::GetRenderTargetView(UINT const mip_index) const -> UINT {
  return rtvs_.at(mip_index);
}


Texture::Texture(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource2> resource, std::vector<UINT> dsvs,
                 std::vector<UINT> rtvs, std::optional<UINT> const srv, std::optional<UINT> const uav,
                 TextureDesc const& desc) :
  Resource{std::move(allocation), std::move(resource), srv, uav},
  desc_{desc},
  dsvs_{std::move(dsvs)},
  rtvs_{std::move(rtvs)} {
}
}
