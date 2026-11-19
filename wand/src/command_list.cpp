#include "wand/command_list.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <ranges>
#include <utility>

#include "wand/common.hpp"
#include "wand/flags.hpp"
#include "wand/util.hpp"
#include "wand/detail/as_utils.hpp"
#include "wand/detail/buffer_utils.hpp"
#include "wand/platforms/detail/d3d12_internals.hpp"

using Microsoft::WRL::ComPtr;


namespace wand {
namespace {
constexpr auto kSealedLayout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
constexpr auto kMaxEvenNameLength = 30uz;
}


auto CommandList::Begin(PipelineState const* pipeline_state) -> void {
  ThrowIfFailed(allocator_->Reset(), "Failed to reset command allocator.");
  ThrowIfFailed(cmd_list_->Reset(allocator_.Get(), pipeline_state ? pipeline_state->pipeline_state_.Get() : nullptr),
    "Failed to reset command list.");
  cmd_list_->SetDescriptorHeaps(2,
    std::array{res_desc_heap_->GetInternalPtr(), sampler_heap_->GetInternalPtr()}.data());
  compute_pipeline_set_ = pipeline_state && pipeline_state->is_compute_;
  SetRootSignature(pipeline_state ? pipeline_state->num_params_ : 0);
  local_buffer_states_.Clear();
  local_texture_states_.Clear();
  required_initial_layouts_.clear();
  sealed_in_list_.clear();
  textures_to_seal_.clear();
  buffer_barriers_.clear();
  texture_barriers_.clear();

#ifndef NDEBUG
  event_depth_ = 0;
#endif
}


auto CommandList::End() -> void {
  for (auto const* const tex : textures_to_seal_) {
    TrackAccess(*tex, {
      .scope = {.sync = D3D12_BARRIER_SYNC_NONE, .access = D3D12_BARRIER_ACCESS_NO_ACCESS}, .layout = kSealedLayout
    });
  }

  RecordBarriers();

  assert(event_depth_ == 0 && "CommandList::End before EndEvent");
  ThrowIfFailed(cmd_list_->Close(), "Failed to close command list.");
}


auto CommandList::BeginEvent(std::wstring_view const name) -> void {
  std::array<wchar_t, kMaxEvenNameLength + 1> buf;
  auto const len = std::min(name.size(), kMaxEvenNameLength);
  std::ranges::copy(name.substr(0, len), buf.begin());
  buf[len] = L'\0';
  cmd_list_->BeginEvent(0, buf.data(), static_cast<UINT>((len + 1) * sizeof(wchar_t)));

#ifndef NDEBUG
  ++event_depth_;
#endif
}


auto CommandList::EndEvent() -> void {
  assert(event_depth_ > 0 && "EndEvent without maching BeginEvent");
  cmd_list_->EndEvent();

#ifndef NDEBUG
  --event_depth_;
#endif
}


auto CommandList::BuildAccelerationStructure(BuildAccelerationStructureDesc const& desc) -> void {
  auto geometry_storage = std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>{};
  auto const d3d_inputs = detail::ToD3D12(desc.inputs, geometry_storage);

#ifndef NDEBUG
  {
    ComPtr<ID3D12Device5> device;
    ThrowIfFailed(cmd_list_->GetDevice(IID_PPV_ARGS(&device)), "Failed to get device.");
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&d3d_inputs, &info);

    assert(desc.dst_as.get().GetRegion().size >= info.ResultDataMaxSizeInBytes && "Build AS destination too small.");

    auto const is_update = (std::visit([](auto const& in) { return in.flags; }, desc.inputs) &
                            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) != 0;
    auto const needed_scratch_size = is_update ? info.UpdateScratchDataSizeInBytes : info.ScratchDataSizeInBytes;
    assert((desc.src_as != nullptr) == is_update && "Source AS and update flag must be passed together.");
    assert(
      (needed_scratch_size == 0 || (desc.scratch_buffer && desc.scratch_buffer->GetRegion().size >=
          needed_scratch_size)
      ) && "Build AS scratch buffer too small.");
  }
#endif

  assert(
    ((std::holds_alternative<BuildBottomLevelAsInputs>(desc.inputs) && desc.dst_as.get().GetDesc().type ==
        AccelerationStructureType::kBottomLevel) ||
      (std::holds_alternative<BuildTopLevelAsInputs>(desc.inputs) && desc.dst_as.get().GetDesc().type ==
        AccelerationStructureType::kTopLevel)) && "AS build input type must match destination type.");

