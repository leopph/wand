#pragma once

#include <wand/buffer.hpp>


namespace wand::detail {
[[nodiscard]]
auto IsGpuWritable(BufferDesc const& desc) noexcept -> bool;
}
