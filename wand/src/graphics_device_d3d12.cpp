#ifdef _WIN64

#include "graphics_device_d3d12.hpp"

#include <cassert>

namespace wand {
auto GraphicsDeviceD3D12::SignalAndWaitFence(ID3D12Fence* fence, UINT64 const signal_value, UINT64 const wait_value) const noexcept -> void {
  [[maybe_unused]] auto hr{direct_queue_->Signal(fence, signal_value)};
  assert(SUCCEEDED(hr));

  if (fence->GetCompletedValue() < wait_value) {
    hr = fence->SetEventOnCompletion(wait_value, nullptr);
    assert(SUCCEEDED(hr));
  }
}

auto GraphicsDeviceD3D12::WaitForAllFrames() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

auto GraphicsDeviceD3D12::WaitForInFlightFrameLimit() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value - max_frames_in_flight_ + 1};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

GraphicsDeviceD3D12::GraphicsDeviceD3D12(HWND const hwnd) {
  using Microsoft::WRL::ComPtr;

  [[maybe_unused]] HRESULT hr;

#ifndef NDEBUG
  ComPtr<ID3D12Debug5> debug;
  hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
  assert(SUCCEEDED(hr));
  debug->EnableDebugLayer();
#endif

  UINT factory_create_flags{0};
#ifndef NDEBUG
  factory_create_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  hr = CreateDXGIFactory2(factory_create_flags, IID_PPV_ARGS(&factory_));
  assert(SUCCEEDED(hr));

  hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device_));
  assert(SUCCEEDED(hr));

#ifndef NDEBUG
  ComPtr<ID3D12InfoQueue> info_queue;
  hr = device_.As(&info_queue);
  assert(SUCCEEDED(hr));
  hr = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
  assert(SUCCEEDED(hr));
  hr = info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
  assert(SUCCEEDED(hr));
#endif

  D3D12_COMMAND_QUEUE_DESC constexpr direct_queue_desc{
    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    .NodeMask = 0
  };

  hr = device_->CreateCommandQueue(&direct_queue_desc, IID_PPV_ARGS(&direct_queue_));
  assert(SUCCEEDED(hr));

  for (auto i{0}; i < max_frames_in_flight_; i++) {
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct_command_allocators_[i]));
    assert(SUCCEEDED(hr));

    hr = device_->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&direct_command_lists_[i]));
    assert(SUCCEEDED(hr));
  }

  DXGI_SWAP_CHAIN_DESC1 constexpr swap_chain_desc{
    .Width = 0,
    .Height = 0,
    .Format = swap_chain_format_,
    .Stereo = FALSE,
    .SampleDesc = {.Count = 1, .Quality = 0},
    .BufferUsage = 0,
    .BufferCount = swap_chain_buffer_count_,
    .Scaling = DXGI_SCALING_NONE,
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
    .Flags = 0
  };

  ComPtr<IDXGISwapChain1> swap_chain;
  hr = factory_->CreateSwapChainForHwnd(direct_queue_.Get(), hwnd, &swap_chain_desc, nullptr, nullptr, &swap_chain);
  assert(SUCCEEDED(hr));
  hr = swap_chain.As(&swap_chain_);
  assert(SUCCEEDED(hr));

  hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frame_fence_.GetAddressOf()));
  assert(SUCCEEDED(hr));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr resource_descriptor_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = res_desc_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    .NodeMask = 0
  };

  hr = device_->CreateDescriptorHeap(&resource_descriptor_heap_desc, IID_PPV_ARGS(&resource_descriptor_heap_));
  assert(SUCCEEDED(hr));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr rtv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    .NumDescriptors = rtv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    .NodeMask = 0
  };

  hr = device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_));
  assert(SUCCEEDED(hr));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr dsv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
    .NumDescriptors = dsv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    .NodeMask = 0
  };

  hr = device_->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap_));
  assert(SUCCEEDED(hr));

  for (auto i{0}; i < res_desc_heap_size_; i++) {
    resource_descriptor_heap_free_indices_.emplace_back(i);
  }

  for (auto i{0}; i < rtv_heap_size_; i++) {
    rtv_heap_free_indices_.emplace_back(i);
  }

  for (auto i{0}; i < dsv_heap_size_; i++) {
    dsv_heap_free_indices_.emplace_back(i);
  }
}
}

#endif