  // Source first: for an in-place update (src == dst) the tracker must end in the write state so a later read
  // generates a barrier.
  if (desc.src_as) {
    assert(desc.src_as->GetDesc().type == desc.dst_as.get().GetDesc().type &&
      "AS build source and destination types must match.");
    TrackAccess(*desc.src_as->GetBuffer(), {
      .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
      .access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ
    });
  }

  std::visit(Overloaded{
    [this](BuildBottomLevelAsInputs const& in) {
      for (auto const& geom : in.geometries) {
        TrackAccess(geom.vertices.GetBuffer(), {
          .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
          .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
        });

        if (geom.indices) {
          TrackAccess(geom.indices->GetBuffer(), {
            .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
          });
        }

        if (geom.transform) {
          TrackAccess(geom.transform->GetBuffer(), {
            .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
            .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
          });
        }
      }
    },
    [this](BuildTopLevelAsInputs const& in) {
      TrackAccess(in.instances.GetBuffer(), {
        .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
        .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
      });
    }
  }, desc.inputs);

  TrackAccess(*desc.dst_as.get().GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
    .access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE
  });

  if (desc.scratch_buffer) {
    assert(HasAny(desc.scratch_buffer->GetBuffer().GetDesc().usage, BufferUsage::kAccelerationScratch));
    assert(desc.scratch_buffer->GetRegion().offset % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0
      && "AS build scratch buffer must be properly aligned.");
    TrackAccess(desc.scratch_buffer->GetBuffer(), {
      .sync = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
      .access = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS
    });
  }

  RecordBarriers();

  auto const d3d_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
    .DestAccelerationStructureData = desc.dst_as.get().GetGpuAddress(),
    .Inputs = d3d_inputs,
    .SourceAccelerationStructureData = desc.src_as
                                         ? desc.src_as->GetGpuAddress()
                                         : 0,
    .ScratchAccelerationStructureData = desc.scratch_buffer
                                          ? detail::GetGpuAddress(*desc.scratch_buffer)
                                          : 0
  };

  cmd_list_->BuildRaytracingAccelerationStructure(&d3d_desc, 0, nullptr);
}


auto CommandList::ClearDepthStencil(Texture const& tex, D3D12_CLEAR_FLAGS const clear_flags, FLOAT const depth,
                                    UINT8 const stencil, std::span<D3D12_RECT const> const rects,
                                    UINT16 const mip_level) -> void {
  TrackAccess(tex, {
    .scope = {.sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL, .access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE},
    .layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE
  });
  RecordBarriers();

  cmd_list_->ClearDepthStencilView(dsv_heap_->GetDescriptorCpuHandle(tex.GetDepthStencilView(mip_level)), clear_flags,
    depth, stencil,
    static_cast<UINT>(rects.size()), rects.data());
}


auto CommandList::ClearRenderTarget(Texture const& tex, std::span<FLOAT const, 4> const color_rgba,
                                    std::span<D3D12_RECT const> const rects, UINT16 const mip_level) -> void {
  TrackAccess(tex, {
    .scope = {.sync = D3D12_BARRIER_SYNC_RENDER_TARGET, .access = D3D12_BARRIER_ACCESS_RENDER_TARGET},
    .layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET
  });
  RecordBarriers();

  cmd_list_->ClearRenderTargetView(rtv_heap_->GetDescriptorCpuHandle(tex.GetRenderTargetView(mip_level)),
    color_rgba.data(),
    static_cast<UINT>(rects.size()), rects.data());
}


auto CommandList::CopyBuffer(Buffer const& dst, Buffer const& src) -> void {
  assert(HasAny(src.GetDesc().usage, BufferUsage::kCopySource));
  assert(HasAny(dst.GetDesc().usage, BufferUsage::kCopyDestination));

  TrackAccess(src, {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_SOURCE});
  TrackAccess(dst, {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_DEST});
  RecordBarriers();

  cmd_list_->CopyResource(dst.GetInternalResource(), src.GetInternalResource());
}


