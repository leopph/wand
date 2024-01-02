#include "texture.hpp"

namespace wand {
Texture::Texture(Desc const& desc) :
  desc_{desc} {
}

auto Texture::GetDesc() const -> Desc const& {
  return desc_;
}
}
