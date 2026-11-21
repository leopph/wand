#include "wand/detail/boundary_barrier_builder.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <ranges>

#include "wand/command_list.hpp"


namespace wand::detail {
auto BoundaryBarrierBuilder::Build(std::span<CommandList const* const> const cmd_lists,
                                   GlobalResourceStateTracker const& global_states) -> void {
  assert(std::ranges::none_of(cmd_lists, [](auto const* const cmd) { return !cmd; }) && "Command list cannot be null.");

  predecessor_buffer_states_.Clear();
  buffer_barriers_.clear();
  buffer_barrier_ranges_.clear();
  predecessor_texture_states_.Clear();
  texture_barriers_.clear();
  texture_barrier_ranges_.clear();
  textures_to_seal_.clear();
  texture_seal_barrier_range_.reset();

  buffer_barrier_ranges_.reserve(cmd_lists.size());
  texture_barrier_ranges_.reserve(cmd_lists.size());

  for (auto const* const cmd : cmd_lists) {
    BuildBufferBarriers(*cmd);
    BuildTextureBarriers(*cmd, global_states);
  }

  BuildTextureSealBarriers(cmd_lists, global_states);
}


auto BoundaryBarrierBuilder::GetBoundaryBarriersForCommandList(
  std::size_t const idx
) const -> BoundaryBarriers {
  assert(buffer_barrier_ranges_.size() == texture_barrier_ranges_.size() && "Range count inconsistent.");
  assert(idx < buffer_barrier_ranges_.size() && "Index out of range.");

  auto const& buffer_range = buffer_barrier_ranges_[idx];
  auto const& texture_range = texture_barrier_ranges_[idx];

  auto const buf_barriers = std::span{buffer_barriers_};
  auto const tex_barriers = std::span{texture_barriers_};

  return BoundaryBarriers{
    .buffer = buf_barriers.subspan(buffer_range.offset, buffer_range.count),
    .texture = tex_barriers.subspan(texture_range.offset, texture_range.count)
  };
}


auto BoundaryBarrierBuilder::GetTextureSealBarriers() const -> std::span<D3D12_TEXTURE_BARRIER const> {
  return texture_seal_barrier_range_
           ? std::span{texture_barriers_}.subspan(texture_seal_barrier_range_->offset,
             texture_seal_barrier_range_->count)
           : std::span<D3D12_TEXTURE_BARRIER const>{};
}


auto BoundaryBarrierBuilder::BuildBufferBarriers(CommandList const& cmd) -> void {
  auto const first_barrier_idx = buffer_barriers_.size();
  auto buffer_barrier_count = 0uz;

  for (auto const& [buf, list_state] : cmd.buffer_states_) {
    auto const predecessor = predecessor_buffer_states_.Get(buf);

    if (!predecessor) {
      predecessor_buffer_states_.Record(buf, {
        .latest_scope = list_state.latest_scope, .has_prior_write = list_state.written_in_list,
        .patchable_barrier_idx = std::nullopt
      });
      continue;
    }

    auto const place_barrier = [&](AccessScope const& requested) {
      predecessor->patchable_barrier_idx = buffer_barriers_.size();
      buffer_barriers_.emplace_back(predecessor->latest_scope.sync, requested.sync, predecessor->latest_scope.access,
        requested.access, buf, 0, UINT64_MAX);
      ++buffer_barrier_count;
    };

    // If this command list recorded a barrier for this buffer, that barrier only covers the state known by the
    // command list at record time, which is its first scope. If that doesn't cover the last scope of the previous
    // command list, then any reads inside that last scope may be reordered with the write in this command list.
    // In that case, we have to protect those reads with a barrier either way.
    auto const predecessor_reads_unprotected = list_state.first_scope_closed && !CoversSync(list_state.first_scope.sync,
                                                 predecessor->latest_scope.sync);
    auto const transition = predecessor_reads_unprotected
                              ? AccessTransition::kPlaceBarrier
                              : ClassifyTransition(predecessor->latest_scope, list_state.first_scope,
                                predecessor->has_prior_write);

    switch (transition) {
      case AccessTransition::kPlaceBarrier: {
        place_barrier(list_state.first_scope);
        break;
      }

      case AccessTransition::kExtendLatestBarrier: {
        // If the latest barrier was a boundary barrier, we can patch it. If it was inside the command list, we can
        // only chain a new one.
        if (predecessor->patchable_barrier_idx) {
          auto& barrier = buffer_barriers_[*predecessor->patchable_barrier_idx];
          auto const new_after = Union({.sync = barrier.SyncAfter, .access = barrier.AccessAfter},
            list_state.first_scope);
          barrier.SyncAfter = new_after.sync;
          barrier.AccessAfter = new_after.access;
          break;
        }

        place_barrier(Union(predecessor->latest_scope, list_state.first_scope));
        break;
      }

      case AccessTransition::kMergeWithoutBarrier: {
        [[fallthrough]];
      }

      case AccessTransition::kAlreadyCovered: {
        break;
      }
    }

    if (list_state.first_scope_closed) {
      // This command list closed its first usage scope of the buffer. That means it placed a barrier for it. Thus,
      // the barrier inside the command list is now the latest one.
      predecessor->latest_scope = list_state.latest_scope;
      predecessor->patchable_barrier_idx = std::nullopt;
    } else if (transition == AccessTransition::kPlaceBarrier) {
      // The command list didn't close its first scope, so first scope == latest scope. We placed a barrier before
      // the command list, which starts a new scope, so the command list's first scope really starts at the boundary.
      predecessor->latest_scope = list_state.first_scope;
    } else {
      // There was no scope restart, so the predecessor's scope grows by this list's accesses.
      predecessor->latest_scope = Union(predecessor->latest_scope, list_state.first_scope);
    }

    predecessor->has_prior_write = predecessor->has_prior_write || list_state.written_in_list;
  }

  buffer_barrier_ranges_.emplace_back(first_barrier_idx, buffer_barrier_count);
}


auto BoundaryBarrierBuilder::BuildTextureBarriers(
  CommandList const& cmd,
  GlobalResourceStateTracker const& global_states
) -> void {
  auto const first_barrier_idx = texture_barriers_.size();
  auto texture_barrier_count = 0uz;

  for (auto const& [tex, list_state] : cmd.texture_states_) {
    auto const predecessor = [&] {
      if (auto const pred = predecessor_texture_states_.Get(tex)) {
        return pred;
      }

      auto const initial_state = global_states.Get(tex);
      assert(initial_state);
      predecessor_texture_states_.Record(tex, {
        .latest_access = {
          .scope = {.sync = D3D12_BARRIER_SYNC_NONE, .access = D3D12_BARRIER_ACCESS_NO_ACCESS},
          .layout = initial_state->layout
        },
        .has_prior_modification = false,
        .patchable_barrier_idx = std::nullopt
      });

      return predecessor_texture_states_.Get(tex);
    }();

    auto const place_barrier = [&](TextureAccess const& requested) {
      predecessor->patchable_barrier_idx = texture_barriers_.size();

      texture_barriers_.emplace_back(predecessor->latest_access.scope.sync, requested.scope.sync,
        predecessor->latest_access.scope.access, requested.scope.access, predecessor->latest_access.layout,
        requested.layout, tex, D3D12_BARRIER_SUBRESOURCE_RANGE{
          .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0, .NumArraySlices = 0,
          .FirstPlane = 0, .NumPlanes = 0
        }, D3D12_TEXTURE_BARRIER_FLAG_NONE);

      ++texture_barrier_count;

      if (predecessor->latest_access.layout != requested.layout) {
        // A layout change is a modification, even though technically not a write access
        predecessor->has_prior_modification = true;
      }
    };

    // If this command list recorded a barrier for this texture, that barrier only covers the state known by the
    // command list at record time, which is its first scope. If that doesn't cover the last scope of the previous
    // command list, then any reads inside that last scope may be reordered with the write in this command list.
    // In that case, we have to protect those reads with a barrier either way.
    auto const predecessor_reads_unprotected = list_state.first_access_scope_closed && !CoversSync(
                                                 list_state.first_access.scope.sync,
                                                 predecessor->latest_access.scope.sync);
    auto const transition = predecessor_reads_unprotected
                              ? AccessTransition::kPlaceBarrier
                              : ClassifyTransition(predecessor->latest_access, list_state.first_access,
                                predecessor->has_prior_modification);

    // This command list's recorded barriers assume it starts in its first layout. Unless a boundary barrier transitions
    // to it, the predecessor must already be in that layout.
    assert((transition == AccessTransition::kPlaceBarrier
        || predecessor->latest_access.layout == list_state.first_access.layout)
      && "Texture must be in the same layout as its first access on the command list.");

    switch (transition) {
      case AccessTransition::kPlaceBarrier: {
        place_barrier(list_state.first_access);
        break;
      }

      case AccessTransition::kExtendLatestBarrier: {
        // If the latest barrier was a boundary barrier, we can patch it. If it was inside the command list, we can
        // only chain a new one.
        if (predecessor->patchable_barrier_idx) {
          auto& barrier = texture_barriers_[*predecessor->patchable_barrier_idx];
          auto const new_after = Union({.sync = barrier.SyncAfter, .access = barrier.AccessAfter},
            list_state.first_access.scope);
          barrier.SyncAfter = new_after.sync;
          barrier.AccessAfter = new_after.access;
          break;
        }

        place_barrier({
          .scope = Union(predecessor->latest_access.scope, list_state.first_access.scope),
          .layout = list_state.first_access.layout
        });
        break;
      }

      case AccessTransition::kMergeWithoutBarrier: {
        [[fallthrough]];
      }

      case AccessTransition::kAlreadyCovered: {
        break;
      }
    }

    if (list_state.first_access_scope_closed) {
      // This command list closed its first usage scope of the texture. That means it placed a barrier for it. Thus,
      // the barrier inside the command list is now the latest one.
      predecessor->latest_access = list_state.latest_access;
      predecessor->patchable_barrier_idx = std::nullopt;
    } else if (transition == AccessTransition::kPlaceBarrier) {
      // The command list didn't close its first scope, so first scope == latest scope. We placed a barrier before
      // the command list, which starts a new scope, so the command list's first scope really starts at the boundary.
      predecessor->latest_access = list_state.first_access;
    } else {
      // There was no scope restart, so the predecessor's scope grows by this list's accesses.
      predecessor->latest_access = {
        .scope = Union(predecessor->latest_access.scope, list_state.first_access.scope),
        .layout = list_state.first_access.layout
      };
    }

    predecessor->has_prior_modification = predecessor->has_prior_modification || list_state.modified_in_list;
  }

  texture_barrier_ranges_.emplace_back(first_barrier_idx, texture_barrier_count);
}


auto BoundaryBarrierBuilder::BuildTextureSealBarriers(
  std::span<CommandList const* const> const cmd_lists,
  GlobalResourceStateTracker const& global_states
) -> void {
  assert(std::ranges::none_of(cmd_lists, [](auto const* const cmd) { return !cmd; }) && "Command list cannot be null.");

  for (auto const& cmd : cmd_lists) {
    for (auto const* const tex : cmd->sealed_textures_in_list_) {
      // The texture might have been sealed since the recording of the command list.
      if (!tex->IsSealed()) {
        textures_to_seal_.emplace(tex);
      }
    }
  }

  auto const first_barrier_idx = texture_barriers_.size();
  texture_barriers_.reserve(texture_barriers_.size() + textures_to_seal_.size());

  std::ranges::transform(textures_to_seal_, std::back_inserter(texture_barriers_), [&](Texture const* const tex) {
    auto const last_access = [&] {
      if (auto const pred = predecessor_texture_states_.Get(tex->GetInternalResource())) {
        return pred->latest_access;
      }

      auto const global_state = global_states.Get(tex->GetInternalResource());
      assert(global_state);

      return TextureAccess{
        .scope = {.sync = D3D12_BARRIER_SYNC_NONE, .access = D3D12_BARRIER_ACCESS_NO_ACCESS},
        .layout = global_state->layout
      };
    }();

    return D3D12_TEXTURE_BARRIER{
      .SyncBefore = last_access.scope.sync, .SyncAfter = D3D12_BARRIER_SYNC_NONE,
      .AccessBefore = last_access.scope.access, .AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS,
      .LayoutBefore = last_access.layout, .LayoutAfter = kSealedLayout, .pResource = tex->GetInternalResource(),
      .Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{
        .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0, .NumArraySlices = 0,
        .FirstPlane = 0, .NumPlanes = 0
      },
      .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
    };
  });

  if (!textures_to_seal_.empty()) {
    texture_seal_barrier_range_ = BarrierRange{.offset = first_barrier_idx, .count = textures_to_seal_.size()};
  }
}
}
