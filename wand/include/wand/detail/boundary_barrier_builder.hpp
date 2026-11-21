#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

#include <wand/detail/resource_tracking.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
class CommandList;
class Texture;


namespace detail {
class BoundaryBarrierBuilder {
public:
  auto Build(std::span<CommandList const* const> cmd_lists, GlobalResourceStateTracker const& global_states) -> void;


  struct BoundaryBarriers {
    std::span<D3D12_BUFFER_BARRIER const> buffer;
    std::span<D3D12_TEXTURE_BARRIER const> texture;
  };


  [[nodiscard]]
  auto GetBoundaryBarriersForCommandList(std::size_t idx) const -> BoundaryBarriers;

  [[nodiscard]]
  auto GetTextureSealBarriers() const -> std::span<D3D12_TEXTURE_BARRIER const>;

private:
  auto BuildBufferBarriers(CommandList const& cmd) -> void;
  auto BuildTextureBarriers(CommandList const& cmd, GlobalResourceStateTracker const& global_states) -> void;
  auto BuildTextureSealBarriers(std::span<CommandList const* const> cmd_lists,
                                GlobalResourceStateTracker const& global_states) -> void;


  struct BarrierRange {
    std::size_t offset;
    std::size_t count;
  };


  struct PredecessorBufferState {
    AccessScope latest_scope;
    bool has_prior_write;
    std::optional<std::size_t> patchable_barrier_idx;
  };


  struct PredecessorTextureState {
    TextureAccess latest_access;
    bool has_prior_modification;
    std::optional<std::size_t> patchable_barrier_idx;
  };


  ResourceStateTracker<PredecessorBufferState> predecessor_buffer_states_;
  std::vector<D3D12_BUFFER_BARRIER> buffer_barriers_;
  // The ith range corresponds to the ith command list
  std::vector<BarrierRange> buffer_barrier_ranges_;

  ResourceStateTracker<PredecessorTextureState> predecessor_texture_states_;
  std::vector<D3D12_TEXTURE_BARRIER> texture_barriers_;
  // The ith range corresponds to the ith command list
  std::vector<BarrierRange> texture_barrier_ranges_;

  std::unordered_set<Texture const*> textures_to_seal_;
  std::optional<BarrierRange> texture_seal_barrier_range_;
};
}
}
