#pragma once

#include "core.hpp"

#include <array>
#include <cstdint>

namespace wand {
class Texture {
public:
  enum class Dimension : std::uint8_t {
    k1D,
    k2D,
    k3D,
    kCube
  };

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

  enum Usage : std::uint8_t {
    kUsageNone = 0,
    kUsageShaderResource = 1 << 0,
    kUsageUnorderedAccess = 1 << 1,
    kUsageRenderTarget = 1 << 2,
    kUsageDepthStencil = 1 << 3,
  };

  struct Desc {
    std::uint32_t width;
    std::uint32_t height;
    std::uint16_t mip_count;
    std::uint16_t depth_or_array_size;
    std::uint32_t sample_count;
    std::uint32_t sample_quality;
    std::array<float, 4> render_target_clear_value;
    float depth_clear_value;
    std::uint8_t stencil_clear_value;
    Format format;
    Dimension dimension;
    Usage usage;
  };

protected:
  explicit Texture(Desc const& desc);

public:
  Texture(Texture const& other) = delete;
  Texture(Texture&& other) = delete;

  virtual ~Texture() = default;

  auto operator=(Texture const& other) -> void = delete;
  auto operator=(Texture&& other) -> void = delete;

  [[nodiscard]] auto GetDesc() const -> Desc const&;

private:
  Desc desc_;
};

DEFINE_ENUM_FLAG_OPS(Texture::Usage)
}
