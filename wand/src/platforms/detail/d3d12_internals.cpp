#include "wand/platforms/detail/d3d12_internals.hpp"


namespace wand::d3d12::detail {
namespace {
constexpr D3D12_BARRIER_ACCESS kWriteAccess =
  D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
  D3D12_BARRIER_ACCESS_STREAM_OUTPUT |
  D3D12_BARRIER_ACCESS_COPY_DEST |
  D3D12_BARRIER_ACCESS_RESOLVE_DEST |
  D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
}


auto IsWriteAccess(D3D12_BARRIER_ACCESS const access) -> bool {
  return (access & kWriteAccess) != 0;
}
}
