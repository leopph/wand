#include "wand/acceleration_structure.hpp"

#include <utility>

#include "wand/buffer_range.hpp"
#include "wand/detail/buffer_utils.hpp"


namespace wand {
auto AccelerationStructure::GetDesc() const -> AccelerationStructureDesc const& {
  return desc_;
}


auto AccelerationStructure::GetBuffer() const -> SharedDeviceHandle<Buffer> const& {
  return buf_;
}


auto AccelerationStructure::GetGpuAddress() const -> D3D12_GPU_VIRTUAL_ADDRESS {
  return detail::GetGpuAddress(BufferRange{*buf_, desc_.region});
}


auto AccelerationStructure::GetRegion() const -> BufferRegion const& {
  return desc_.region;
}


auto AccelerationStructure::GetShaderResource() const -> UINT {
  return srv_.value();
}


AccelerationStructure::AccelerationStructure(SharedDeviceHandle<Buffer> buf, std::optional<UINT> const srv,
                                             AccelerationStructureDesc const& desc) :
  buf_{std::move(buf)},
  desc_{desc},
  srv_{srv} {}
}
