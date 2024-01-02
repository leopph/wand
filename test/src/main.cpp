#include <wand.hpp>

#include <cassert>

auto main() -> int {
  auto const gd{wand::GraphicsDevice::New(wand::GraphicsApi::kD3D12)};
  assert(gd);
}