auto CommandList::CopyBufferRegion(Buffer const& dst, UINT64 const dst_offset, Buffer const& src,
                                   UINT64 const src_offset, UINT64 const num_bytes) -> void {
  assert(HasAny(src.GetDesc().usage, BufferUsage::kCopySource));
  assert(HasAny(dst.GetDesc().usage, BufferUsage::kCopyDestination));

  TrackAccess(src, {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_SOURCE});
  TrackAccess(dst, {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_DEST});
  RecordBarriers();

  cmd_list_->CopyBufferRegion(dst.GetInternalResource(), dst_offset, src.GetInternalResource(), src_offset, num_bytes);
}


auto CommandList::CopyTexture(Texture const& dst, Texture const& src) -> void {
  TrackAccess(src, {
    .scope = {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_SOURCE},
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE
  });
  TrackAccess(dst, {
    .scope = {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_DEST},
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST
  });
  RecordBarriers();

  cmd_list_->CopyResource(dst.GetInternalResource(), src.GetInternalResource());
}


auto CommandList::CopyTextureRegion(Texture const& dst, UINT const dst_subresource_index, UINT const dst_x,
                                    UINT const dst_y, UINT const dst_z, Texture const& src,
                                    UINT const src_subresource_index, D3D12_BOX const* src_box) -> void {
  TrackAccess(src, {
    .scope = {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_SOURCE},
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE
  });
  TrackAccess(dst, {
    .scope = {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_DEST},
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST
  });
  RecordBarriers();

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
  assert(HasAny(src.GetDesc().usage, BufferUsage::kCopySource));

  TrackAccess(src, {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_SOURCE});
  TrackAccess(dst, {
    .scope = {.sync = D3D12_BARRIER_SYNC_COPY, .access = D3D12_BARRIER_ACCESS_COPY_DEST},
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST
  });
  RecordBarriers();

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
  TrackAccess(tex, {
    .scope = {.sync = D3D12_BARRIER_SYNC_RENDER_TARGET, .access = D3D12_BARRIER_ACCESS_RENDER_TARGET},
    .layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET
  });
  RecordBarriers();
  cmd_list_->DiscardResource(tex.GetInternalResource(), region ? &*region : nullptr);
}


auto CommandList::DiscardDepthStencil(Texture const& tex, std::optional<D3D12_DISCARD_REGION> const& region) -> void {
  TrackAccess(tex, {
    .scope = {.sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL, .access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE},
    .layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE
  });
  RecordBarriers();
  cmd_list_->DiscardResource(tex.GetInternalResource(), region ? &*region : nullptr);
}


auto CommandList::Dispatch(UINT const thread_group_count_x, UINT const thread_group_count_y,
                           UINT const thread_group_count_z) -> void {
  RecordBarriers();
  cmd_list_->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}


auto CommandList::DispatchMesh(UINT const thread_group_count_x, UINT const thread_group_count_y,
                               UINT const thread_group_count_z) -> void {
  RecordBarriers();
  cmd_list_->DispatchMesh(thread_group_count_x, thread_group_count_y, thread_group_count_z);
}


