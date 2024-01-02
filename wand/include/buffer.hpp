#pragma once

namespace wand {
class Buffer {
public:
  struct Desc {
    int width;
  };

protected:
  explicit Buffer(Desc const& desc);

public:
  Buffer(Buffer const& other) = delete;
  Buffer(Buffer&& other) = delete;

  virtual ~Buffer() = default;

  auto operator=(Buffer const& other) -> void = delete;
  auto operator=(Buffer&& other) -> void = delete;

  [[nodiscard]] auto GetDesc() const -> Desc const&;

private:
  Desc desc_;
};
}
