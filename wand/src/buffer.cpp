#include "wand/buffer.hpp"

using Microsoft::WRL::ComPtr;

namespace wand {
auto Buffer::GetDesc() const -> BufferDesc const& {
  return desc_;
}


Buffer::Buffer(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource2> resource, BufferDesc const& desc) :
  Resource{std::move(allocation), std::move(resource)},
  desc_{desc} {
}
}
