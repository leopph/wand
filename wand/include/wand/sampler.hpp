#pragma once

#include <wand/common.hpp>
#include <wand/wandapi.hpp>
#include <wand/platforms/d3d12.hpp>


namespace wand {
using Sampler = UINT;

class GraphicsDevice;


class UniqueSamplerHandle {
public:
  UniqueSamplerHandle() = default;
  WANDAPI UniqueSamplerHandle(UINT resource, GraphicsDevice& device);
  UniqueSamplerHandle(UniqueSamplerHandle const& other) = delete;
  WANDAPI UniqueSamplerHandle(UniqueSamplerHandle&& other) noexcept;

  WANDAPI ~UniqueSamplerHandle();

  auto operator=(UniqueSamplerHandle const& other) -> void = delete;
  WANDAPI auto operator=(UniqueSamplerHandle&& other) noexcept -> UniqueSamplerHandle&;

  [[nodiscard]] WANDAPI
  auto Get() const -> UINT;

  [[nodiscard]] WANDAPI
  auto IsValid() const -> bool;

private:
  auto InternalDestruct() const -> void;

  UINT resource_{kInvalidResourceIndex};
  GraphicsDevice* device_{nullptr};
};
}
