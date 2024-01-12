#pragma once

#include "core.hpp"

#include <cstdint>

namespace wand {
class Buffer {
public:
  enum Usage : std::uint8_t {
    kUsageNone = 0,
    kUsageConstantBuffer = 1 << 0,
    kUsageShaderResource = 1 << 1,
    kUsageUnorderedAccess = 1 << 2
  };

  struct Desc {
    std::uint32_t width;
    std::uint32_t stride;
    Usage usage;
    bool mappable;
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
  [[nodiscard]] WANDAPI virtual auto Map() const -> void* = 0;
  WANDAPI virtual auto Unmap() const -> void = 0;

private:
  Desc desc_;
};

DEFINE_ENUM_FLAG_OPS(Buffer::Usage)

}
