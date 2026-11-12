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


auto CreateBufferWithAs(
  GraphicsDevice& device,
  BufferDesc const& buffer_desc,
  AccelerationStructureDesc const& as_desc
) -> SharedDeviceHandle<AccelerationStructure> {
  // Cpu access none because an AS must live in a default heap
  return device.CreateAccelerationStructure(as_desc, device.CreateBuffer(buffer_desc, CpuAccess::kNone));
}
}
