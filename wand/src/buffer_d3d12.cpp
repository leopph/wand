#include "buffer_d3d12.hpp"

#include <utility>

namespace wand {
BufferD3D12::BufferD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc) :
  Buffer{desc},
  alloc_{std::move(alloc)} {
}
}
