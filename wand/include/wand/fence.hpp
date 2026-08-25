#pragma once

#include <mutex>

#include <wand/wandapi.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
class Fence {
public:
  [[nodiscard]] WANDAPI
  auto GetNextValue() const -> UINT64;

  [[nodiscard]] WANDAPI
  auto GetCompletedValue() const -> UINT64;

  WANDAPI
  auto Wait(UINT64 wait_value) const -> void;

  WANDAPI
  auto Signal() -> void;

private:
  explicit Fence(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT64 next_value);

  Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
  std::atomic<UINT64> next_val_;
  std::mutex mutex_;

  friend class GraphicsDevice;
};
}
