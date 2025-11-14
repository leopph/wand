#pragma once

#include <wand/platforms/d3d12.hpp>

namespace wand {
struct PipelineDesc {
  D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type{D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE};
  CD3DX12_SHADER_BYTECODE vs;
  CD3DX12_SHADER_BYTECODE gs;
  D3D12_STREAM_OUTPUT_DESC stream_output;
  CD3DX12_SHADER_BYTECODE hs;
  CD3DX12_SHADER_BYTECODE ds;
  CD3DX12_SHADER_BYTECODE ps;
  CD3DX12_SHADER_BYTECODE as;
  CD3DX12_SHADER_BYTECODE ms;
  CD3DX12_SHADER_BYTECODE cs;
  CD3DX12_BLEND_DESC blend_state{D3D12_DEFAULT};
  CD3DX12_DEPTH_STENCIL_DESC1 depth_stencil_state{D3D12_DEFAULT};
  DXGI_FORMAT ds_format;
  CD3DX12_RASTERIZER_DESC rasterizer_state{D3D12_DEFAULT};
  CD3DX12_RT_FORMAT_ARRAY rt_formats;
  DXGI_SAMPLE_DESC sample_desc{1, 0};
  UINT sample_mask{D3D12_DEFAULT_SAMPLE_MASK};
  CD3DX12_VIEW_INSTANCING_DESC view_instancing_desc{D3D12_DEFAULT};
};


class PipelineState {
  PipelineState(Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature,
                Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state, std::uint8_t num_params, bool is_compute,
                bool allows_ds_write);

  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state_;
  std::uint8_t num_params_;
  bool is_compute_;
  bool allows_ds_write_;

  friend class GraphicsDevice;
  friend class CommandList;
};
}
