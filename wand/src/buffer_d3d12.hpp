#pragma once

#include "platform_d3d12.hpp"
#include "buffer.hpp"

namespace wand {
class BufferD3D12 final : public Buffer {
  Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc_;

public:
  BufferD3D12(Desc const& desc, Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc);
};
}
