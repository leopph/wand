#pragma once

#include <cstdint>
#include <optional>

#include <wand/resource.hpp>
#include <wand/wandapi.hpp>


namespace wand {
enum class BufferUsage : std::uint32_t {
  kNone = 0,

  kConstantBuffer  = 1 << 0,
  kShaderResource  = 1 << 1,
  kUnorderedAccess = 1 << 2,

  kVertexBuffer     = 1 << 3,
  kIndexBuffer      = 1 << 4,
  kIndirectArgument = 1 << 5,

  kCopySource      = 1 << 6,
  kCopyDestination = 1 << 7,

  kAccelerationStructure = 1 << 8,
  kAccelerationScratch   = 1 << 9,
};


struct BufferDesc {
  UINT64 size;
  BufferUsage usage;
};


class Buffer : public Resource {
public:
  [[nodiscard]] WANDAPI
  auto GetDesc() const -> BufferDesc const&;

  [[nodiscard]] WANDAPI
  auto GetAccelerationStructure() const -> UINT;

private:
  Buffer(Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation, Microsoft::WRL::ComPtr<ID3D12Resource2> resource,
         std::optional<UINT> as, BufferDesc const& desc);

  BufferDesc desc_;
  std::optional<UINT> as_;

  friend GraphicsDevice;
};
}
