#pragma once

#include <cstdint>


namespace wand {
constexpr auto kRemainingSize = ~std::uint64_t{0};


struct BufferRegion {
  std::uint64_t offset{0};
  std::uint64_t size{kRemainingSize};
  std::uint32_t stride{0};
};


[[nodiscard]] constexpr
auto ResolveRegion(BufferRegion const& region, std::uint64_t buffer_size) -> BufferRegion;

[[nodiscard]] constexpr
auto IsValidRegion(BufferRegion const& region, std::uint64_t buffer_size) -> bool;
}


#include <wand/buffer_region.inl>
