#pragma once

#include "pipeline_state.hpp"
#include "platform_d3d12.hpp"

namespace wand {
class PipelineStateD3D12 : public PipelineState {
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

public:
  PipelineStateD3D12(Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature,
                     Microsoft::WRL::ComPtr<ID3D12PipelineState> pso);
};
}
