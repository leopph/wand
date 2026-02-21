#pragma once

#include <cstdint>
#include <string_view>

#include <wand/platforms/d3d12.hpp>

namespace wand {
class RtStateObjectDesc {
public:
  [[nodiscard]] auto AddDxilLibrary() -> CD3DX12_DXIL_LIBRARY_SUBOBJECT&;
  [[nodiscard]] auto AddHitGroup() -> CD3DX12_HIT_GROUP_SUBOBJECT&;
  [[nodiscard]] auto AddShaderConfig() -> CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT&;
  [[nodiscard]] auto AddPipelineConfig() -> CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT&;

private:
  CD3DX12_STATE_OBJECT_DESC desc_{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};

  friend class GraphicsDevice;
};


class RtStateObject {
public:
  [[nodiscard]]
  auto GetShaderIdentifier(std::wstring_view export_name) const -> void*;

private:
  RtStateObject(Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature,
                Microsoft::WRL::ComPtr<ID3D12StateObject> state_object,
                Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props,
                std::uint8_t num_params);

  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature_;
  Microsoft::WRL::ComPtr<ID3D12StateObject> state_object_;
  Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props_;
  std::uint8_t num_params_;

  friend class GraphicsDevice;
  friend class CommandList;
};
}
