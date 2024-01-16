#pragma once

#include "core.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "pipeline_state.hpp"

#include <memory>

namespace wand {
enum class GraphicsApi {
#ifdef _WIN64
  kD3D12 = 0,
#endif
};

class Device {
protected:
  Device() = default;

public:
  Device(Device const& other) = delete;
  Device(Device&& other) = delete;

  virtual ~Device() = default;

  auto operator=(Device const& other) -> void = delete;
  auto operator=(Device&& other) -> void = delete;

  [[nodiscard]] WANDAPI static auto New(void* window_handle, GraphicsApi api) -> std::unique_ptr<Device>;

  [[nodiscard]] virtual WANDAPI auto CreateBuffer(Buffer::Desc const& desc) -> std::unique_ptr<Buffer> = 0;
  [[nodiscard]] virtual WANDAPI auto CreateTexture(Texture::Desc const& desc) -> std::unique_ptr<Texture> = 0;
  [[nodiscard]] virtual WANDAPI auto CreatePipelineState(GraphicsPipelineStateInfo const& info) -> std::unique_ptr<PipelineState> = 0;
};

enum ResourceViewFlags {
  RTV_BIT = 1 << 0,
  DSV_BIT = 1 << 1,
  SRV_BIT = 1 << 2,
  UAV_BIT = 1 << 3,
  CBV_BIT = 1 << 4,
};
}
