#pragma once

namespace wand {
constexpr auto GraphicsDeviceCapabilities::SupportsMeshShaders() const -> bool {
  return mesh_shader_tier >= D3D12_MESH_SHADER_TIER_1;
}


constexpr auto GraphicsDeviceCapabilities::SupportsRaytracing() const -> bool {
  return raytracing_tier >= D3D12_RAYTRACING_TIER_1_0;
}


constexpr auto GraphicsDeviceCapabilities::SupportsInlineRaytracing() const -> bool {
  return raytracing_tier >= D3D12_RAYTRACING_TIER_1_1;
}
}
