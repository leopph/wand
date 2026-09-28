#pragma once

#include <wand/platforms/d3d12.hpp>


namespace wand::d3d12::detail {
[[nodiscard]]
auto IsWriteAccess(D3D12_BARRIER_ACCESS access) -> bool;
}
