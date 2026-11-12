#pragma once

#include <wand/acceleration_structure.hpp>
#include <wand/buffer.hpp>
#include <wand/buffer_view.hpp>
#include <wand/graphics_device.hpp>
#include <wand/wandapi.hpp>


namespace wand {
[[nodiscard]] WANDAPI
auto CreateBufferWithView(
  GraphicsDevice& device,
  BufferDesc const& buffer_desc,
  BufferViewDesc const& view_desc,
  CpuAccess cpu_access
) -> SharedDeviceHandle<BufferView>;

[[nodiscard]] WANDAPI
auto CreateBufferWithAs(
  GraphicsDevice& device,
  BufferDesc const& buffer_desc,
  AccelerationStructureDesc const& as_desc
) -> SharedDeviceHandle<AccelerationStructure>;
}
