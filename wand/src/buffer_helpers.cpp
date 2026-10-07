#include "wand/buffer_helpers.hpp"


namespace wand {
auto CreateBufferWithView(
  GraphicsDevice& device,
  BufferDesc const& buffer_desc,
  BufferViewDesc const& view_desc,
  CpuAccess const cpu_access
) -> SharedDeviceHandle<BufferView> {
  return device.CreateBufferView(view_desc, device.CreateBuffer(buffer_desc, cpu_access));
}
}
