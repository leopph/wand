#include "wand/format.hpp"

namespace wand {
auto MakeDepthTypeless(DXGI_FORMAT const depth_format) -> DXGI_FORMAT {
  if (depth_format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
    return DXGI_FORMAT_R32G8X24_TYPELESS;
  }

  if (depth_format == DXGI_FORMAT_D32_FLOAT) {
    return DXGI_FORMAT_R32_TYPELESS;
  }

  if (depth_format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
    return DXGI_FORMAT_R24G8_TYPELESS;
  }

  if (depth_format == DXGI_FORMAT_D16_UNORM) {
    return DXGI_FORMAT_R16_TYPELESS;
  }

  return depth_format;
}


auto MakeDepthUnderlyingLinear(DXGI_FORMAT const depth_format) -> DXGI_FORMAT {
  if (depth_format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
    return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
  }

  if (depth_format == DXGI_FORMAT_D32_FLOAT) {
    return DXGI_FORMAT_R32_FLOAT;
  }

  if (depth_format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  }

  if (depth_format == DXGI_FORMAT_D16_UNORM) {
    return DXGI_FORMAT_R16_UNORM;
  }

  return depth_format;
}
}
