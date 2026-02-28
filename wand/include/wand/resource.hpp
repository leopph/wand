#pragma once

#include <string_view>

#include <wand/platforms/d3d12.hpp>

namespace wand {
class GraphicsDevice;


class Resource {
public:
  auto SetDebugName(std::wstring_view name) const -> void;

  [[nodiscard]]
  auto Map() const -> void*;
  auto Unmap() const -> void;

  [[nodiscard]]
  auto GetInternalResource() const -> ID3D12Resource2*;

protected:
  Resource(Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation, Microsoft::WRL::ComPtr<ID3D12Resource2> resource);

  [[nodiscard]] auto InternalMap(UINT subresource, D3D12_RANGE const* read_range) const -> void*;
  auto InternalUnmap(UINT subresource, D3D12_RANGE const* written_range) const -> void;

private:
  Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation_;
  Microsoft::WRL::ComPtr<ID3D12Resource2> resource_;

  friend GraphicsDevice;
};
}
