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


auto Covers(ResourceAccessScope const& scope, ResourceAccessScope const& other) -> bool {
  // NO_ACCESS != 0, so it needs special handling.
  // NO_ACCESS is covered by everything.
  // NO_ACCESS covers nothing but NO_ACCESS.

  // Aggregate sync bits are treated as though all of their superseded bits are set.

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


auto Union(ResourceAccessScope const& scope1, ResourceAccessScope const& scope2) -> ResourceAccessScope {
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


auto ClassifyTransition(ResourceAccessScope const& current, ResourceAccessScope const& requested,
                        bool const written) -> AccessTransition {
  // Write->read, read->write, or write->write
  if (d3d12::detail::IsWriteAccess(current.access) || d3d12::detail::IsWriteAccess(requested.access)) {
    return AccessTransition::kPlaceBarrier;
  }
  // Read->read
  if (!written) {
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
                        bool const written) -> AccessTransition {
  // This is stricter than necessary but safe. There could be layouts that aren't equal but still compatible.
  if (current.layout != requested.layout) {
    return AccessTransition::kPlaceBarrier;
  }
  // Subsequent OM writes don't need a barrier.
  if (current.scope.access == requested.scope.access && d3d12::detail::IsOutputMergerWrite(requested.scope.access)) {
    return AccessTransition::kMergeWithoutBarrier;
  }
  return ClassifyTransition(current.scope, requested.scope, written);
}
}
