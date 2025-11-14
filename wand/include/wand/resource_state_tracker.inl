#pragma once

#include "resource_state_tracker.hpp"

#include <iterator>

namespace wand::details {
template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Record(ID3D12Resource* const resource,
                                                     ResourceStateType const state) -> void {
  resource_states_[resource] = state;
}

template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Get(
  ID3D12Resource* const resource) const -> std::optional<ResourceStateType> {
  if (auto const it{resource_states_.find(resource)}; it != std::end(resource_states_)) {
    return it->second;
  }

  return std::nullopt;
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
}
