#pragma once

#include <mutex>

#include <wand/platforms/d3d12.hpp>

namespace wand::details {
class DescriptorHeap {
public:
  [[nodiscard]] auto Allocate() -> UINT;
  auto Release(UINT index) -> void;

  [[nodiscard]] auto GetDescriptorCpuHandle(UINT descriptor_index) const -> D3D12_CPU_DESCRIPTOR_HANDLE;
  [[nodiscard]] auto GetDescriptorGpuHandle(UINT descriptor_index) const -> D3D12_GPU_DESCRIPTOR_HANDLE;

  [[nodiscard]] auto GetInternalPtr() const -> ID3D12DescriptorHeap*;

  DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap, ID3D12Device& device);
  DescriptorHeap(DescriptorHeap const&) = delete;
  DescriptorHeap(DescriptorHeap&&) = delete;

  ~DescriptorHeap() = default;

  auto operator=(DescriptorHeap const&) -> void = delete;
  auto operator=(DescriptorHeap&&) -> void = delete;

private:
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
  std::vector<UINT> free_indices_;
  std::mutex mutex_;
  UINT increment_size_;
  UINT reserved_idx_count_;
  UINT heap_size_;
};
}
