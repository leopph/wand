#pragma once

#include <wand/platforms/d3d12.hpp>


namespace wand {
struct GraphicsDeviceCapabilities {
  D3D_SHADER_MODEL highest_shader_model;
  D3D12_MESH_SHADER_TIER mesh_shader_tier;
  D3D12_RAYTRACING_TIER raytracing_tier;
  bool gpu_upload_heap;
  bool vp_and_rt_array_index_from_any_shader;

  [[nodiscard]] constexpr auto SupportsMeshShaders() const -> bool;
  [[nodiscard]] constexpr auto SupportsRaytracing() const -> bool;
  [[nodiscard]] constexpr auto SupportsInlineRaytracing() const -> bool;
};
}

#include <wand/graphics_device_capabilities.inl>
