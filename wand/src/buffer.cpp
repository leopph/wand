#include "wand/buffer.hpp"

using Microsoft::WRL::ComPtr;


namespace wand {
auto Buffer::GetDesc() const -> BufferDesc const& {
  return desc_;
}


auto Buffer::GetAccelerationStructure() const -> UINT {
  return as_.value();
}


Buffer::Buffer(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource2> resource, std::optional<UINT> const as,
               BufferDesc const& desc) :
  Resource{std::move(allocation), std::move(resource)},
  desc_{desc},
  as_{as} {}
}
