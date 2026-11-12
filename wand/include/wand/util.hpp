#pragma once

#include <concepts>


namespace wand {
template<std::integral To, std::integral From>
[[nodiscard]] constexpr auto ClampCast(From what) -> To;


template<class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
}


#include <wand/util.inl>
