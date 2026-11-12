#pragma once

#include <wand/buffer.hpp>
#include <wand/buffer_range.hpp>


namespace wand::detail {
[[nodiscard]]
auto IsGpuWritable(BufferDesc const& desc) noexcept -> bool;

[[nodiscard]]
auto GetGpuAddress(BufferRange const& range) -> D3D12_GPU_VIRTUAL_ADDRESS;
}