auto CommandList::DispatchRays(DispatchRaysDesc const& desc) -> void {
  [[maybe_unused]] auto const is_table_start_aligned = [](BufferRegion const& region) {
    return region.offset % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0;
  };

  [[maybe_unused]] auto const is_table_stride_valid = [](BufferRegion const& region) {
    return region.stride % D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT == 0 &&
           region.stride <= D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE &&
           (region.stride == 0 || region.stride >= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  };

  assert(is_table_start_aligned(desc.ray_generation_shader_record.GetRegion()));
  assert(desc.ray_generation_shader_record.GetRegion().size >= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  assert(desc.ray_generation_shader_record.GetRegion().stride == 0);

  if (desc.miss_shader_table) {
    assert(is_table_start_aligned(desc.miss_shader_table->GetRegion()));
    assert(is_table_stride_valid(desc.miss_shader_table->GetRegion()));
  }

  if (desc.hit_group_table) {
    assert(is_table_start_aligned(desc.hit_group_table->GetRegion()));
    assert(is_table_stride_valid(desc.hit_group_table->GetRegion()));
  }

  if (desc.callable_shader_table) {
    assert(is_table_start_aligned(desc.callable_shader_table->GetRegion()));
    assert(is_table_stride_valid(desc.callable_shader_table->GetRegion()));
  }

  TrackAccess(desc.ray_generation_shader_record.GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_RAYTRACING,
    .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
  });

  if (desc.miss_shader_table) {
    TrackAccess(desc.miss_shader_table->GetBuffer(), {
      .sync = D3D12_BARRIER_SYNC_RAYTRACING,
      .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
    });
  }

  if (desc.hit_group_table) {
    TrackAccess(desc.hit_group_table->GetBuffer(), {
      .sync = D3D12_BARRIER_SYNC_RAYTRACING,
      .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
    });
  }

  if (desc.callable_shader_table) {
    TrackAccess(desc.callable_shader_table->GetBuffer(), {
      .sync = D3D12_BARRIER_SYNC_RAYTRACING,
      .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
    });
  }

  RecordBarriers();

  auto const to_shader_table = [](std::optional<BufferRange> const& range) {
    return range
             ? D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
               .StartAddress = detail::GetGpuAddress(*range),
               .SizeInBytes = range->GetRegion().size,
               .StrideInBytes = range->GetRegion().stride
             }
             : D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{};
  };

  D3D12_DISPATCH_RAYS_DESC const d3d_desc{
    .RayGenerationShaderRecord = {
      .StartAddress = detail::GetGpuAddress(desc.ray_generation_shader_record),
      .SizeInBytes = desc.ray_generation_shader_record.GetRegion().size
    },
    .MissShaderTable = to_shader_table(desc.miss_shader_table),
    .HitGroupTable = to_shader_table(desc.hit_group_table),
    .CallableShaderTable = to_shader_table(desc.callable_shader_table),
    .Width = desc.width,
    .Height = desc.height,
    .Depth = desc.depth
  };

  cmd_list_->DispatchRays(&d3d_desc);
}


auto CommandList::DrawIndexedInstanced(UINT const index_count_per_instance, UINT const instance_count,
                                       UINT const start_index_location, INT const base_vertex_location,
                                       UINT const start_instance_location) -> void {
  RecordBarriers();

  auto const offsets = std::array{std::bit_cast<UINT const>(base_vertex_location), start_instance_location};
  cmd_list_->SetGraphicsRoot32BitConstants(d3d12::detail::kDrawCallParamsRootIdx, static_cast<UINT>(offsets.size()),
    offsets.data(), 0);
  cmd_list_->DrawIndexedInstanced(index_count_per_instance, instance_count, start_index_location, base_vertex_location,
    start_instance_location);
}


auto CommandList::DrawInstanced(UINT const vertex_count_per_instance, UINT const instance_count,
                                UINT const start_vertex_location, UINT const start_instance_location) -> void {
  RecordBarriers();

  auto const offsets = std::array{0u, start_instance_location};
  cmd_list_->SetGraphicsRoot32BitConstants(d3d12::detail::kDrawCallParamsRootIdx, static_cast<UINT>(offsets.size()),
    offsets.data(), 0);
  cmd_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}


auto CommandList::Resolve(Texture const& dst, Texture const& src, DXGI_FORMAT const format) -> void {
  TrackAccess(src, {
    .scope = {.sync = D3D12_BARRIER_SYNC_RESOLVE, .access = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE},
    .layout = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE
  });
  TrackAccess(dst, {
    .scope = {.sync = D3D12_BARRIER_SYNC_RESOLVE, .access = D3D12_BARRIER_ACCESS_RESOLVE_DEST},
    .layout = D3D12_BARRIER_LAYOUT_RESOLVE_DEST
  });
  RecordBarriers();
  cmd_list_->ResolveSubresource(dst.GetInternalResource(), 0, src.GetInternalResource(), 0, format);
}


auto CommandList::Seal(Buffer const& buf) -> void {
  sealed_in_list_.emplace_back(&buf);
}


auto CommandList::Seal(Texture const& tex) -> void {
  sealed_in_list_.emplace_back(&tex);
  textures_to_seal_.emplace_back(&tex);
}


auto CommandList::SetBlendFactor(std::span<FLOAT const, 4> const blend_factor) const -> void {
  cmd_list_->OMSetBlendFactor(blend_factor.data());
}


