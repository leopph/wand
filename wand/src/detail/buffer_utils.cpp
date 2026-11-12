#include <wand/detail/buffer_utils.hpp>

#include "wand/flags.hpp"


namespace wand::detail {
auto IsGpuWritable(BufferDesc const& desc) noexcept -> bool {
  constexpr auto writes =
    BufferUsage::kUnorderedAccess |
    BufferUsage::kCopyDestination |
    BufferUsage::kAccelerationStructure |
    BufferUsage::kAccelerationScratch;

  return HasAny(desc.usage, writes);
}


auto GetGpuAddress(BufferRange const& range) -> D3D12_GPU_VIRTUAL_ADDRESS {
  return range.GetBuffer().GetInternalResource()->GetGPUVirtualAddress() + range.GetRegion().offset;
}
}
