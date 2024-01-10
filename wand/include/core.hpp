#pragma once

#include <type_traits>

#ifdef WAND_EXPORT
#define WANDAPI __declspec(dllexport)
#else
#define WANDAPI __declspec(dllimport)
#endif

#define DEFINE_ENUM_FLAG_OPS(EnumType)\
[[nodiscard]] constexpr inline auto operator~(EnumType const self) -> EnumType { return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(self)); } \
[[nodiscard]] constexpr inline auto operator&(EnumType const left, EnumType const right) -> EnumType { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(left) & static_cast<std::underlying_type_t<EnumType>>(right)); }\
[[nodiscard]] constexpr inline auto operator|(EnumType const left, EnumType const right) -> EnumType { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(left) | static_cast<std::underlying_type_t<EnumType>>(right)); }\
[[nodiscard]] constexpr inline auto operator^(EnumType const left, EnumType const right) -> EnumType { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(left) ^ static_cast<std::underlying_type_t<EnumType>>(right)); }\
[[nodiscard]] inline auto operator&=(EnumType& left, EnumType const right) -> EnumType& { return reinterpret_cast<EnumType&>(reinterpret_cast<std::underlying_type_t<EnumType>&>(left) &= static_cast<std::underlying_type_t<EnumType>>(right)); }\
[[nodiscard]] inline auto operator|=(EnumType& left, EnumType const right) -> EnumType& { return reinterpret_cast<EnumType&>(reinterpret_cast<std::underlying_type_t<EnumType>&>(left) |= static_cast<std::underlying_type_t<EnumType>>(right)); }\
[[nodiscard]] inline auto operator^=(EnumType& left, EnumType const right) -> EnumType& { return reinterpret_cast<EnumType&>(reinterpret_cast<std::underlying_type_t<EnumType>&>(left) ^= static_cast<std::underlying_type_t<EnumType>>(right)); }
