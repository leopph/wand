#include "wand/pipeline.hpp"

using Microsoft::WRL::ComPtr;

namespace wand {
PipelineState::PipelineState(ComPtr<ID3D12RootSignature> root_signature, ComPtr<ID3D12PipelineState> pipeline_state,
                             std::uint8_t const num_params, bool const is_compute, bool const allows_ds_write) :
  root_signature_{std::move(root_signature)},
  pipeline_state_{std::move(pipeline_state)},
  num_params_{num_params},
  is_compute_{is_compute},
  allows_ds_write_{allows_ds_write} {
}
}
