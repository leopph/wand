#pragma once

#include <type_traits>


namespace wand {
template<typename FlagType> requires std::is_enum_v<FlagType>
[[nodiscard]] constexpr
auto HasAny(FlagType flags, FlagType test_flags) noexcept -> bool;

template<typename FlagType> requires std::is_enum_v<FlagType>
[[nodiscard]] constexpr
auto HasAll(FlagType flags, FlagType test_flags) noexcept -> bool;

template<typename FlagType> requires std::is_enum_v<FlagType>
[[nodiscard]] constexpr
auto operator|(FlagType lhs, FlagType rhs) noexcept -> FlagType;

template<typename FlagType> requires std::is_enum_v<FlagType>
constexpr
auto operator|=(FlagType& lhs, FlagType rhs) noexcept -> FlagType&;
}


#include "flags.inl"
