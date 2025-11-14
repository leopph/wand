#pragma once

#include <span>

#include <wand/device_child.hpp>

namespace wand {
struct SwapChainDesc {
  UINT width;
  UINT height;
  UINT buffer_count;
  DXGI_FORMAT format;
  DXGI_USAGE usage;
  DXGI_SCALING scaling;
};


class SwapChain {
public:
  [[nodiscard]] auto GetTextures() const -> std::span<SharedDeviceChildHandle<Texture const> const>;
  [[nodiscard]] auto GetCurrentTextureIndex() const -> UINT;
  [[nodiscard]] auto GetCurrentTexture() const -> Texture const&;

  [[nodiscard]] auto GetSyncInterval() const -> UINT;
  auto SetSyncInterval(UINT sync_interval) -> void;

private:
  explicit SwapChain(Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain, UINT present_flags);

  Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain_;
  std::vector<SharedDeviceChildHandle<Texture>> textures_;
  std::atomic<UINT> sync_interval_{0};
  UINT present_flags_;

  friend GraphicsDevice;
};
}
