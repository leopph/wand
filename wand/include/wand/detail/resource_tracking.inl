#pragma once


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


template<typename ResourceStateType> template<typename Self>
auto ResourceStateTracker<ResourceStateType>::Get(this Self& self, ID3D12Resource* resource) -> decltype(auto) {
  auto const it = self.resource_states_.find(resource);
  return it != self.resource_states_.end() ? &it->second : nullptr;
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::Clear() -> void {
  resource_states_.clear();
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::begin() const {
  return resource_states_.begin();
}


template<typename ResourceStateType>
auto ResourceStateTracker<ResourceStateType>::end() const {
  return resource_states_.end();
}
}
