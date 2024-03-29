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

namespace wand {
enum class Format : std::uint8_t {
  kUnknown = 0,
  kR32G32B32A32Float = 2,
  kR32G32B32A32Uint = 3,
  kR32G32B32A32Sint = 4,
  kR32G32B32Float = 6,
  kR32G32B32Uint = 7,
  kR32G32B32Sint = 8,
  kR16G16B16A16Float = 10,
  kR16G16B16A16Unorm = 11,
  kR16G16B16A16Uint = 12,
  kR16G16B16A16Sint = 13,
  kR32G32Float = 16,
  kR32G32Uint = 17,
  kR32G32Sint = 18,
  kD32FloatS8X24Uint = 20,
  kR10G10B10A2Typeless = 23,
  kR10G10B10A2Unorm = 24,
  kR10G10B10A2Uint = 25,
  kR11G11B10Float = 26,
  kR8G8B8A8Unorm = 28,
  kR8G8B8A8UnormSrgb = 29,
  kR8G8B8A8Uint = 30,
  kR8G8B8A8Snorm = 31,
  kR8G8B8A8Sint = 32,
  kR16G16Float = 34,
  kR16G16Unorm = 35,
  kR16G16Uint = 36,
  kR16G16Snorm = 37,
  kR16G16Sint = 38,
  kD32Float = 40,
  kR32Float = 41,
  kR32Uint = 42,
  kR32Sint = 43,
  kD24UnormS8Uint = 45,
  kR24UnormX8Typeless = 46,
  kX24TypelessG8Uint = 47,
  kR8G8Unorm = 49,
  kR8G8Uint = 50,
  kR8G8Snorm = 51,
  kR8G8Sint = 52,
  kR16Float = 54,
  kD16Unorm = 55,
  kR16Unorm = 56,
  kR16Uint = 57,
  kR16Snorm = 58,
  kR16Sint = 59,
  kR8Unorm = 61,
  kR8Uint = 62,
  kR8Snorm = 63,
  kR8Sint = 64,
  kA8Unorm = 65,
  kR1Unorm = 66,
  kR9G9B9E5Sharedexp = 67,
  kR8G8B8G8Unorm = 68,
  kG8R8G8B8Unorm = 69,
  kBc1Unorm = 71,
  kBc1UnormSrgb = 72,
  kBc2Unorm = 74,
  kBc2UnormSrgb = 75,
  kBc3Unorm = 77,
  kBc3UnormSrgb = 78,
  kBc4Unorm = 80,
  kBc4Snorm = 81,
  kBc5Unorm = 83,
  kBc5Snorm = 84,
  kB5G6R5Unorm = 85,
  kB5G5R5A1Unorm = 86,
  kB8G8R8A8Unorm = 87,
  kB8G8R8X8Unorm = 88,
  kR10G10B10XrBiasA2Unorm = 89,
  kB8G8R8A8UnormSrgb = 91,
  kB8G8R8X8UnormSrgb = 93,
  kBc6HUf16 = 95,
  kBc6HSf16 = 96,
  kBc7Unorm = 98,
  kBc7UnormSrgb = 99,
  kAyuv = 100,
  kY410 = 101,
  kY416 = 102,
  kNv12 = 103,
  kP010 = 104,
  kP016 = 105,
  k420Opaque = 106,
  kYuy2 = 107,
  kY210 = 108,
  kY216 = 109,
  kNv11 = 110,
  kAi44 = 111,
  kIa44 = 112,
  kP8 = 113,
  kA8P8 = 114,
  kB4G4R4A4Unorm = 115,
  kP208 = 130,
  kV208 = 131,
  kV408 = 132,
  kSamplerFeedbackMinMipOpaque,
  kSamplerFeedbackMipRegionUsedOpaque,
};
}
