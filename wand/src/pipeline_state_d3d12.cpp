#include "pipeline_state_d3d12.hpp"

#include <utility>

namespace wand {
PipelineStateD3D12::PipelineStateD3D12(
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature,
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso) : root_signature_{
  std::move(root_signature)
}, pso_{std::move(pso)} {
}
}
