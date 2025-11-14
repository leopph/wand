#include "wand/descriptor_heap.hpp"

#include <algorithm>

#include "wand/common.hpp"

using Microsoft::WRL::ComPtr;

namespace wand::details {
auto DescriptorHeap::Allocate() -> UINT {
  std::scoped_lock const lock{mutex_};

  if (free_indices_.empty()) {
    auto const old_reserve_count{reserved_idx_count_};
    reserved_idx_count_ = std::min(heap_size_, reserved_idx_count_ * 2);

    if (old_reserve_count == reserved_idx_count_) {
      throw std::runtime_error{"Failed to allocate descriptor heap indices: the heap is full."};
    }

    free_indices_.reserve(reserved_idx_count_ - old_reserve_count);

    for (UINT idx{old_reserve_count}; idx < reserved_idx_count_; idx++) {
      free_indices_.emplace_back(idx);
    }

    // TODO start descriptor heap itself small, grow it on demand
  }

  auto const idx{free_indices_.back()};
  free_indices_.pop_back();
  return idx;
}


auto DescriptorHeap::Release(UINT const index) -> void {
  if (index == kInvalidResourceIndex) {
    return;
  }

  std::scoped_lock const lock{mutex_};
  free_indices_.insert(std::ranges::upper_bound(free_indices_, index), index);
  free_indices_.erase(std::ranges::unique(free_indices_).begin(), free_indices_.end());
}


auto DescriptorHeap::GetDescriptorCpuHandle(UINT const descriptor_index) const -> D3D12_CPU_DESCRIPTOR_HANDLE {
  if (descriptor_index >= reserved_idx_count_) {
    throw std::runtime_error{"Failed to convert descriptor index to CPU handle: descriptor index is out of range."};
  }

  return CD3DX12_CPU_DESCRIPTOR_HANDLE{
    heap_->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(descriptor_index), increment_size_
  };
}


auto DescriptorHeap::GetDescriptorGpuHandle(UINT const descriptor_index) const -> D3D12_GPU_DESCRIPTOR_HANDLE {
  if (descriptor_index >= reserved_idx_count_) {
    throw std::runtime_error{"Failed to convert descriptor index to GPU handle: descriptor index is out of range."};
  }

  return CD3DX12_GPU_DESCRIPTOR_HANDLE{
    heap_->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(descriptor_index), increment_size_
  };
}


auto DescriptorHeap::GetInternalPtr() const -> ID3D12DescriptorHeap* {
  return heap_.Get();
}


DescriptorHeap::DescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap, ID3D12Device& device) :
  heap_{std::move(heap)},
  increment_size_{device.GetDescriptorHandleIncrementSize(heap_->GetDesc().Type)},
  reserved_idx_count_{1},
  heap_size_{heap_->GetDesc().NumDescriptors} {
  free_indices_.reserve(reserved_idx_count_);
  for (UINT idx{0}; idx < reserved_idx_count_; idx++) {
    free_indices_.emplace_back(idx);
  }
}
}
