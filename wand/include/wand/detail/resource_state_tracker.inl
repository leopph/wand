#pragma once

#include "resource_state_tracker.hpp"

#include <iterator>


namespace wand::detail {
template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Record(ID3D12Resource* const resource,
                                                     ResourceStateType const state) -> void {
  resource_states_[resource] = state;
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Erase(ID3D12Resource* const resource) -> void {
  resource_states_.erase(resource);
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Get(
  ID3D12Resource* const resource) -> ResourceStateType* {
  if (auto const it{resource_states_.find(resource)}; it != std::end(resource_states_)) {
    return &it->second;
  }

  return nullptr;
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Clear() -> void {
  resource_states_.clear();
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::begin() const {
  return std::begin(resource_states_);
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::end() const {
  return std::end(resource_states_);
}


constexpr auto GetSupersededScopeBits(D3D12_BARRIER_SYNC const sync) -> D3D12_BARRIER_SYNC {
  if (sync == D3D12_BARRIER_SYNC_ALL) {
    return D3D12_BARRIER_SYNC_DRAW
           | D3D12_BARRIER_SYNC_INDEX_INPUT
           | D3D12_BARRIER_SYNC_VERTEX_SHADING
           | D3D12_BARRIER_SYNC_PIXEL_SHADING
           | D3D12_BARRIER_SYNC_DEPTH_STENCIL
           | D3D12_BARRIER_SYNC_RENDER_TARGET
           | D3D12_BARRIER_SYNC_COMPUTE_SHADING
           | D3D12_BARRIER_SYNC_RAYTRACING
           | D3D12_BARRIER_SYNC_COPY
           | D3D12_BARRIER_SYNC_RESOLVE
           | D3D12_BARRIER_SYNC_EXECUTE_INDIRECT
           | D3D12_BARRIER_SYNC_PREDICATION
           | D3D12_BARRIER_SYNC_VIDEO_DECODE
           | D3D12_BARRIER_SYNC_VIDEO_PROCESS
           | D3D12_BARRIER_SYNC_VIDEO_ENCODE
           | D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;
  }

  if (sync == D3D12_BARRIER_SYNC_DRAW) {
    return D3D12_BARRIER_SYNC_INDEX_INPUT
           | D3D12_BARRIER_SYNC_VERTEX_SHADING
           | D3D12_BARRIER_SYNC_PIXEL_SHADING
           | D3D12_BARRIER_SYNC_DEPTH_STENCIL
           | D3D12_BARRIER_SYNC_RENDER_TARGET;
  }

  if (sync == D3D12_BARRIER_SYNC_ALL_SHADING) {
    return D3D12_BARRIER_SYNC_VERTEX_SHADING
           | D3D12_BARRIER_SYNC_PIXEL_SHADING
           | D3D12_BARRIER_SYNC_COMPUTE_SHADING;
  }

  if (sync == D3D12_BARRIER_SYNC_NON_PIXEL_SHADING) {
    return D3D12_BARRIER_SYNC_VERTEX_SHADING
           | D3D12_BARRIER_SYNC_COMPUTE_SHADING;
  }

  return sync;
}


constexpr auto ExpandAggregateScopeBits(D3D12_BARRIER_SYNC const sync) -> D3D12_BARRIER_SYNC {
  auto ret = auto{sync};

  if ((ret & D3D12_BARRIER_SYNC_ALL) == D3D12_BARRIER_SYNC_ALL) {
    ret &= ~D3D12_BARRIER_SYNC_ALL;
    ret |= GetSupersededScopeBits(D3D12_BARRIER_SYNC_ALL);
  }

  if ((ret & D3D12_BARRIER_SYNC_DRAW) == D3D12_BARRIER_SYNC_DRAW) {
    ret &= ~D3D12_BARRIER_SYNC_DRAW;
    ret |= GetSupersededScopeBits(D3D12_BARRIER_SYNC_DRAW);
  }

  if ((ret & D3D12_BARRIER_SYNC_ALL_SHADING) == D3D12_BARRIER_SYNC_ALL_SHADING) {
    ret &= ~D3D12_BARRIER_SYNC_ALL_SHADING;
    ret |= GetSupersededScopeBits(D3D12_BARRIER_SYNC_ALL_SHADING);
  }

  if ((ret & D3D12_BARRIER_SYNC_NON_PIXEL_SHADING) == D3D12_BARRIER_SYNC_NON_PIXEL_SHADING) {
    ret &= ~D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
    ret |= GetSupersededScopeBits(D3D12_BARRIER_SYNC_NON_PIXEL_SHADING);
  }

  return ret;
}


constexpr auto Covers(ResourceAccessScope const& scope, ResourceAccessScope const& other) -> bool {
  // NO_ACCESS != 0, so it needs special handling.
  // NO_ACCESS is covered by everything.
  // NO_ACCESS covers nothing but NO_ACCESS.

  // Sync can contain aggregate bits can be treated as though all of the superseded scope bits are set.

  auto const expanded_scope_sync = ExpandAggregateScopeBits(scope.sync);
  auto const expanded_other_sync = ExpandAggregateScopeBits(other.sync);

  auto const sync_covered = (expanded_scope_sync & expanded_other_sync) == expanded_other_sync;
  auto const access_covered = other.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                                ? true
                                : scope.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                                    ? false
                                    : (scope.access & other.access) == other.access;
  return sync_covered && access_covered;
}


constexpr auto Union(ResourceAccessScope const& scope1, ResourceAccessScope const& scope2) -> ResourceAccessScope {
  // NO_ACCESS cannot be combined with other bits, so it needs special handling.
  // scope1 == NO_ACCESS && scope2 == other -> scope2
  // scope1 == other && scope2 == NO_ACCESS -> scope1
  // scope1 == scope2 == NO_ACCESS -> NO_ACCESS

  return ResourceAccessScope{
    .sync = scope1.sync | scope2.sync,
    .access = scope1.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                ? scope2.access
                : scope2.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                    ? scope1.access
                    : scope1.access | scope2.access
  };
}
}
