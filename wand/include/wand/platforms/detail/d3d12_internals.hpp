#pragma once

#include <wand/platforms/d3d12.hpp>


namespace wand::d3d12::detail {
[[nodiscard]]
auto IsWriteAccess(D3D12_BARRIER_ACCESS access) -> bool;

constexpr static UINT kDrawCallParamsRootIdx{0};
constexpr static UINT kDrawParamsRootIdx{1};
}
