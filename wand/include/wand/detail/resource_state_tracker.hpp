#pragma once

#include <unordered_map>

#include <wand/platforms/d3d12.hpp>


namespace wand::detail {
template<typename ResourceStateType>
class ResourceStateTracker {
public:
  auto Record(ID3D12Resource* resource, ResourceStateType state) -> void;

  auto Erase(ID3D12Resource* resource) -> void;

  [[nodiscard]]
  auto Get(ID3D12Resource* resource) -> ResourceStateType*;

  auto Clear() -> void;

  [[nodiscard]]
  auto begin() const;

  [[nodiscard]]
  auto end() const;

private:
  std::unordered_map<ID3D12Resource*, ResourceStateType> resource_states_;
};


struct ResourceAccessScope {
  D3D12_BARRIER_SYNC sync{D3D12_BARRIER_SYNC_NONE};
  D3D12_BARRIER_ACCESS access{D3D12_BARRIER_ACCESS_NO_ACCESS};
};


[[nodiscard]] constexpr
auto Covers(ResourceAccessScope const& scope, ResourceAccessScope const& other) -> bool;

[[nodiscard]] constexpr
auto Union(ResourceAccessScope const& scope1, ResourceAccessScope const& scope2) -> ResourceAccessScope;


struct TextureAccess {
  ResourceAccessScope scope;
  D3D12_BARRIER_LAYOUT layout;
};


struct PipelineBufferState {
  ResourceAccessScope current;
  bool written;
};


struct PipelineTextureState {
  TextureAccess current;
  bool written;
};


struct GlobalResourceState {
  D3D12_BARRIER_LAYOUT layout{D3D12_BARRIER_LAYOUT_UNDEFINED};
};


using GlobalResourceStateTracker = ResourceStateTracker<GlobalResourceState>;
}


#include <wand/detail/resource_state_tracker.inl>
