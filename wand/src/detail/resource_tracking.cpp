#include "wand/detail/resource_tracking.hpp"

#include "wand/platforms/detail/d3d12_internals.hpp"


namespace wand::detail {
auto GetSupersededScopeBits(D3D12_BARRIER_SYNC const sync) -> D3D12_BARRIER_SYNC {
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


auto ExpandAggregateScopeBits(D3D12_BARRIER_SYNC const sync) -> D3D12_BARRIER_SYNC {
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


auto CoversSync(D3D12_BARRIER_SYNC const current, D3D12_BARRIER_SYNC const requested) -> bool {
  // Aggregate sync bits are treated as though all of their superseded bits are set.

  auto const expanded_current = ExpandAggregateScopeBits(current);
  auto const expandend_requested = ExpandAggregateScopeBits(requested);
  return (expanded_current & expandend_requested) == expandend_requested;
}


auto Covers(AccessScope const& scope, AccessScope const& other) -> bool {
  // NO_ACCESS != 0, so it needs special handling.
  // NO_ACCESS is covered by everything.
  // NO_ACCESS covers nothing but NO_ACCESS.

  auto const access_covered = other.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                                ? true
                                : scope.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                                    ? false
                                    : (scope.access & other.access) == other.access;
  return CoversSync(scope.sync, other.sync) && access_covered;
}


auto IsEmpty(AccessScope const& scope) -> bool {
  return scope.sync == D3D12_BARRIER_SYNC_NONE && scope.access == D3D12_BARRIER_ACCESS_NO_ACCESS;
}


auto Union(AccessScope const& scope1, AccessScope const& scope2) -> AccessScope {
  // NO_ACCESS cannot be combined with other bits, so it needs special handling.
  // scope1 == NO_ACCESS && scope2 == other -> scope2
  // scope1 == other && scope2 == NO_ACCESS -> scope1
  // scope1 == scope2 == NO_ACCESS -> NO_ACCESS

  return AccessScope{
    .sync = scope1.sync | scope2.sync,
    .access = scope1.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                ? scope2.access
                : scope2.access == D3D12_BARRIER_ACCESS_NO_ACCESS
                    ? scope1.access
                    : scope1.access | scope2.access
  };
}


auto ClassifyTransition(AccessScope const& current, AccessScope const& requested,
                        bool const has_prior_modification) -> AccessTransition {
  // Modification is write or layout change. From the perspective of barriers, a layout change is also a write.
  // The bytes in the memory change.

  // Any use -> no use.
  if (IsEmpty(requested)) {
    return AccessTransition::kAlreadyCovered;
  }
  // No use -> any use.
  if (IsEmpty(current)) {
    return AccessTransition::kMergeWithoutBarrier;
  }
  // Write->read, read->write, or write->write
  if (d3d12::detail::IsWriteAccess(current.access) || d3d12::detail::IsWriteAccess(requested.access)) {
    return AccessTransition::kPlaceBarrier;
  }
  // Read->read
  if (!has_prior_modification) {
    return AccessTransition::kMergeWithoutBarrier;
  }
  // Write->read->read where the last barrier already covered the requested usage
  if (Covers(current, requested)) {
    return AccessTransition::kAlreadyCovered;
  }
  // Write->read->read where the requested usage was not covered by the last barrier
  return AccessTransition::kExtendLatestBarrier;
}


auto ClassifyTransition(TextureAccess const& current, TextureAccess const& requested,
                        bool const has_prior_modification) -> AccessTransition {
  if (current.layout != requested.layout) {
    return AccessTransition::kPlaceBarrier;
  }
  // Subsequent OM writes don't need a barrier.
  if (current.scope.access == requested.scope.access && d3d12::detail::IsOutputMergerWrite(requested.scope.access)) {
    return AccessTransition::kMergeWithoutBarrier;
  }
  return ClassifyTransition(current.scope, requested.scope, has_prior_modification);
}
}
