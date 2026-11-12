#pragma once

#include <functional>

#include <wand/buffer.hpp>
#include <wand/buffer_region.hpp>


namespace wand {
class BufferRange {
public:
  explicit inline
  BufferRange(Buffer const& buffer, BufferRegion const& region = {});
  explicit
  BufferRange(Buffer const&& buffer, BufferRegion const& region = {}) = delete;

  [[nodiscard]] constexpr
  auto GetBuffer() const -> Buffer const&;

  [[nodiscard]] constexpr
  auto GetRegion() const -> BufferRegion const&;

private:
  std::reference_wrapper<Buffer const> buffer_;
  BufferRegion region_;
};
}


#include <wand/buffer_range.inl>
