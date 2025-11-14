#pragma once

#include <cstdint>

#include <wand/resource.hpp>

namespace wand {
enum class TextureDimension : std::uint8_t {
  k1D,
  k2D,
  k3D,
  kCube
};


struct TextureDesc {
  TextureDimension dimension;
  UINT width;
  UINT height;
  UINT16 depth_or_array_size;
  UINT16 mip_levels;
  DXGI_FORMAT format;
  UINT sample_count;

  bool depth_stencil;
  bool render_target;
  bool shader_resource;
  bool unordered_access;
};


// Get the actual mip level count of the texture. In TextureDesc, mip_levels can be 0, which means that the mip level count is auto calculated.
[[nodiscard]] auto GetActualMipLevels(TextureDesc const& desc) -> UINT;


class Texture : public Resource {
public:
  [[nodiscard]]
  auto GetDesc() const -> TextureDesc const&;
  [[nodiscard]]
  auto Map(UINT subresource) const -> void*;
  auto Unmap(UINT subresource) const -> void;
  [[nodiscard]]
  auto GetDepthStencilView(UINT mip_index) const -> UINT;
  [[nodiscard]]
  auto GetRenderTargetView(UINT mip_index) const -> UINT;

private:
  Texture(Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation, Microsoft::WRL::ComPtr<ID3D12Resource2> resource,
          std::vector<UINT> dsvs, std::vector<UINT> rtvs, std::optional<UINT> srv, std::optional<UINT> uav,
          TextureDesc const& desc);

  TextureDesc desc_;
  // One per mip
  std::vector<UINT> dsvs_;
  // One per mip
  std::vector<UINT> rtvs_;

  friend GraphicsDevice;
};
}
