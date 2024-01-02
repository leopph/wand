#pragma once

#include "platform_d3d12.hpp"
#include "graphics_device.hpp"

#include <array>
#include <deque>

namespace wand {
  class GraphicsDeviceD3D12 : public GraphicsDevice {
    constexpr static auto MAX_FRAMES_IN_FLIGHT{2};
    constexpr static auto RES_DESC_HEAP_SIZE{1'000'000};
    constexpr static auto RTV_HEAP_SIZE{1'000'000};
    constexpr static auto DSV_HEAP_SIZE{1'000'000};

    Microsoft::WRL::ComPtr<IDXGIFactory7> mFactory;
    Microsoft::WRL::ComPtr<ID3D12Device9> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCmdQueue;

    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, MAX_FRAMES_IN_FLIGHT> mCmdAllocs;
    std::array<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>, MAX_FRAMES_IN_FLIGHT> mCmdLists;

    UINT64 mFrameFenceVal{MAX_FRAMES_IN_FLIGHT - 1};
    Microsoft::WRL::ComPtr<ID3D12Fence1> mFrameFence;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mResDescHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    std::deque<int> mResDescFreeIndices;
    std::deque<int> mRtvFreeIndices;
    std::deque<int> mDsvFreeIndices;

    auto SignalAndWaitFence(ID3D12Fence* fence, UINT64 signalValue, UINT64 waitValue) const noexcept -> void;
    auto WaitForAllFrames() noexcept -> void;
    auto WaitForInFlightFrameLimit() noexcept -> void;

  public:
    explicit GraphicsDeviceD3D12(HWND hwnd);
  };
}
