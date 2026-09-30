#pragma once

#include <utility>


namespace wand {
template<typename FlagType> requires std::is_enum_v<FlagType>
constexpr
auto HasAny(FlagType const flags, FlagType const test_flags) noexcept -> bool {
  return (std::to_underlying(flags) & std::to_underlying(test_flags)) != 0;
}


template<typename FlagType> requires std::is_enum_v<FlagType>
constexpr
auto HasAll(FlagType const flags, FlagType const test_flags) noexcept -> bool {
  return (std::to_underlying(flags) & std::to_underlying(test_flags)) == std::to_underlying(test_flags);
}


template<typename FlagType> requires std::is_enum_v<FlagType>
constexpr
auto operator|(FlagType const lhs, FlagType const rhs) noexcept -> FlagType {
  return static_cast<FlagType>(std::to_underlying(lhs) | std::to_underlying(rhs));
}


template<typename FlagType> requires std::is_enum_v<FlagType>
constexpr 
auto operator|=(FlagType& lhs, FlagType const rhs) noexcept -> FlagType& {
  lhs = lhs | rhs;
  return lhs;
}
}
