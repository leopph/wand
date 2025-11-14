#pragma once

#include <wand/common.hpp>
#include <wand/platforms/d3d12.hpp>

namespace wand {
using Sampler = UINT;

class GraphicsDevice;

class UniqueSamplerHandle {
public:
  UniqueSamplerHandle() = default;
  UniqueSamplerHandle(UINT resource, GraphicsDevice& device);
  UniqueSamplerHandle(UniqueSamplerHandle const& other) = delete;
  UniqueSamplerHandle(UniqueSamplerHandle&& other) noexcept;

  ~UniqueSamplerHandle();

  auto operator=(UniqueSamplerHandle const& other) -> void = delete;
  auto operator=(UniqueSamplerHandle&& other) noexcept -> UniqueSamplerHandle&;

  [[nodiscard]] auto Get() const -> UINT;
  [[nodiscard]] auto IsValid() const -> bool;

private:
  auto InternalDestruct() const -> void;

  UINT resource_{kInvalidResourceIndex};
  GraphicsDevice* device_{nullptr};
};
}
