#include "wand/fence.hpp"

#include <wand/common.hpp>

using Microsoft::WRL::ComPtr;

namespace wand {
auto Fence::GetNextValue() const -> UINT64 {
  return next_val_;
}


auto Fence::GetCompletedValue() const -> UINT64 {
  return fence_->GetCompletedValue();
}


auto Fence::Wait(UINT64 const wait_value) const -> void {
  ThrowIfFailed(fence_->SetEventOnCompletion(wait_value, nullptr), "Failed to wait fence from CPU.");
}


auto Fence::Signal() -> void {
  auto const new_fence_val{next_val_.load()};
  ThrowIfFailed(fence_->Signal(new_fence_val), "Failed to signal fence from the CPU.");
  next_val_ = new_fence_val + 1;
}


Fence::Fence(ComPtr<ID3D12Fence> fence, UINT64 const next_value) :
  fence_{std::move(fence)},
  next_val_{next_value} {
}
}
