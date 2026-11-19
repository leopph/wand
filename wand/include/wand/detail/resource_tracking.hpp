#pragma once

#include <cstdint>
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


[[nodiscard]]
auto GetSupersededScopeBits(D3D12_BARRIER_SYNC sync) -> D3D12_BARRIER_SYNC;

[[nodiscard]]
auto ExpandAggregateScopeBits(D3D12_BARRIER_SYNC sync) -> D3D12_BARRIER_SYNC;

[[nodiscard]]
auto Covers(ResourceAccessScope const& scope, ResourceAccessScope const& other) -> bool;

[[nodiscard]]
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


enum class AccessTransition : std::uint8_t {
  // The latest barrier already covers the request: nothing to do.
  kAlreadyCovered,
  // No hazard and nothing to make visible: OR the request into the tracked scope.
  kMergeWithoutBarrier,
  // A new kind of read after a write: extend the latest barrier's after scope (widen it if it's still pending, else
  // chain a new one).
  kExtendLatestBarrier,
  // Layout change or a write involved: barrier from the tracked scope to the request.
  kPlaceBarrier,
};


[[nodiscard]]
auto ClassifyTransition(ResourceAccessScope const& current, ResourceAccessScope const& requested,
                        bool written) -> AccessTransition;

[[nodiscard]]
auto ClassifyTransition(TextureAccess const& current, TextureAccess const& requested, bool written) -> AccessTransition;
}


#include <wand/detail/resource_tracking.inl>