auto CommandList::SetIndexBuffer(Buffer const& buf, DXGI_FORMAT const index_format) -> void {
  assert(HasAny(buf.GetDesc().usage, BufferUsage::kIndexBuffer));
  TrackAccess(buf, {.sync = D3D12_BARRIER_SYNC_INDEX_INPUT, .access = D3D12_BARRIER_ACCESS_INDEX_BUFFER});

  D3D12_INDEX_BUFFER_VIEW const ibv{
    .BufferLocation = buf.GetInternalResource()->GetGPUVirtualAddress(),
    .SizeInBytes = static_cast<UINT>(buf.GetInternalResource()->GetDesc1().Width),
    .Format = index_format
  };
  cmd_list_->IASetIndexBuffer(&ibv);
}


auto CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY const primitive_topology) const -> void {
  cmd_list_->IASetPrimitiveTopology(primitive_topology);
}


auto CommandList::SetRenderTargets(std::span<Texture const* const> const render_targets,
                                   Texture const* const depth_stencil, UINT16 const mip_level) -> void {
  assert(render_targets.size() <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT &&
    "At most D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT render targets can be set.");
  auto const rts = render_targets.first(std::min<std::size_t>(render_targets.size(),
    D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT));

  std::ranges::for_each(rts, [this](Texture const* const tex) {
    if (tex) {
      TrackAccess(*tex, {
        .scope = {.sync = D3D12_BARRIER_SYNC_RENDER_TARGET, .access = D3D12_BARRIER_ACCESS_RENDER_TARGET},
        .layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET
      });
    }
  });

  if (depth_stencil) {
    TrackAccess(*depth_stencil,
      pipeline_allows_ds_write_
        ? detail::TextureAccess{
          .scope = {
            .sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
            .access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE
          },
          .layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE
        }
        : detail::TextureAccess{
          .scope = {
            .sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
            .access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ
          },
          .layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ
        });
  }

  auto rt_handles = std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT>{};

  std::ranges::transform(rts, rt_handles.begin(), [&](Texture const* const tex) {
    return tex
             ? rtv_heap_->GetDescriptorCpuHandle(tex->GetRenderTargetView(mip_level))
             : D3D12_CPU_DESCRIPTOR_HANDLE{};
  });

  auto const ds_handle = depth_stencil
                           ? dsv_heap_->GetDescriptorCpuHandle(depth_stencil->GetDepthStencilView(mip_level))
                           : D3D12_CPU_DESCRIPTOR_HANDLE{};

  cmd_list_->OMSetRenderTargets(static_cast<UINT>(rts.size()), rt_handles.data(), FALSE,
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


auto CommandList::SetPipelineParameters(UINT const index, UINT const count, void const* const params) const -> void {
  if (compute_pipeline_set_) {
    cmd_list_->SetComputeRoot32BitConstants(d3d12::detail::kDrawParamsRootIdx, count, params, index);
  } else {
    cmd_list_->SetGraphicsRoot32BitConstants(d3d12::detail::kDrawParamsRootIdx, count, params, index);
  }
}


auto CommandList::SetConstantBuffer(ParamField<UINT> const field, BufferView const& buf_view) -> void {
  SetPipelineParameter(field, ReferenceConstantBuffer(buf_view));
}


auto CommandList::SetShaderResource(ParamField<UINT> const field, BufferView const& buf_view) -> void {
  SetPipelineParameter(field, ReferenceShaderResource(buf_view));
}


auto CommandList::SetShaderResource(ParamField<UINT> const field, Texture const& tex) -> void {
  SetPipelineParameter(field, ReferenceShaderResource(tex));
}


auto CommandList::SetUnorderedAccess(ParamField<UINT> const field, BufferView const& buf_view) -> void {
  SetPipelineParameter(field, ReferenceUnorderedAccess(buf_view));
}


auto CommandList::SetUnorderedAccess(ParamField<UINT> const field, Texture const& tex) -> void {
  SetPipelineParameter(field, ReferenceUnorderedAccess(tex));
}


auto CommandList::SetAccelerationStructure(ParamField<UINT> const field, AccelerationStructure const& as) -> void {
  SetPipelineParameter(field, ReferenceAccelerationStructure(as));
}


auto CommandList::ReferenceConstantBuffer(BufferView const& buf_view) -> UINT {
  assert(HasAny(buf_view.GetDesc().usage, BufferViewUsage::kConstantBuffer));
  TrackAccess(*buf_view.GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
    .access = D3D12_BARRIER_ACCESS_CONSTANT_BUFFER
  });
  return buf_view.GetConstantBuffer();
}


auto CommandList::ReferenceShaderResource(BufferView const& buf_view) -> UINT {
  assert(HasAny(buf_view.GetDesc().usage, BufferViewUsage::kShaderResource));
  TrackAccess(*buf_view.GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
    .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
  });
  return buf_view.GetShaderResource();
}


