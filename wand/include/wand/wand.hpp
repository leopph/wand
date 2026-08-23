#pragma once

#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

#include <wand/buffer.hpp>
#include <wand/command_list.hpp>
#include <wand/device_child.hpp>
#include <wand/fence.hpp>
#include <wand/pipeline.hpp>
#include <wand/rt_state_object.hpp>
#include <wand/sampler.hpp>
#include <wand/swapchain.hpp>
#include <wand/texture.hpp>
#include <wand/detail/descriptor_heap.hpp>
#include <wand/detail/resource_state_tracker.hpp>
#include <wand/detail/root_signature_cache.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
enum class CpuAccess : std::uint8_t {
  kNone  = 0,
  kRead  = 1,
  kWrite = 2
};


struct AliasedTextureCreateInfo {
  TextureDesc desc;
  D3D12_BARRIER_LAYOUT initial_layout;
  D3D12_CLEAR_VALUE* clear_value;
};


namespace details {
struct ExecuteBarrierCmdListRecord {
  SharedDeviceChildHandle<CommandList> cmd_list;
  UINT64 fence_completion_val;
};
}


class GraphicsDevice {
public:
  explicit WANDAPI GraphicsDevice(bool enable_debug, bool use_sw_rendering);
  GraphicsDevice(GraphicsDevice const&) = delete;
  GraphicsDevice(GraphicsDevice&&) = delete;

  WANDAPI ~GraphicsDevice() = default;

  auto operator=(GraphicsDevice const&) -> void = delete;
  auto operator=(GraphicsDevice&&) -> void = delete;

  [[nodiscard]] WANDAPI
  auto CreateBuffer(BufferDesc const& desc,
                    CpuAccess cpu_access) -> SharedDeviceChildHandle<Buffer>;

  [[nodiscard]] WANDAPI
  auto CreateTexture(TextureDesc const& desc, CpuAccess cpu_access,
                     D3D12_CLEAR_VALUE const* clear_value) -> SharedDeviceChildHandle<Texture>;

  [[nodiscard]] WANDAPI
  auto CreatePipelineState(PipelineDesc const& desc,
                           std::uint8_t num_32_bit_params) -> SharedDeviceChildHandle<PipelineState>;

  [[nodiscard]] WANDAPI
  auto CreateRtStateObject(RtStateObjectDesc& desc,
                           std::uint8_t num_32_bit_params) -> SharedDeviceChildHandle<RtStateObject>;

  [[nodiscard]]WANDAPI
  auto CreateCommandList() -> SharedDeviceChildHandle<CommandList>;

  [[nodiscard]]WANDAPI
  auto CreateFence(UINT64 initial_value) -> SharedDeviceChildHandle<Fence>;

  [[nodiscard]]WANDAPI
  auto CreateSwapChain(SwapChainDesc const& desc,
                       HWND window_handle) -> SharedDeviceChildHandle<SwapChain>;

  [[nodiscard]] WANDAPI
  auto CreateSampler(D3D12_SAMPLER_DESC const& desc) -> UniqueSamplerHandle;

  WANDAPI
  auto CreateAliasingResources(std::span<BufferDesc const> buffer_descs,
                               std::span<AliasedTextureCreateInfo const> texture_infos, CpuAccess cpu_access,
                               std::vector<SharedDeviceChildHandle<Buffer>>* buffers,
                               std::vector<SharedDeviceChildHandle<Texture>>* textures) -> void;

  WANDAPI
  auto DestroyBuffer(Buffer const* buffer) const -> void;

  WANDAPI
  auto DestroyTexture(Texture const* texture) const -> void;

  WANDAPI
  auto DestroyPipelineState(PipelineState const* pipeline_state) const -> void;

  WANDAPI
  auto DestroyRtStateObject(RtStateObject const* rt_state_object) const -> void;

  WANDAPI
  auto DestroyCommandList(CommandList const* command_list) const -> void;

  WANDAPI
  auto DestroyFence(Fence const* fence) const -> void;

  WANDAPI
  auto DestroySwapChain(SwapChain const* swap_chain) const -> void;

  WANDAPI
  auto DestroySampler(UINT sampler) const -> void;

  WANDAPI
  auto WaitFence(Fence const& fence, UINT64 wait_value) const -> void;

  WANDAPI
  auto SignalFence(Fence& fence) const -> void;

  WANDAPI
  auto ExecuteCommandLists(std::span<CommandList const> cmd_lists) -> void;

  WANDAPI
  auto WaitIdle() const -> void;

  WANDAPI
  auto ResizeSwapChain(SwapChain& swap_chain, UINT width, UINT height) -> void;

  WANDAPI
  auto Present(SwapChain const& swap_chain) -> void;

  WANDAPI
  auto GetCopyableFootprints(TextureDesc const& desc, UINT first_subresource, UINT subresource_count,
                             UINT64 base_offset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts,
                             UINT* row_counts, UINT64* row_sizes, UINT64* total_size) const -> void;

  [[nodiscard]] WANDAPI
  auto GetRtAccelerationStructurePrebuildInfo(
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS const& inputs) const ->
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO;

private:
  auto SwapChainCreateTextures(SwapChain& swap_chain) -> void;

  auto CreateBufferViews(ID3D12Resource2& buffer, BufferDesc const& desc, UINT& cbv, UINT& srv,
                         UINT& uav) const -> void;
  auto CreateTextureViews(ID3D12Resource2& texture, TextureDesc const& desc, std::vector<UINT>& dsvs,
                          std::vector<UINT>& rtvs, std::optional<UINT>& srv,
                          std::optional<UINT>& uav) const -> void;

  [[nodiscard]] auto AcquirePendingBarrierCmdList() -> CommandList&;

  [[nodiscard]] auto MakeHeapType(CpuAccess cpu_access) const -> D3D12_HEAP_TYPE;

  [[nodiscard]] auto GetOrCreateRootSignature(
    std::uint8_t num_32_bit_params) -> Microsoft::WRL::ComPtr<ID3D12RootSignature>;

  static UINT const rtv_heap_size_;
  static UINT const dsv_heap_size_;
  static UINT const res_desc_heap_size_;
  static UINT const sampler_heap_size_;

  Microsoft::WRL::ComPtr<IDXGIFactory7> factory_;
  Microsoft::WRL::ComPtr<ID3D12Device10> device_;
  Microsoft::WRL::ComPtr<D3D12MA::Allocator> allocator_;

  std::unique_ptr<detail::DescriptorHeap> rtv_heap_;
  std::unique_ptr<detail::DescriptorHeap> dsv_heap_;
  std::unique_ptr<detail::DescriptorHeap> res_desc_heap_;
  std::unique_ptr<detail::DescriptorHeap> sampler_heap_;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;

  detail::RootSignatureCache root_signatures_;
  detail::GlobalResourceStateTracker global_resource_states_;

  UINT swap_chain_flags_{0};
  UINT present_flags_{0};

  SharedDeviceChildHandle<Fence> idle_fence_;
  SharedDeviceChildHandle<Fence> execute_barrier_fence_;

  std::vector<details::ExecuteBarrierCmdListRecord> execute_barrier_cmd_lists_;
  std::mutex execute_barrier_mutex_;

  CD3DX12FeatureSupport supported_features_;
};
}
