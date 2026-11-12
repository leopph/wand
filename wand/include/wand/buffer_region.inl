#pragma once

namespace wand {
constexpr auto ResolveRegion(BufferRegion const& region, std::uint64_t const buffer_size) -> BufferRegion {
  auto ret = auto(region);

  if (region.size == kRemainingSize) {
    ret.size = region.offset < buffer_size
                 ? buffer_size - region.offset
                 : std::uint64_t{0};
  }

  return ret;
}


constexpr auto IsValidRegion(BufferRegion const& region, std::uint64_t const buffer_size) -> bool {
  return region.size != 0 && region.offset < buffer_size && region.size <= buffer_size - region.offset;
}
}
