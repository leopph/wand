#ifdef _WIN64

#include "device_d3d12.hpp"
#include "buffer_d3d12.hpp"

#include <cassert>
#include <exception>
#include <stdexcept>
#include <utility>

namespace {
auto ThrowIfFailed(HRESULT const hr) -> void {
  if (FAILED(hr)) {
    throw std::exception{};
  }
}
}

namespace wand {
auto DeviceD3D12::SignalAndWaitFence(ID3D12Fence* fence, UINT64 const signal_value, UINT64 const wait_value) const noexcept -> void {
  ThrowIfFailed(direct_queue_->Signal(fence, signal_value));
  if (fence->GetCompletedValue() < wait_value) {
    ThrowIfFailed(fence->SetEventOnCompletion(wait_value, nullptr));
  }
}

auto DeviceD3D12::WaitForAllFrames() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

auto DeviceD3D12::WaitForInFlightFrameLimit() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value - max_frames_in_flight_ + 1};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

DeviceD3D12::DeviceD3D12(HWND const hwnd) {
  using Microsoft::WRL::ComPtr;

#ifndef NDEBUG
  ComPtr<ID3D12Debug5> debug;
  ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
  debug->EnableDebugLayer();
#endif

  UINT factory_create_flags{0};
#ifndef NDEBUG
  factory_create_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  ThrowIfFailed(CreateDXGIFactory2(factory_create_flags, IID_PPV_ARGS(&factory_)));
  ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)));

#ifndef NDEBUG
  ComPtr<ID3D12InfoQueue> info_queue;
  ThrowIfFailed(device_.As(&info_queue));
  ThrowIfFailed(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
  ThrowIfFailed(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
#endif

  CD3DX12FeatureSupport feature_support;
  ThrowIfFailed(feature_support.Init(device_.Get()));

  if (feature_support.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3 || feature_support.HighestShaderModel() < D3D_SHADER_MODEL_6_6) {
    throw std::runtime_error{"Unsuppported hardware."};
  }

  ComPtr<IDXGIAdapter> adapter;
  ThrowIfFailed(factory_->EnumAdapterByLuid(device_->GetAdapterLuid(), IID_PPV_ARGS(&adapter)));

  D3D12MA::ALLOCATOR_DESC const allocator_desc{
    .Flags = D3D12MA::ALLOCATOR_FLAG_NONE,
    .pDevice = device_.Get(),
    .PreferredBlockSize = 0,
    .pAllocationCallbacks = nullptr,
    .pAdapter = adapter.Get()
  };

  ThrowIfFailed(CreateAllocator(&allocator_desc, &allocator_));

  D3D12_COMMAND_QUEUE_DESC constexpr direct_queue_desc{
    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    .NodeMask = 0
  };

  ThrowIfFailed(device_->CreateCommandQueue(&direct_queue_desc, IID_PPV_ARGS(&direct_queue_)));

  for (auto i{0}; i < max_frames_in_flight_; i++) {
    ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct_command_allocators_[i])));
    ThrowIfFailed(device_->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&direct_command_lists_[i])));
  }

  DXGI_SWAP_CHAIN_DESC1 constexpr swap_chain_desc{
    .Width = 0,
    .Height = 0,
    .Format = swap_chain_format_,
    .Stereo = FALSE,
    .SampleDesc = {
      .Count = 1,
      .Quality = 0
    },
    .BufferUsage = 0,
    .BufferCount = swap_chain_buffer_count_,
    .Scaling = DXGI_SCALING_NONE,
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
    .Flags = 0
  };

  ComPtr<IDXGISwapChain1> swap_chain;
  ThrowIfFailed(factory_->CreateSwapChainForHwnd(direct_queue_.Get(), hwnd, &swap_chain_desc, nullptr, nullptr, &swap_chain));
  ThrowIfFailed(swap_chain.As(&swap_chain_));

  ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frame_fence_.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr resource_descriptor_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = res_desc_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    .NodeMask = 0
  };

  ThrowIfFailed(device_->CreateDescriptorHeap(&resource_descriptor_heap_desc, IID_PPV_ARGS(&resource_descriptor_heap_)));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr rtv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    .NumDescriptors = rtv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    .NodeMask = 0
  };

  ThrowIfFailed(device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_)));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr dsv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
    .NumDescriptors = dsv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    .NodeMask = 0
  };

  ThrowIfFailed(device_->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap_)));

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

auto DeviceD3D12::CreateBuffer(Buffer::Desc const& desc) -> std::unique_ptr<Buffer> {
  D3D12MA::ALLOCATION_DESC const allocation_desc{
    .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
    .HeapType = desc.mappable ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
    .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE,
    .CustomPool = nullptr,
    .pPrivateData = nullptr
  };

  auto const buffer_desc{CD3DX12_RESOURCE_DESC1::Buffer(desc.width)};

  Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
  ThrowIfFailed(allocator_->CreateResource2(&allocation_desc, &buffer_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, &allocation, IID_NULL, nullptr));

  std::uint32_t cbv_idx{invalid_descriptor_idx_};
  std::uint32_t srv_idx{invalid_descriptor_idx_};
  std::uint32_t uav_idx{invalid_descriptor_idx_};

  if (desc.usage & Buffer::kUsageConstantBuffer) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC const cbv_desc{
      .BufferLocation = allocation->GetResource()->GetGPUVirtualAddress(),
      .SizeInBytes = desc.width
    };

    cbv_idx = resource_descriptor_heap_free_indices_.front();
    resource_descriptor_heap_free_indices_.pop_back();
    device_->CreateConstantBufferView(&cbv_desc, CD3DX12_CPU_DESCRIPTOR_HANDLE{resource_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(cbv_idx), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)});
  }

  if (desc.usage & Buffer::kUsageShaderResource) {
    D3D12_SHADER_RESOURCE_VIEW_DESC const srv_desc{
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Buffer = {
        .FirstElement = 0,
        .NumElements = desc.width / desc.stride,
        .StructureByteStride = desc.stride,
        .Flags = D3D12_BUFFER_SRV_FLAG_NONE
      }
    };

    srv_idx = resource_descriptor_heap_free_indices_.front();
    resource_descriptor_heap_free_indices_.pop_front();
    device_->CreateShaderResourceView(allocation->GetResource(), &srv_desc, CD3DX12_CPU_DESCRIPTOR_HANDLE{resource_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(srv_idx), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)});
  }

  if (desc.usage & Buffer::kUsageUnorderedAccess) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC const uav_desc{
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
      .Buffer = {
        .FirstElement = 0,
        .NumElements = desc.width / desc.stride,
        .StructureByteStride = desc.stride,
        .CounterOffsetInBytes = 0,
        .Flags = D3D12_BUFFER_UAV_FLAG_NONE
      }
    };

    uav_idx = resource_descriptor_heap_free_indices_.front();
    resource_descriptor_heap_free_indices_.pop_front();
    device_->CreateUnorderedAccessView(allocation->GetResource(), nullptr, &uav_desc, CD3DX12_CPU_DESCRIPTOR_HANDLE{resource_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(uav_idx), device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)});
  }

  return std::make_unique<BufferD3D12>(desc, std::move(allocation), cbv_idx, srv_idx, uav_idx);
}

auto DeviceD3D12::CreateTexture(Texture::Desc const& desc) -> std::unique_ptr<Texture> {
  D3D12MA::ALLOCATION_DESC constexpr allocation_desc{
    .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
    .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE,
    .CustomPool = nullptr,
    .pPrivateData = nullptr
  };

  auto const texture_desc{
    [&desc] {
      // TODO
    }
  };

  return nullptr;
}
}

#endif
