#ifdef _WIN64

#include "graphics_device_d3d12.hpp"

#include <cassert>

namespace wand {
  auto GraphicsDeviceD3D12::SignalAndWaitFence(ID3D12Fence* fence, UINT64 const signalValue, UINT64 const waitValue) const noexcept -> void {
    [[maybe_unused]] auto hr{mCmdQueue->Signal(fence, signalValue)};
    assert(SUCCEEDED(hr));

    if (fence->GetCompletedValue() < waitValue) {
      hr = fence->SetEventOnCompletion(waitValue, nullptr);
      assert(SUCCEEDED(hr));
    }
  }

  auto GraphicsDeviceD3D12::WaitForAllFrames() noexcept -> void {
    auto const signalValue{++mFrameFenceVal};
    auto const waitValue{signalValue};
    SignalAndWaitFence(mFrameFence.Get(), signalValue, waitValue);
  }

  auto GraphicsDeviceD3D12::WaitForInFlightFrameLimit() noexcept -> void {
    auto const signalValue{++mFrameFenceVal};
    auto const waitValue{signalValue - MAX_FRAMES_IN_FLIGHT + 1};
    SignalAndWaitFence(mFrameFence.Get(), signalValue, waitValue);
  }

  GraphicsDeviceD3D12::GraphicsDeviceD3D12(HWND const hwnd) {
    using Microsoft::WRL::ComPtr;

    [[maybe_unused]] HRESULT hr;

#ifndef NDEBUG
    {
      ComPtr<ID3D12Debug5> debug;
      hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
      assert(SUCCEEDED(hr));

      debug->EnableDebugLayer();
    }
#endif

    UINT dxgiFactoryFlags{0};
#ifndef NDEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory));
    assert(SUCCEEDED(hr));

    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice));
    assert(SUCCEEDED(hr));

#ifndef NDEBUG
    {
      ComPtr<ID3D12InfoQueue> infoQueue;
      hr = mDevice.As(&infoQueue);
      assert(SUCCEEDED(hr));

      hr = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
      assert(SUCCEEDED(hr));

      hr = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      assert(SUCCEEDED(hr));
    }
#endif

    D3D12_COMMAND_QUEUE_DESC constexpr commandQueueDesc{
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0
    };

    hr = mDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCmdQueue));
    assert(SUCCEEDED(hr));

    for (auto i{0}; i < MAX_FRAMES_IN_FLIGHT; i++) {
      hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocs[i]));
      assert(SUCCEEDED(hr));

      hr = mDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCmdLists[i]));
      assert(SUCCEEDED(hr));
    }

    hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFrameFence.GetAddressOf()));
    assert(SUCCEEDED(hr));

    D3D12_DESCRIPTOR_HEAP_DESC constexpr resDescHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = RES_DESC_HEAP_SIZE,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    hr = mDevice->CreateDescriptorHeap(&resDescHeapDesc, IID_PPV_ARGS(&mResDescHeap));
    assert(SUCCEEDED(hr));

    D3D12_DESCRIPTOR_HEAP_DESC constexpr rtvDescHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = RTV_HEAP_SIZE,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    hr = mDevice->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&mRtvHeap));
    assert(SUCCEEDED(hr));

    D3D12_DESCRIPTOR_HEAP_DESC constexpr dsvDescHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
      .NumDescriptors = DSV_HEAP_SIZE,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    hr = mDevice->CreateDescriptorHeap(&dsvDescHeapDesc, IID_PPV_ARGS(&mDsvHeap));
    assert(SUCCEEDED(hr));

    for (auto i{0}; i < RES_DESC_HEAP_SIZE; i++) {
      mResDescFreeIndices.emplace_back(i);
    }

    for (auto i{0}; i < RTV_HEAP_SIZE; i++) {
      mRtvFreeIndices.emplace_back(i);
    }

    for (auto i{0}; i < DSV_HEAP_SIZE; i++) {
      mDsvFreeIndices.emplace_back(i);
    }
  }
}

#endif
