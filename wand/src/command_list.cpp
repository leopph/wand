#include "wand/command_list.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <iterator>

#include "wand/common.hpp"

using Microsoft::WRL::ComPtr;

namespace wand {
auto CommandList::Begin(PipelineState const* pipeline_state) -> void {
  ThrowIfFailed(allocator_->Reset(), "Failed to reset command allocator.");
  ThrowIfFailed(cmd_list_->Reset(allocator_.Get(), pipeline_state ? pipeline_state->pipeline_state_.Get() : nullptr),
                "Failed to reset command list.");
  cmd_list_->SetDescriptorHeaps(2,
                                std::array{res_desc_heap_->GetInternalPtr(), sampler_heap_->GetInternalPtr()}.data());
  compute_pipeline_set_ = pipeline_state && pipeline_state->is_compute_;
  SetRootSignature(pipeline_state ? pipeline_state->num_params_ : 0);
  local_resource_states_.Clear();
  pending_barriers_.clear();
}


auto CommandList::End() const -> void {
  ThrowIfFailed(cmd_list_->Close(), "Failed to close command list.");
}


auto CommandList::ClearDepthStencil(Texture const& tex, D3D12_CLEAR_FLAGS const clear_flags, FLOAT const depth,
                                    UINT8 const stencil, std::span<D3D12_RECT const> const rects,
                                    UINT16 const mip_level) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
                  D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);

  cmd_list_->ClearDepthStencilView(dsv_heap_->GetDescriptorCpuHandle(tex.GetDepthStencilView(mip_level)), clear_flags,
                                   depth, stencil,
                                   static_cast<UINT>(rects.size()), rects.data());
}


auto CommandList::ClearRenderTarget(Texture const& tex, std::span<FLOAT const, 4> const color_rgba,
                                    std::span<D3D12_RECT const> const rects, UINT16 const mip_level) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                  D3D12_BARRIER_LAYOUT_RENDER_TARGET);

  cmd_list_->ClearRenderTargetView(rtv_heap_->GetDescriptorCpuHandle(tex.GetRenderTargetView(mip_level)),
                                   color_rgba.data(),
                                   static_cast<UINT>(rects.size()), rects.data());
}


auto CommandList::CopyBuffer(Buffer const& dst, Buffer const& src) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST);
  cmd_list_->CopyResource(dst.GetInternalResource(), src.GetInternalResource());
}


auto CommandList::CopyBufferRegion(Buffer const& dst, UINT64 const dst_offset, Buffer const& src,
                                   UINT64 const src_offset, UINT64 const num_bytes) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST);
  cmd_list_->CopyBufferRegion(dst.GetInternalResource(), dst_offset, src.GetInternalResource(), src_offset, num_bytes);
}


auto CommandList::CopyTexture(Texture const& dst, Texture const& src) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST);
  cmd_list_->CopyResource(dst.GetInternalResource(), src.GetInternalResource());
}


auto CommandList::CopyTextureRegion(Texture const& dst, UINT const dst_subresource_index, UINT const dst_x,
                                    UINT const dst_y, UINT const dst_z, Texture const& src,
                                    UINT const src_subresource_index, D3D12_BOX const* src_box) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST);

  D3D12_TEXTURE_COPY_LOCATION const dst_loc{
    .pResource = dst.GetInternalResource(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = dst_subresource_index
  };
  D3D12_TEXTURE_COPY_LOCATION const src_loc{
    .pResource = src.GetInternalResource(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = src_subresource_index
  };
  cmd_list_->CopyTextureRegion(&dst_loc, dst_x, dst_y, dst_z, &src_loc, src_box);
}


auto CommandList::CopyTextureRegion(Texture const& dst, UINT const dst_subresource_index, UINT const dst_x,
                                    UINT const dst_y, UINT const dst_z, Buffer const& src,
                                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& src_footprint) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION const dst_loc{
    .pResource = dst.GetInternalResource(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
    .SubresourceIndex = dst_subresource_index
  };
  D3D12_TEXTURE_COPY_LOCATION const src_loc{
    .pResource = src.GetInternalResource(), .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
    .PlacedFootprint = src_footprint
  };
  cmd_list_->CopyTextureRegion(&dst_loc, dst_x, dst_y, dst_z, &src_loc, nullptr);
}


auto CommandList::DiscardRenderTarget(Texture const& tex, std::optional<D3D12_DISCARD_REGION> const& region) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                  D3D12_BARRIER_LAYOUT_RENDER_TARGET);
  cmd_list_->DiscardResource(tex.GetInternalResource(), region ? &*region : nullptr);
}


