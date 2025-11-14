#pragma once

#include <wand/platforms/d3d12.hpp>

namespace wand {
class Texture;
class Buffer;

struct GlobalBarrier {
  D3D12_BARRIER_SYNC sync_before;
  D3D12_BARRIER_SYNC sync_after;
  D3D12_BARRIER_ACCESS access_before;
  D3D12_BARRIER_ACCESS access_after;
};


struct TextureBarrier {
  D3D12_BARRIER_SYNC sync_before;
  D3D12_BARRIER_SYNC sync_after;
  D3D12_BARRIER_ACCESS access_before;
  D3D12_BARRIER_ACCESS access_after;
  D3D12_BARRIER_LAYOUT layout_before;
  D3D12_BARRIER_LAYOUT layout_after;
  Texture const* texture;
  D3D12_BARRIER_SUBRESOURCE_RANGE subresources;
  D3D12_TEXTURE_BARRIER_FLAGS flags;
};


struct BufferBarrier {
  D3D12_BARRIER_SYNC sync_before;
  D3D12_BARRIER_SYNC sync_after;
  D3D12_BARRIER_ACCESS access_before;
  D3D12_BARRIER_ACCESS access_after;
  Buffer const* buffer;
  UINT64 offset;
  UINT64 size;
};
}
