#pragma once

#include <optional>
#include <unordered_map>

#include <wand/platforms/d3d12.hpp>


namespace wand::detail {
template<typename ResourceStateType>
class ResourceStateTracker {
public:
  auto Record(ID3D12Resource* resource, ResourceStateType state) -> void;

  auto Erase(ID3D12Resource* resource) -> void;

  [[nodiscard]]
  auto Get(ID3D12Resource* resource) const -> std::optional<ResourceStateType>;

  auto Clear() -> void;

  [[nodiscard]]
  auto begin() const;

  [[nodiscard]]
  auto end() const;

private:
  std::unordered_map<ID3D12Resource*, ResourceStateType> resource_states_;
};


struct GlobalResourceState {
  D3D12_BARRIER_LAYOUT layout{D3D12_BARRIER_LAYOUT_UNDEFINED};
};


struct PipelineResourceState {
  D3D12_BARRIER_SYNC accum_sync{D3D12_BARRIER_SYNC_NONE};
  D3D12_BARRIER_ACCESS accum_access{D3D12_BARRIER_ACCESS_NO_ACCESS};
  D3D12_BARRIER_LAYOUT layout{D3D12_BARRIER_LAYOUT_UNDEFINED};
};


using GlobalResourceStateTracker = ResourceStateTracker<GlobalResourceState>;
using PipelineResourceStateTracker = ResourceStateTracker<PipelineResourceState>;
}


#include <wand/detail/resource_state_tracker.inl>
