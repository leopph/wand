#include "wand/buffer_view.hpp"

#include <utility>


namespace wand {
auto BufferView::GetDesc() const -> BufferViewDesc const& {
  return desc_;
}


auto BufferView::GetBuffer() const -> SharedDeviceHandle<Buffer> const& {
  return buf_;
}


auto BufferView::GetConstantBuffer() const -> UINT {
  return cbv_.value();
}


auto BufferView::GetShaderResource() const -> UINT {
  return srv_.value();
}


auto BufferView::GetUnorderedAccess() const -> UINT {
  return uav_.value();
}


BufferView::BufferView(SharedDeviceHandle<Buffer> buf, std::optional<UINT> const cbv,
                       std::optional<UINT> const srv, std::optional<UINT> const uav, BufferViewDesc const& desc) :
  buf_{std::move(buf)},
  cbv_{cbv},
  srv_{srv},
  uav_{uav},
  desc_{desc} {}
}
