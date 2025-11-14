#pragma once

#include <span>

#include <wand/buffer.hpp>
#include <wand/descriptor_heap.hpp>
#include <wand/pipeline.hpp>
#include <wand/resource_state_tracker.hpp>
#include <wand/root_signature_cache.hpp>
#include <wand/texture.hpp>

namespace wand {
namespace details {
struct PendingBarrier {
  D3D12_BARRIER_LAYOUT layout;
  ID3D12Resource* resource;
};
}


class CommandList {
public:
  auto Begin(PipelineState const* pipeline_state) -> void;
  auto End() const -> void;
  auto ClearDepthStencil(Texture const& tex, D3D12_CLEAR_FLAGS clear_flags, FLOAT depth, UINT8 stencil,
                         std::span<D3D12_RECT const> rects, UINT16 mip_level = 0) -> void;
  auto ClearRenderTarget(Texture const& tex, std::span<FLOAT const, 4> color_rgba,
                         std::span<D3D12_RECT const> rects, UINT16 mip_level = 0) -> void;
  auto CopyBuffer(Buffer const& dst, Buffer const& src) -> void;
  auto CopyBufferRegion(Buffer const& dst, UINT64 dst_offset, Buffer const& src, UINT64 src_offset,
                        UINT64 num_bytes) -> void;
  auto CopyTexture(Texture const& dst, Texture const& src) -> void;
  auto CopyTextureRegion(Texture const& dst, UINT dst_subresource_index, UINT dst_x, UINT dst_y, UINT dst_z,
                         Texture const& src, UINT src_subresource_index, D3D12_BOX const* src_box) -> void;
  auto CopyTextureRegion(Texture const& dst, UINT dst_subresource_index, UINT dst_x, UINT dst_y, UINT dst_z,
                         Buffer const& src, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& src_footprint) -> void;
  auto DiscardRenderTarget(Texture const& tex, std::optional<D3D12_DISCARD_REGION> const& region) -> void;
  auto DiscardDepthStencil(Texture const& tex, std::optional<D3D12_DISCARD_REGION> const& region) -> void;
  auto Dispatch(UINT thread_group_count_x, UINT thread_group_count_y,
                UINT thread_group_count_z) const -> void;
  auto DispatchMesh(UINT thread_group_count_x, UINT thread_group_count_y,
                    UINT thread_group_count_z) const -> void;
  auto DrawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, UINT start_index_location,
                            INT base_vertex_location, UINT start_instance_location) const -> void;
  auto DrawInstanced(UINT vertex_count_per_instance, UINT instance_count, UINT start_vertex_location,
                     UINT start_instance_location) const -> void;
  auto Resolve(Texture const& dst, Texture const& src, DXGI_FORMAT format) -> void;
  auto SetBlendFactor(std::span<FLOAT const, 4> blend_factor) const -> void;
  auto SetIndexBuffer(Buffer const& buf, DXGI_FORMAT index_format) -> void;
  auto SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_topology) const -> void;
  auto SetRenderTargets(std::span<Texture const* const> render_targets, Texture const* depth_stencil,
                        UINT16 mip_level = 0) -> void;
  auto SetStencilRef(UINT stencil_ref) const -> void;
  auto SetScissorRects(std::span<D3D12_RECT const> rects) const -> void;
  auto SetViewports(std::span<D3D12_VIEWPORT const> viewports) const -> void;
  auto SetPipelineParameter(UINT index, UINT value) const -> void;
  auto SetPipelineParameters(UINT index, std::span<UINT const> values) const -> void;
  auto SetConstantBuffer(UINT param_idx, Buffer const& buf) -> void;
  auto SetShaderResource(UINT param_idx, Buffer const& buf) -> void;
  auto SetShaderResource(UINT param_idx, Texture const& tex) -> void;
  auto SetUnorderedAccess(UINT param_idx, Buffer const& buf) -> void;
  auto SetUnorderedAccess(UINT param_idx, Texture const& tex) -> void;
  auto SetPipelineState(PipelineState const& pipeline_state) -> void;

private:
  auto SetRootSignature(std::uint8_t num_params) const -> void;

  CommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator,
              Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmd_list, details::DescriptorHeap const* dsv_heap,
              details::DescriptorHeap const* rtv_heap, details::DescriptorHeap const* res_desc_heap,
              details::DescriptorHeap const* sampler_heap, details::RootSignatureCache* root_signatures);

  auto GenerateBarrier(Buffer const& buf, D3D12_BARRIER_SYNC sync, D3D12_BARRIER_ACCESS access) -> void;
  auto GenerateBarrier(Texture const& tex, D3D12_BARRIER_SYNC sync, D3D12_BARRIER_ACCESS access,
                       D3D12_BARRIER_LAYOUT layout) -> void;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmd_list_;
  details::PipelineResourceStateTracker local_resource_states_;
  std::vector<details::PendingBarrier> pending_barriers_;
  details::DescriptorHeap const* dsv_heap_;
  details::DescriptorHeap const* rtv_heap_;
  details::DescriptorHeap const* res_desc_heap_;
  details::DescriptorHeap const* sampler_heap_;
  details::RootSignatureCache* root_signatures_;
  bool compute_pipeline_set_{false};
  bool pipeline_allows_ds_write_{false};

  friend GraphicsDevice;
};
}