auto CommandList::ReferenceShaderResource(Texture const& tex) -> UINT {
  TrackAccess(tex, {
    .scope = {
      .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
      .access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE
    },
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
  });
  return tex.GetShaderResource();
}


auto CommandList::ReferenceUnorderedAccess(BufferView const& buf_view) -> UINT {
  assert(HasAny(buf_view.GetDesc().usage, BufferViewUsage::kUnorderedAccess));
  TrackAccess(*buf_view.GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
    .access = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS
  });
  return buf_view.GetUnorderedAccess();
}


auto CommandList::ReferenceUnorderedAccess(Texture const& tex) -> UINT {
  TrackAccess(tex, {
    .scope = {
      .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
      .access = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS
    },
    .layout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS
  });
  return tex.GetUnorderedAccess();
}


auto CommandList::ReferenceAccelerationStructure(AccelerationStructure const& as) -> UINT {
  assert(
    as.GetDesc().type == AccelerationStructureType::kTopLevel && "an AS referenced on the pipeline must be top-level.");
  TrackAccess(*as.GetBuffer(), {
    .sync = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING,
    .access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ
  });
  return as.GetShaderResource();
}


auto CommandList::UseBuffer(Buffer const& buf, D3D12_BARRIER_SYNC const sync,
                            D3D12_BARRIER_ACCESS const access) -> void {
  TrackAccess(buf, {.sync = sync, .access = access});
}


auto CommandList::UseTexture(Texture const& tex, D3D12_BARRIER_SYNC const sync, D3D12_BARRIER_ACCESS const access,
                             D3D12_BARRIER_LAYOUT const layout) -> void {
  TrackAccess(tex, {.scope = {.sync = sync, .access = access}, .layout = layout});
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
                         detail::DescriptorHeap const* dsv_heap, detail::DescriptorHeap const* rtv_heap,
                         detail::DescriptorHeap const* res_desc_heap, detail::DescriptorHeap const* sampler_heap,
                         detail::RootSignatureCache* root_signatures) :
  allocator_{std::move(allocator)},
  cmd_list_{std::move(cmd_list)},
  dsv_heap_{dsv_heap},
  rtv_heap_{rtv_heap},
  res_desc_heap_{res_desc_heap},
  sampler_heap_{sampler_heap},
  root_signatures_{root_signatures} {}


auto CommandList::TrackAccess(Buffer const& buf, detail::ResourceAccessScope const requested) -> void {
  if (buf.IsSealed()) {
    assert(!d3d12::detail::IsWriteAccess(requested.access));
    return;
  }

  // Read operations don't need syncing, there's no writes to be made visible,
  // and buffers have no layout, so there's nothing to generate a barrier for.
  if (!detail::IsGpuWritable(buf.GetDesc())) {
    return;
  }

  auto const requested_is_write = d3d12::detail::IsWriteAccess(requested.access);
  auto* const local_state = local_buffer_states_.Get(buf.GetInternalResource());

  // First use.
  if (!local_state) {
    local_buffer_states_.Record(buf.GetInternalResource(), {
      .current = requested,
      .written = requested_is_write
    });
    return;
  }

  auto const make_barrier = [&](detail::ResourceAccessScope const& after) {
    return D3D12_BUFFER_BARRIER{
      .SyncBefore = local_state->current.sync,
      .SyncAfter = after.sync,
      .AccessBefore = local_state->current.access,
      .AccessAfter = after.access,
      .pResource = buf.GetInternalResource(),
      .Offset = 0,
      .Size = UINT64_MAX
    };
  };

  switch (detail::ClassifyTransition(local_state->current, requested, local_state->written)) {
    case detail::AccessTransition::kPlaceBarrier: {
      buffer_barriers_.emplace_back(make_barrier(requested));
      local_state->current = requested;
      local_state->written = local_state->written || requested_is_write;
      break;
    }

    case detail::AccessTransition::kMergeWithoutBarrier: {
      local_state->current = detail::Union(local_state->current, requested);
      break;
    }

    case detail::AccessTransition::kAlreadyCovered: {
      break;
    }

    case detail::AccessTransition::kExtendLatestBarrier: {
      if (auto const [it, end] = std::ranges::find_last(buffer_barriers_, buf.GetInternalResource(),
        &D3D12_BUFFER_BARRIER::pResource); it != end) {
        auto const new_scope = detail::Union({.sync = it->SyncAfter, .access = it->AccessAfter}, requested);
        it->SyncAfter = new_scope.sync;
        it->AccessAfter = new_scope.access;
        local_state->current = new_scope;
        break;
      }

      auto const new_scope = detail::Union(local_state->current, requested);
      buffer_barriers_.emplace_back(make_barrier(new_scope));
      local_state->current = new_scope;
      break;
    }
  }
}


