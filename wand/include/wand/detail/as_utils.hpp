#pragma once

#include <vector>

#include <wand/command_list.hpp>


namespace wand::detail {
[[nodiscard]]
auto ToD3D12(
  BuildAccelerationStructureInputs const& inputs,
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometry_storage
) -> D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS;
}
