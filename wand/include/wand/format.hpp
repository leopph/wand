#pragma once

#include <wand/platforms/d3d12.hpp>

namespace wand {
// Convert a depth format to its typeless equivalent.
[[nodiscard]] auto MakeDepthTypeless(DXGI_FORMAT depth_format) -> DXGI_FORMAT;
// Convert a depth format to its underlying linear equivalent.
[[nodiscard]] auto MakeDepthUnderlyingLinear(DXGI_FORMAT depth_format) -> DXGI_FORMAT;
}
