#include "wand/swapchain.hpp"

#include <bit>

using Microsoft::WRL::ComPtr;

namespace wand {
auto SwapChain::GetTextures() const -> std::span<SharedDeviceChildHandle<Texture const> const> {
  return *std::bit_cast<std::span<SharedDeviceChildHandle<Texture const>>*>(&textures_);
}


auto SwapChain::GetCurrentTextureIndex() const -> UINT {
  return swap_chain_->GetCurrentBackBufferIndex();
}


auto SwapChain::GetCurrentTexture() const -> Texture const& {
  return *textures_[GetCurrentTextureIndex()];
}


auto SwapChain::GetSyncInterval() const -> UINT {
  return sync_interval_;
}


auto SwapChain::SetSyncInterval(UINT const sync_interval) -> void {
  sync_interval_ = sync_interval;
}


SwapChain::SwapChain(ComPtr<IDXGISwapChain4> swap_chain, UINT const present_flags) :
  swap_chain_{std::move(swap_chain)},
  present_flags_{present_flags} {
}
}
