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

  template<typename Self>
  [[nodiscard]]
  auto Get(this Self& self, ID3D12Resource* resource) -> decltype(auto);

  auto Clear() -> void;

  [[nodiscard]]
  auto begin() const;

  [[nodiscard]]
  auto end() const;

private:
  std::unordered_map<ID3D12Resource*, ResourceStateType> resource_states_;
};


struct AccessScope {
  D3D12_BARRIER_SYNC sync{D3D12_BARRIER_SYNC_NONE};
  D3D12_BARRIER_ACCESS access{D3D12_BARRIER_ACCESS_NO_ACCESS};
};


[[nodiscard]]
auto GetSupersededScopeBits(D3D12_BARRIER_SYNC sync) -> D3D12_BARRIER_SYNC;

[[nodiscard]]
auto ExpandAggregateScopeBits(D3D12_BARRIER_SYNC sync) -> D3D12_BARRIER_SYNC;

[[nodiscard]]
auto CoversSync(D3D12_BARRIER_SYNC current, D3D12_BARRIER_SYNC requested) -> bool;

[[nodiscard]]
auto Covers(AccessScope const& scope, AccessScope const& other) -> bool;

[[nodiscard]]
auto IsEmpty(AccessScope const& scope) -> bool;

[[nodiscard]]
auto Union(AccessScope const& scope1, AccessScope const& scope2) -> AccessScope;


struct TextureAccess {
  AccessScope scope;
  D3D12_BARRIER_LAYOUT layout;
};


struct BufferTrackingState {
  AccessScope first_scope;
  bool first_scope_closed;
  AccessScope latest_scope;
  bool written_in_list;
};


struct TextureTrackingState {
  TextureAccess first_access;
  bool first_access_scope_closed;
  TextureAccess latest_access;
  bool modified_in_list;
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
auto ClassifyTransition(AccessScope const& current, AccessScope const& requested,
                        bool has_prior_modification) -> AccessTransition;

[[nodiscard]]
auto ClassifyTransition(TextureAccess const& current, TextureAccess const& requested,
                        bool has_prior_modification) -> AccessTransition;


constexpr auto kSealedLayout = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
}


#include <wand/detail/resource_tracking.inl>
