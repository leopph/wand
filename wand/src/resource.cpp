#include "wand/resource.hpp"

#include "wand/common.hpp"

using Microsoft::WRL::ComPtr;

namespace wand {
auto Resource::SetDebugName(std::wstring_view const name) const -> void {
  ThrowIfFailed(resource_->SetName(name.data()), "Failed to set D3D12 resource debug name.");
}


auto Resource::Map() const -> void* {
  return InternalMap(0, nullptr);
}


auto Resource::Unmap() const -> void {
  InternalUnmap(0, nullptr);
}


auto Resource::GetShaderResource() const -> UINT {
  return srv_.value();
}


auto Resource::GetUnorderedAccess() const -> UINT {
  return uav_.value();
}


auto Resource::GetInternalResource() const -> ID3D12Resource2* {
  return resource_.Get();
}


Resource::Resource(ComPtr<D3D12MA::Allocation> allocation, ComPtr<ID3D12Resource2> resource,
                   std::optional<UINT> const srv, std::optional<UINT> const uav) :
  allocation_{std::move(allocation)},
  resource_{std::move(resource)},
  srv_{srv},
  uav_{uav} {
}


auto Resource::InternalMap(UINT const subresource, D3D12_RANGE const* read_range) const -> void* {
  void* mapped;
  ThrowIfFailed(resource_->Map(subresource, read_range, &mapped), "Failed to map D3D12 resource.");
  return mapped;
}


auto Resource::InternalUnmap(UINT const subresource, D3D12_RANGE const* written_range) const -> void {
  resource_->Unmap(subresource, written_range);
}
}