auto CommandList::DiscardDepthStencil(Texture const& tex, std::optional<D3D12_DISCARD_REGION> const& region) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
                  D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
  cmd_list_->DiscardResource(tex.GetInternalResource(), region ? &*region : nullptr);
}


auto CommandList::Dispatch(UINT const thread_group_count_x, UINT const thread_group_count_y,
                           UINT const thread_group_count_z) const -> void {
  cmd_list_->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}


auto CommandList::DispatchMesh(UINT const thread_group_count_x, UINT const thread_group_count_y,
                               UINT const thread_group_count_z) const -> void {
  cmd_list_->DispatchMesh(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}


auto CommandList::DrawIndexedInstanced(UINT const index_count_per_instance, UINT const instance_count,
                                       UINT const start_index_location, INT const base_vertex_location,
                                       UINT const start_instance_location) const -> void {
  std::array const offsets{*std::bit_cast<UINT const*>(&base_vertex_location), start_instance_location};
  cmd_list_->SetGraphicsRoot32BitConstants(1, static_cast<UINT>(offsets.size()), offsets.data(), 0);
  cmd_list_->DrawIndexedInstanced(index_count_per_instance, instance_count, start_index_location, base_vertex_location,
                                  start_instance_location);
}


auto CommandList::DrawInstanced(UINT const vertex_count_per_instance, UINT const instance_count,
                                UINT const start_vertex_location, UINT const start_instance_location) const -> void {
  std::array const offsets{0u, start_instance_location};
  cmd_list_->SetGraphicsRoot32BitConstants(1, static_cast<UINT>(offsets.size()), offsets.data(), 0);
  cmd_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}


auto CommandList::Resolve(Texture const& dst, Texture const& src, DXGI_FORMAT const format) -> void {
  GenerateBarrier(src, D3D12_BARRIER_SYNC_RESOLVE, D3D12_BARRIER_ACCESS_RESOLVE_SOURCE,
                  D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE);
  GenerateBarrier(dst, D3D12_BARRIER_SYNC_RESOLVE, D3D12_BARRIER_ACCESS_RESOLVE_DEST,
                  D3D12_BARRIER_LAYOUT_RESOLVE_DEST);
  cmd_list_->ResolveSubresource(dst.GetInternalResource(), 0, src.GetInternalResource(), 0, format);
}


auto CommandList::SetBlendFactor(std::span<FLOAT const, 4> const blend_factor) const -> void {
  cmd_list_->OMSetBlendFactor(blend_factor.data());
}


auto CommandList::SetIndexBuffer(Buffer const& buf, DXGI_FORMAT const index_format) -> void {
  GenerateBarrier(buf, D3D12_BARRIER_SYNC_VERTEX_SHADING, D3D12_BARRIER_ACCESS_INDEX_BUFFER);
  D3D12_INDEX_BUFFER_VIEW const ibv{
    buf.GetInternalResource()->GetGPUVirtualAddress(), static_cast<UINT>(buf.GetInternalResource()->GetDesc1().Width),
    index_format
  };
  cmd_list_->IASetIndexBuffer(&ibv);
}


auto CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY const primitive_topology) const -> void {
  cmd_list_->IASetPrimitiveTopology(primitive_topology);
}


