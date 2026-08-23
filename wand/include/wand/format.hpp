#pragma once

#include <wand/wandapi.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
// Convert a depth format to its typeless equivalent.
[[nodiscard]] WANDAPI
auto MakeDepthTypeless(DXGI_FORMAT depth_format) -> DXGI_FORMAT;

// Convert a depth format to its underlying linear equivalent.
[[nodiscard]] WANDAPI
auto MakeDepthUnderlyingLinear(DXGI_FORMAT depth_format) -> DXGI_FORMAT;
}
