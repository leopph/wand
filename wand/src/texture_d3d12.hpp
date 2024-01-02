#pragma once

#include "platform_d3d12.hpp"
#include "texture.hpp"

namespace wand {
class TextureD3D12 final : public Texture {
public:
  TextureD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc);

private:
  Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc_;
};
}
