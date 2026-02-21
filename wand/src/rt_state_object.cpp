#include "wand/rt_state_object.hpp"

namespace wand {
auto RtStateObjectDesc::AddDxilLibrary() -> CD3DX12_DXIL_LIBRARY_SUBOBJECT& {
  return *desc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
}


auto RtStateObjectDesc::AddHitGroup() -> CD3DX12_HIT_GROUP_SUBOBJECT& {
  return *desc_.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
}


auto RtStateObjectDesc::AddShaderConfig() -> CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT& {
  return *desc_.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
}


auto RtStateObjectDesc::AddPipelineConfig() -> CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT& {
  return *desc_.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
}


auto RtStateObject::GetShaderIdentifier(std::wstring_view const export_name) const -> void* {
  return props_->GetShaderIdentifier(export_name.data());
}


RtStateObject::RtStateObject(Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature,
                             Microsoft::WRL::ComPtr<ID3D12StateObject> state_object,
                             Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> props, std::uint8_t const num_params) :
  root_signature_{std::move(root_signature)}, state_object_{std::move(state_object)}, props_{std::move(props)},
  num_params_{num_params} {
}
}
