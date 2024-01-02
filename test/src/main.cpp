#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wand.hpp>

#include <cassert>
#include <memory>
#include <type_traits>

namespace {
auto CALLBACK WindowProc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam) -> LRESULT {
  if (msg == WM_CLOSE) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}
}

auto main() -> int {
  WNDCLASSW const wnd_class{
    .style = 0,
    .lpfnWndProc = &WindowProc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = GetModuleHandleW(nullptr),
    .hIcon = nullptr,
    .hCursor = LoadCursorW(nullptr, IDC_ARROW),
    .hbrBackground = nullptr,
    .lpszMenuName = nullptr,
    .lpszClassName = L"Wand Test"
  };

  if (!RegisterClassW(&wnd_class)) {
    return -1;
  }

  using WindowDeleter = decltype([](HWND const hwnd) {
    if (hwnd) {
      DestroyWindow(hwnd);
    }
  });

  std::unique_ptr<std::remove_pointer_t<HWND>, WindowDeleter> const hwnd{CreateWindowExW(0, wnd_class.lpszClassName, L"Want Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, wnd_class.hInstance, nullptr)};

  if (!hwnd) {
    return -1;
  }

  ShowWindow(hwnd.get(), SW_SHOW);

  auto const gd{wand::GraphicsDevice::New(hwnd.get(), wand::GraphicsApi::kD3D12)};

  auto const buffer{gd->CreateBuffer(wand::Buffer::Desc{128})};

  if (!gd) {
    return -1;
  }

  while (true) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        return static_cast<int>(msg.wParam);
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
}
