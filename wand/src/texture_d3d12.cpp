#include "texture_d3d12.hpp"

#include <utility>

namespace wand {
TextureD3D12::TextureD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc) :
  Texture{desc}, alloc_{std::move(alloc)} {
}
}
