#pragma once

namespace wand {
class Texture {
public:
  enum class Dimension {
    k1D,
    k2D,
    k3D
  };

  struct Desc {
    int width;
    int height;
    int mip_count;
    int array_size;
    // format
    Dimension dimension;
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
}
