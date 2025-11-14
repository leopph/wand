#pragma once

#include <concepts>

namespace wand {
template<std::integral To, std::integral From>
[[nodiscard]] constexpr auto ClampCast(From what) -> To;
}

#include <wand/util.inl>
