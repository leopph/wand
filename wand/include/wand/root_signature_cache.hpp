#pragma once

#include <mutex>
#include <unordered_map>

#include <wand/platforms/d3d12.hpp>

namespace wand::details {
class RootSignatureCache {
public:
  auto Add(std::uint8_t num_params,
           Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature) -> Microsoft::WRL::ComPtr<ID3D12RootSignature>;
  [[nodiscard]] auto Get(std::uint8_t num_params) -> Microsoft::WRL::ComPtr<ID3D12RootSignature>;

private:
  std::unordered_map<std::uint8_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> root_signatures_;
  std::mutex mutex_;
};
}
