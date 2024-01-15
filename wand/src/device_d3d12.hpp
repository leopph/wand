#pragma once

#include "platform_d3d12.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

namespace wand {
class DeviceD3D12 final : public Device {
  friend class BufferD3D12;
  friend class TextureD3D12;

  constexpr static auto max_frames_in_flight_{2};
  constexpr static auto swap_chain_buffer_count_{2};
  constexpr static auto swap_chain_format_{DXGI_FORMAT_R8G8B8A8_UNORM};
  constexpr static auto res_desc_heap_size_{1'000'000};
  constexpr static auto rtv_heap_size_{1'000'000};
  constexpr static auto dsv_heap_size_{1'000'000};
public:
  constexpr static auto invalid_descriptor_idx_{std::numeric_limits<std::uint32_t>::max()};

private:
  Microsoft::WRL::ComPtr<IDXGIFactory7> factory_;
  Microsoft::WRL::ComPtr<ID3D12Device9> device_;
  Microsoft::WRL::ComPtr<D3D12MA::Allocator> allocator_;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> direct_queue_;

  std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, max_frames_in_flight_> direct_command_allocators_;
  std::array<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>, max_frames_in_flight_> direct_command_lists_;

  Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain_;

  UINT64 frame_fence_value_{max_frames_in_flight_ - 1};
  Microsoft::WRL::ComPtr<ID3D12Fence1> frame_fence_;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> resource_descriptor_heap_;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap_;

  using ResViewIdxType = std::uint32_t;
  using RtvIdxType = std::uint32_t;
  using DsvIdxType = std::uint32_t;

  std::mutex resource_desc_heap_index_mutex_;
  std::vector<ResViewIdxType> resource_descriptor_heap_free_indices_;

  std::mutex rtv_heap_index_mutex_;
  std::vector<RtvIdxType> rtv_heap_free_indices_;

  std::mutex dsv_heap_index_mutex_;
  std::vector<DsvIdxType> dsv_heap_free_indices_;

  auto SignalAndWaitFence(ID3D12Fence* fence, UINT64 signal_value, UINT64 wait_value) const noexcept -> void;
  auto WaitForAllFrames() noexcept -> void;
  auto WaitForInFlightFrameLimit() noexcept -> void;

  [[nodiscard]] auto CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC const& cbv_desc) -> ResViewIdxType;
  auto ReleaseConstantBufferView(ResViewIdxType idx) -> void;
  [[nodiscard]] auto CreateShaderResourceView(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const& srv_desc) -> ResViewIdxType;
  auto ReleaseShaderResourceView(ResViewIdxType idx) -> void;
  [[nodiscard]] auto CreateUnorderedAccessView(ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uav_desc) -> ResViewIdxType;
  auto ReleaseUnorderedAccessView(ResViewIdxType idx) -> void;
  [[nodiscard]] auto CreateRenderTargetView(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC const& rtv_desc) -> RtvIdxType;
  auto ReleaseRenderTargetView(RtvIdxType idx) -> void;
  [[nodiscard]] auto CreateDepthStencilView(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC const& dsv_desc) -> DsvIdxType;
  auto ReleaseDepthStencilView(DsvIdxType idx) -> void;

public:
  explicit DeviceD3D12(HWND hwnd);

  [[nodiscard]] auto CreateBuffer(Buffer::Desc const& desc) -> std::unique_ptr<Buffer> override;
  [[nodiscard]] auto CreateTexture(Texture::Desc const& desc) -> std::unique_ptr<Texture> override;
};
}
