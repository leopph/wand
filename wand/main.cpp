#include "wand.hpp"

extern "C" {
__declspec(dllexport) extern UINT const D3D12SDKVersion = D3D12_SDK_VERSION;
__declspec(dllexport) extern char const* D3D12SDKPath = R"(.\D3D12\)";
}

auto main() -> int {
  wand::GraphicsDevice gd;
}
