#pragma once

#include "core.hpp"

#include <memory>

namespace wand {
  enum class GraphicsApi {
#ifdef _WIN64
    kD3D12 = 0
#endif
  };

  class GraphicsDevice {
  protected:
    GraphicsDevice() = default;

  public:
    GraphicsDevice(GraphicsDevice const& other) = delete;
    GraphicsDevice(GraphicsDevice&& other) = delete;

    virtual ~GraphicsDevice() = default;

    auto operator=(GraphicsDevice const& other) -> void = delete;
    auto operator=(GraphicsDevice&& other) -> void = delete;

    [[nodiscard]] WANDAPI static auto New(GraphicsApi api) -> std::unique_ptr<GraphicsDevice>;
  };

  enum ResourceViewFlags {
    RTV_BIT = 1 << 0,
    DSV_BIT = 1 << 1,
    SRV_BIT = 1 << 2,
    UAV_BIT = 1 << 3,
    CBV_BIT = 1 << 4,
  };
}
