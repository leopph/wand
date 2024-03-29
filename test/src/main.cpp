#include "ps.h"
#include "vs.h"

#include <wand.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <memory>
#include <type_traits>

namespace {
auto CALLBACK WindowProc(HWND const hwnd, UINT const msg, WPARAM const wparam,
                         LPARAM const lparam) -> LRESULT {
  if (msg == WM_CLOSE) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}
}

auto main() -> int {
  WNDCLASSW const wnd_class{
    .style = 0, .lpfnWndProc = &WindowProc, .cbClsExtra = 0, .cbWndExtra = 0,
    .hInstance = GetModuleHandleW(nullptr), .hIcon = nullptr,
    .hCursor = LoadCursorW(nullptr, IDC_ARROW), .hbrBackground = nullptr,
    .lpszMenuName = nullptr, .lpszClassName = L"Wand Test"
  };

  if (!RegisterClassW(&wnd_class)) {
    return -1;
  }

  using WindowDeleter = decltype([](HWND const hwnd) {
    if (hwnd) {
      DestroyWindow(hwnd);
    }
  });

  std::unique_ptr<std::remove_pointer_t<HWND>, WindowDeleter> const hwnd{
    CreateWindowExW(0, wnd_class.lpszClassName, L"Want Test",
                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
                    wnd_class.hInstance, nullptr)
  };

  if (!hwnd) {
    return -1;
  }

  ShowWindow(hwnd.get(), SW_SHOW);

  auto const gd{wand::Device::New(hwnd.get())};

  if (!gd) {
    return -1;
  }

  auto const buffer{
    gd->CreateBuffer(wand::Buffer::Desc{
      .width = 256, .stride = 16, .usage = wand::Buffer::kUsageConstantBuffer,
      .mappable = true
    })
  };

  if (!buffer->Map()) {
    return -1;
  }

  buffer->Unmap();

  auto const tex{
    gd->CreateTexture(wand::Texture::Desc{
      .width = 1024, .height = 1024, .mip_count = 1, .depth_or_array_size = 1,
      .sample_count = 1, .sample_quality = 0, .render_target_clear_value = {},
      .depth_clear_value = {}, .stencil_clear_value = {},
      .format = wand::Format::kB8G8R8A8UnormSrgb,
      .dimension = wand::Texture::Dimension::k2D,
      .usage = wand::Texture::kUsageShaderResource
    })
  };

  auto const rt{
    gd->CreateTexture(wand::Texture::Desc{
      .width = 2560, .height = 1440, .mip_count = 1, .depth_or_array_size = 1,
      .sample_count = 4, .sample_quality = 0,
      .render_target_clear_value = {0.0f, 0.0f, 0.0f, 1.0f},
      .depth_clear_value = {}, .stencil_clear_value = {},
      .format = wand::Format::kR16G16B16A16Float,
      .dimension = wand::Texture::Dimension::k2D,
      .usage = wand::Texture::Usage::kUsageRenderTarget
    })
  };

  auto const pso{
    gd->CreatePipelineState(wand::GraphicsPipelineStateInfo{
      .vertex_shader = {kVsBin, ARRAYSIZE(kVsBin)}, .hull_shader = {},
      .domain_shader = {}, .geometry_shader = {},
      .pixel_shader = {kPsBin, ARRAYSIZE(kPsBin)},
      .blend_state = {
        .logic_op = std::nullopt,
        .render_targets = {
          wand::RenderTargetBlendInfo{
            false, {}, {}, {}, {}, {}, {}, wand::ColorWriteEnable::kAll
          }
        },
        .blend_constant = {}
      },
      .rasterizer_state = {
        .fill_mode = wand::PolygonFillMode::kSolid,
        .cull_mode = wand::FaceCullingMode::kNone,
        .front_face = wand::FrontFace::kClockwise, .depth_bias = 0,
        .depth_bias_clamp = 0, .depth_bias_slope = 0
      },
      .depth_stencil_state = {
        .depth_test_enable = false, .depth_write_enable = false,
        .depth_compare_op = {}, .stencil_test_enable = {},
        .stencil_front_face = {}, .stencil_back_face = {},
        .stencil_read_mask = {}, .stencil_write_mask = {},
        .stencil_reference = {}
      },
      .multisample_state = {
        .alpha_to_coverage_enable = false, .sample_count = 1
      },
      .formats = {
        .render_targets = {wand::Format::kR8G8B8A8Unorm}, .depth_stencil = {}
      },
      .input_assembly = {
        .primitive_topology = wand::PrimitiveTopology::kTriangleList,
        .primitive_restart_enable = false
      },
      .parameter_count = 0, .render_target_count = 1
    })
  };

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
