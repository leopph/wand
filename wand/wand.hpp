#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>

#include <array>
#include <deque>

namespace wand {
  enum ResourceViewFlags {
    RTV_BIT = 1 << 0,
    DSV_BIT = 1 << 1,
    SRV_BIT = 1 << 2,
    UAV_BIT = 1 << 3,
    CBV_BIT = 1 << 4,
  };


  class Texture {
  public:
    enum class Dimension {
      e1D,
      e2D,
      e3D
    };


    UINT width;
    UINT height;
    UINT depth;
    UINT arraySize;
    UINT mips;
    Dimension dim;

    int rtvIdx;
    int dsvIdx;
    int srvIdx;
    int uavIdx;

    D3D12MA::Allocation* alloc;

    DXGI_FORMAT format;
  };


  class Buffer {
  public:
    UINT size;

    int srvIdx;
    int uavIdx;
    int cbvIdx;

    D3D12MA::Allocation* alloc;
  };


  class GraphicsDevice {
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
    GraphicsDevice();
  };
}
