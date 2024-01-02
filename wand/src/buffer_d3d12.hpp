#pragma once

#include "platform_d3d12.hpp"
#include "buffer.hpp"

namespace wand {
class BufferD3D12 final : public Buffer {
  UINT size;

  int srvIdx;
  int uavIdx;
  int cbvIdx;

  D3D12MA::Allocation* alloc;
};
}
