#include "wand/root_signature_cache.hpp"

using Microsoft::WRL::ComPtr;

namespace wand::details {
auto RootSignatureCache::Add(std::uint8_t const num_params,
                             ComPtr<ID3D12RootSignature> root_signature) -> ComPtr<ID3D12RootSignature> {
  std::scoped_lock const lock{mutex_};
  return root_signatures_.try_emplace(num_params, std::move(root_signature)).first->second;
}


auto RootSignatureCache::Get(std::uint8_t const num_params) -> ComPtr<ID3D12RootSignature> {
  std::scoped_lock const lock{mutex_};

  if (auto const it{root_signatures_.find(num_params)}; it != std::end(root_signatures_)) {
    return it->second;
  }

  return nullptr;
}
}
