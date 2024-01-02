#include "buffer.hpp"

namespace wand {
Buffer::Buffer(Desc const& desc) :
  desc_{desc} {
}

auto Buffer::GetDesc() const -> Desc const& {
  return desc_;
}
}
