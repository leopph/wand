#pragma once

namespace wand {
class Texture {
protected:
  Texture() = default;

public:
  Texture(Texture const& other) = delete;
  Texture(Texture&& other) = delete;

  virtual ~Texture() = default;

  auto operator=(Texture const& other) -> void = delete;
  auto operator=(Texture&& other) -> void = delete;

  enum class Dimension {
    k1D,
    k2D,
    k3D
  };
};
}
