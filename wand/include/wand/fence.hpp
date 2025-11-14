#pragma once

#include <wand/platforms/d3d12.hpp>

namespace wand {
class Fence {
public:
  [[nodiscard]] auto GetNextValue() const -> UINT64;
  [[nodiscard]] auto GetCompletedValue() const -> UINT64;
  auto Wait(UINT64 wait_value) const -> void;
  auto Signal() -> void;

private:
  explicit Fence(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT64 next_value);

  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  std::atomic<UINT64> next_val_;

  friend class GraphicsDevice;
};
}