auto CommandList::SetRenderTargets(std::span<Texture const* const> const render_targets,
                                   Texture const* const depth_stencil, UINT16 const mip_level) -> void {
  std::ranges::for_each(render_targets, [this](Texture const* const tex) {
    if (tex) {
      GenerateBarrier(*tex, D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                      D3D12_BARRIER_LAYOUT_RENDER_TARGET);
    }
  });

  if (depth_stencil) {
    GenerateBarrier(*depth_stencil, D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                    pipeline_allows_ds_write_
                      ? D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE
                      : D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
                    pipeline_allows_ds_write_
                      ? D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE
                      : D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ);
  }

  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rt_handles;
  rt_handles.reserve(render_targets.size());
  std::ranges::transform(render_targets, std::back_inserter(rt_handles), [this, mip_level](Texture const* const tex) {
    return tex ? rtv_heap_->GetDescriptorCpuHandle(tex->GetRenderTargetView(mip_level)) : D3D12_CPU_DESCRIPTOR_HANDLE{};
  });

  auto const ds_handle{
    depth_stencil
      ? dsv_heap_->GetDescriptorCpuHandle(depth_stencil->GetDepthStencilView(mip_level))
      : D3D12_CPU_DESCRIPTOR_HANDLE{}
  };

  cmd_list_->OMSetRenderTargets(static_cast<UINT>(rt_handles.size()), rt_handles.data(), FALSE,
                                depth_stencil ? &ds_handle : nullptr);
}


auto CommandList::SetStencilRef(UINT const stencil_ref) const -> void {
  cmd_list_->OMSetStencilRef(stencil_ref);
}


auto CommandList::SetScissorRects(std::span<D3D12_RECT const> const rects) const -> void {
  cmd_list_->RSSetScissorRects(static_cast<UINT>(rects.size()), rects.data());
}


auto CommandList::SetViewports(std::span<D3D12_VIEWPORT const> const viewports) const -> void {
  cmd_list_->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}


auto CommandList::SetPipelineParameter(UINT const index, UINT const value) const -> void {
  if (compute_pipeline_set_) {
    cmd_list_->SetComputeRoot32BitConstant(0, value, index);
  } else {
    cmd_list_->SetGraphicsRoot32BitConstant(0, value, index);
  }
}


auto CommandList::SetPipelineParameters(UINT const index, std::span<UINT const> const values) const -> void {
  if (compute_pipeline_set_) {
    cmd_list_->SetComputeRoot32BitConstants(0, static_cast<UINT>(values.size()), values.data(), index);
  } else {
    cmd_list_->SetGraphicsRoot32BitConstants(0, static_cast<UINT>(values.size()), values.data(), index);
  }
}


auto CommandList::SetConstantBuffer(UINT const param_idx, Buffer const& buf) -> void {
  GenerateBarrier(buf, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER);
  SetPipelineParameter(param_idx, buf.GetConstantBuffer());
}


auto CommandList::SetShaderResource(UINT const param_idx, Buffer const& buf) -> void {
  GenerateBarrier(buf, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE);
  SetPipelineParameter(param_idx, buf.GetShaderResource());
}


auto CommandList::SetShaderResource(UINT const param_idx, Texture const& tex) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE);
  SetPipelineParameter(param_idx, tex.GetShaderResource());
}


