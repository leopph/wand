#pragma once

#include "platform_d3d12.hpp"
#include "texture.hpp"

namespace wand {
class TextureD3D12 final : public Texture {
  UINT width;
  UINT height;
  UINT depth;
  UINT arraySize;
  UINT mips;
  Dimension dim;

  int rtvIdx;
  int dsvIdx;
  int srvIdx;
  int uavIdx;

  D3D12MA::Allocation* alloc;

  DXGI_FORMAT format;
};
}
