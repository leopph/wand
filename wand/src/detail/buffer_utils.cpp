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
}