auto CommandList::TrackAccess(Texture const& tex, detail::TextureAccess const requested) -> void {
  if (tex.IsSealed()) {
    assert(!d3d12::detail::IsWriteAccess(requested.scope.access) && requested.layout == kSealedLayout);
    return;
  }

  auto const requested_is_write = d3d12::detail::IsWriteAccess(requested.scope.access);
  auto const local_state = local_texture_states_.Get(tex.GetInternalResource());

  // First use.
  if (!local_state) {
    required_initial_layouts_.emplace_back(requested.layout, tex.GetInternalResource());
    local_texture_states_.Record(tex.GetInternalResource(), {
      .current = requested,
      .written = requested_is_write
    });
    return;
  }

  auto const make_barrier = [&](detail::TextureAccess const after) {
    return D3D12_TEXTURE_BARRIER{
      .SyncBefore = local_state->current.scope.sync,
      .SyncAfter = after.scope.sync,
      .AccessBefore = local_state->current.scope.access,
      .AccessAfter = after.scope.access,
      .LayoutBefore = local_state->current.layout,
      .LayoutAfter = after.layout,
      .pResource = tex.GetInternalResource(),
      .Subresources = {
        .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0, .NumArraySlices = 0,
        .FirstPlane = 0, .NumPlanes = 0
      },
      .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
    };
  };

  switch (detail::ClassifyTransition(local_state->current, requested, local_state->written)) {
    case detail::AccessTransition::kPlaceBarrier: {
      texture_barriers_.emplace_back(make_barrier(requested));
      local_state->current = requested;
      local_state->written = local_state->written || requested_is_write;
      break;
    }

    case detail::AccessTransition::kMergeWithoutBarrier: {
      local_state->current.scope = detail::Union(local_state->current.scope, requested.scope);
      break;
    }

    case detail::AccessTransition::kAlreadyCovered: {
      break;
    }

    case detail::AccessTransition::kExtendLatestBarrier: {
      if (auto const [it, end] = std::ranges::find_last(texture_barriers_, tex.GetInternalResource(),
        &D3D12_TEXTURE_BARRIER::pResource); it != end) {
        auto const new_scope = detail::Union({.sync = it->SyncAfter, .access = it->AccessAfter}, requested.scope);
        it->SyncAfter = new_scope.sync;
        it->AccessAfter = new_scope.access;
        local_state->current.scope = new_scope;
        break;
      }

      auto const new_scope = detail::Union(local_state->current.scope, requested.scope);
      texture_barriers_.emplace_back(make_barrier({.scope = new_scope, .layout = local_state->current.layout}));
      local_state->current.scope = new_scope;
      break;
    }
  }
}


auto CommandList::RecordBarriers() -> void {
  auto groups = std::array<D3D12_BARRIER_GROUP, 2>{};
  auto group_count = UINT32{0};

  if (!buffer_barriers_.empty()) {
    groups[group_count++] = D3D12_BARRIER_GROUP{
      .Type = D3D12_BARRIER_TYPE_BUFFER,
      .NumBarriers = static_cast<UINT32>(buffer_barriers_.size()),
      .pBufferBarriers = buffer_barriers_.data()
    };
  }

  if (!texture_barriers_.empty()) {
    groups[group_count++] = D3D12_BARRIER_GROUP{
      .Type = D3D12_BARRIER_TYPE_TEXTURE,
      .NumBarriers = static_cast<UINT32>(texture_barriers_.size()),
      .pTextureBarriers = texture_barriers_.data()
    };
  }

  if (group_count > 0) {
    cmd_list_->Barrier(group_count, groups.data());
    buffer_barriers_.clear();
    texture_barriers_.clear();
  }
}
}