auto CommandList::SetUnorderedAccess(UINT const param_idx, Buffer const& buf) -> void {
  GenerateBarrier(buf, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
  SetPipelineParameter(param_idx, buf.GetUnorderedAccess());
}


auto CommandList::SetUnorderedAccess(UINT const param_idx, Texture const& tex) -> void {
  GenerateBarrier(tex, D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
                  D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS);
  SetPipelineParameter(param_idx, tex.GetUnorderedAccess());
}


auto CommandList::SetPipelineState(PipelineState const& pipeline_state) -> void {
  cmd_list_->SetPipelineState(pipeline_state.pipeline_state_.Get());
  compute_pipeline_set_ = pipeline_state.is_compute_;
  pipeline_allows_ds_write_ = pipeline_state.allows_ds_write_;
  SetRootSignature(pipeline_state.num_params_);
}


auto CommandList::SetRtState(RtStateObject const& rt_state) -> void {
  cmd_list_->SetPipelineState1(rt_state.state_object_.Get());
  compute_pipeline_set_ = true;
  pipeline_allows_ds_write_ = false;
  SetRootSignature(rt_state.num_params_);
}


auto CommandList::SetRootSignature(std::uint8_t const num_params) const -> void {
  if (compute_pipeline_set_) {
    cmd_list_->SetComputeRootSignature(root_signatures_->Get(num_params).Get());
  } else {
    cmd_list_->SetGraphicsRootSignature(root_signatures_->Get(num_params).Get());
  }
}


CommandList::CommandList(ComPtr<ID3D12CommandAllocator> allocator, ComPtr<ID3D12GraphicsCommandList7> cmd_list,
                         details::DescriptorHeap const* dsv_heap, details::DescriptorHeap const* rtv_heap,
                         details::DescriptorHeap const* res_desc_heap, details::DescriptorHeap const* sampler_heap,
                         details::RootSignatureCache* root_signatures) :
  allocator_{std::move(allocator)},
  cmd_list_{std::move(cmd_list)},
  dsv_heap_{dsv_heap},
  rtv_heap_{rtv_heap},
  res_desc_heap_{res_desc_heap},
  sampler_heap_{sampler_heap},
  root_signatures_{root_signatures} {
}


auto CommandList::GenerateBarrier(Buffer const& buf, D3D12_BARRIER_SYNC const sync,
                                  D3D12_BARRIER_ACCESS const access) -> void {
  auto const local_state{local_resource_states_.Get(buf.GetInternalResource())};
  auto const needs_barrier{local_state && (local_state->access & access) == 0};

  if (!local_state || needs_barrier) {
    local_resource_states_.Record(buf.GetInternalResource(), {
                                    .sync = sync, .access = access, .layout = D3D12_BARRIER_LAYOUT_UNDEFINED
                                  });
  }

  if (needs_barrier) {
    D3D12_BUFFER_BARRIER const barrier{
      local_state->sync, sync, local_state->access, access, buf.GetInternalResource(), 0, UINT64_MAX
    };
    D3D12_BARRIER_GROUP const group{.Type = D3D12_BARRIER_TYPE_BUFFER, .NumBarriers = 1, .pBufferBarriers = &barrier};
    cmd_list_->Barrier(1, &group);
  }
}


auto CommandList::GenerateBarrier(Texture const& tex, D3D12_BARRIER_SYNC const sync, D3D12_BARRIER_ACCESS const access,
                                  D3D12_BARRIER_LAYOUT const layout) -> void {
  auto const local_state{local_resource_states_.Get(tex.GetInternalResource())};
  auto const needs_barrier{local_state && ((local_state->layout & layout) == 0 || (local_state->access & access) == 0)};

  if (!local_state) {
    pending_barriers_.emplace_back(layout, tex.GetInternalResource());
  }

  if (!local_state || needs_barrier) {
    local_resource_states_.Record(tex.GetInternalResource(), {.sync = sync, .access = access, .layout = layout});
  }

  if (needs_barrier) {
    D3D12_TEXTURE_BARRIER const barrier{
      local_state->sync, sync, local_state->access, access, local_state->layout, layout, tex.GetInternalResource(), {
        .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0, .NumArraySlices = 0,
        .FirstPlane = 0, .NumPlanes = 0
      },
      D3D12_TEXTURE_BARRIER_FLAG_NONE
    };
    D3D12_BARRIER_GROUP const group{.Type = D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = 1, .pTextureBarriers = &barrier};
    cmd_list_->Barrier(1, &group);
  }
}
}
