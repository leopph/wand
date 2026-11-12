#pragma once

#include <stdexcept>


namespace wand {
inline BufferRange::BufferRange(Buffer const& buffer, BufferRegion const& region) :
  buffer_{buffer},
  region_{ResolveRegion(region, buffer.GetDesc().size)} {
  if (!IsValidRegion(region_, buffer_.get().GetDesc().size)) {
    throw std::runtime_error{"Failed to create buffer range: region is empty or exceeds the buffer."};
  }
}


constexpr auto BufferRange::GetBuffer() const -> Buffer const& {
  return buffer_;
}


constexpr auto BufferRange::GetRegion() const -> BufferRegion const& {
  return region_;
}
}
