#pragma once

#include <algorithm>
#include <limits>
#include <utility>

namespace wand {
template<std::integral To, std::integral From>
constexpr auto ClampCast(From const what) -> To {
  if constexpr (std::cmp_less_equal(std::numeric_limits<To>::min(), std::numeric_limits<From>::min())) {
    if constexpr (std::cmp_greater_equal(std::numeric_limits<To>::max(), std::numeric_limits<From>::max())) {
      return static_cast<To>(what);
    } else {
      return static_cast<To>(std::min<From>(what, std::numeric_limits<To>::max()));
    }
  } else {
    if constexpr (std::cmp_greater_equal(std::numeric_limits<To>::max(), std::numeric_limits<From>::max())) {
      return static_cast<To>(std::max<From>(what, std::numeric_limits<To>::min()));
    } else {
      return static_cast<To>(std::clamp<From>(what, std::numeric_limits<To>::min(), std::numeric_limits<To>::max()));
    }
  }
}
}
