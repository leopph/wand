#include "wand/buffer.hpp"

using Microsoft::WRL::ComPtr;

namespace wand {
auto Buffer::GetDesc() const -> BufferDesc const& {
  return desc_;
}


auto Buffer::GetConstantBuffer() const -> UINT {
  return cbv_.value();
}


Buffer::Buffer(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource2> resource, std::optional<UINT> const cbv,
               std::optional<UINT> const srv, std::optional<UINT> const uav, BufferDesc const& desc) :
  Resource{std::move(allocation), std::move(resource), srv, uav},
  desc_{desc},
  cbv_{cbv} {
}
}
