#include "wand/buffer_view.hpp"

#include <utility>

namespace wand {
auto BufferView::GetDesc() const -> BufferViewDesc const& {
  return desc_;
}


auto BufferView::GetBuffer() const -> SharedDeviceChildHandle<Buffer> const& {
  return buffer_;
}


auto BufferView::GetConstantBuffer() const -> UINT {
  return cbv_;
}


auto BufferView::GetShaderResource() const -> UINT {
  return srv_;
}


auto BufferView::GetUnorderedAccess() const -> UINT {
  return uav_;
}


auto BufferView::GetAccelerationStructure() const -> UINT {
  return srv_;
}


BufferView::BufferView(SharedDeviceChildHandle<Buffer> buffer, UINT const cbv, UINT const srv, UINT const uav,
                       BufferViewDesc const& desc) :
  buffer_{std::move(buffer)}, desc_{desc}, cbv_{cbv}, srv_{srv}, uav_{uav} {
}
}
