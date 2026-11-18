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


constexpr auto Covers(ResourceAccessScope const& scope, ResourceAccessScope const& other) -> bool {
  // NO_ACCESS != 0, so it needs special handling.
  // NO_ACCESS is covered by everything.
  // NO_ACCESS covers nothing but NO_ACCESS.

  auto const sync_covered = (scope.sync & other.sync) == other.sync;
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
