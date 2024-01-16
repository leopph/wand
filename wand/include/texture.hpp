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
