#pragma once

#include "core.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace wand {
constexpr auto kMaxRenderTargets{8};

enum class LogicOp : std::uint8_t {
  kClear,
  kSet,
  kCopy,
  kCopyInverted,
  kNoOp,
  kInvert,
  kAnd,
  kNand,
  kOr,
  kNor,
  kXor,
  kEquivalent,
  kAndReverse,
  kAndInverted,
  kOrReverse,
  kOrInverted
};

enum class BlendFactor : std::uint8_t {
  kZero = 1,
  kOne = 2,
  kSrcColor = 3,
  kInvSrcColor = 4,
  kSrcAlpha = 5,
  kInvSrcAlpha = 6,
  kDstAlpha = 7,
  kInvDstAlpha = 8,
  kDstColor = 9,
  kInvDstColor = 10,
  kSrcAlphaSaturate = 11,
  kConstant = 14,
  kInvConstant = 15,
  kSrc1Color = 16,
  kInvSrc1Color = 17,
  kSrc1Alpha = 18,
  kInvSrc1Alpha = 19,
  kConstantAlpha = 20,
  kInvConstantAlpha = 21
};

enum class BlendOp : std::uint8_t {
  kAdd = 1,
  kSub = 2,
  kRevSub = 3,
  kMin = 4,
  kMax = 5
};

enum class ColorWriteEnable : std::uint8_t {
  kRed = 0b1,
  kGreen = 0b10,
  kBlue = 0b100,
  kAlpha = 0b1000,
  kAll = 0b1111
};

DEFINE_ENUM_FLAG_OPS(ColorWriteEnable)

struct RenderTargetBlendInfo {
  bool blend_enable;
  BlendFactor src_color_blend_factor;
  BlendFactor dst_color_blend_factor;
  BlendOp color_blend_op;
  BlendFactor src_alpha_blend_factor;
  BlendFactor dst_alpha_blend_factor;
  BlendOp alpha_blend_op;
  ColorWriteEnable color_write_mask;
};

struct BlendStateInfo {
  std::optional<LogicOp> logic_op;
  std::array<RenderTargetBlendInfo, kMaxRenderTargets> render_targets;
  std::array<float, 4> blend_constant;
};

enum class PolygonFillMode : std::uint8_t {
  kWireFrame = 2,
  kSolid = 3
};

enum class FaceCullingMode : std::uint8_t {
  kNone = 1,
  kFront = 2,
  kBack = 3
};

enum class FrontFace : std::uint8_t {
  kClockwise,
  kCounterClockwise
};

struct RasterizerStateInfo {
  PolygonFillMode fill_mode;
  FaceCullingMode cull_mode;
  FrontFace front_face;
  float depth_bias;
  float depth_bias_clamp;
  float depth_bias_slope;
};

enum class CompareOp : std::uint8_t {
  kNever = 1,
  kLess = 2,
  kEqual = 3,
  kLessOrEqual = 4,
  kGreater = 5,
  kNotEqual = 6,
  kGreaterOrEqual = 7,
  kAlways = 8
};

enum class StencilOp : std::uint8_t {
  kKeep = 1,
  kZero = 2,
  kReplace = 3,
  kIncClamp = 4,
  kDecClamp = 5,
  kInvert = 6,
  kIncWrap = 7,
  kDecWrap = 8
};

struct StencilOpInfo {
  StencilOp fail_op;
  StencilOp depth_fail_op;
  StencilOp pass_op;
  CompareOp compare_op;
};

struct DepthStencilStateInfo {
  bool depth_test_enable;
  bool depth_write_enable;
  CompareOp depth_compare_op;
  bool stencil_test_enable;
  StencilOpInfo stencil_front_face;
  StencilOpInfo stencil_back_face;
  std::uint8_t stencil_read_mask;
  std::uint8_t stencil_write_mask;
  std::uint8_t stencil_reference;
};

struct MultisampleStateInfo {
  bool alpha_to_coverage_enable;
  std::uint8_t sample_count;
};

struct RenderTargetFormatInfo {
  std::array<Format, kMaxRenderTargets> render_targets;
  Format depth_stencil;
};

enum class PrimitiveTopology : std::uint8_t {
  kPointList = 1,
  kLineList = 2,
  kLineStrip = 3,
  kTriangleList = 4,
  kTriangleStrip = 5,
  kTriangleFan = 6,
  kLineListAdj = 10,
  kLineStripAdj = 11,
  kTriangleListAdj = 12,
  kTriangleStripAdj = 13,
  kPatchList = 33
};

struct InputAssemblyInfo {
  PrimitiveTopology primitive_topology;
  bool primitive_restart_enable;
};

struct GraphicsPipelineStateInfo {
  std::span<std::uint8_t const> vertex_shader;
  std::span<std::uint8_t const> hull_shader;
  std::span<std::uint8_t const> domain_shader;
  std::span<std::uint8_t> geometry_shader;
  std::span<std::uint8_t const> pixel_shader;
  BlendStateInfo blend_state;
  RasterizerStateInfo rasterizer_state;
  DepthStencilStateInfo depth_stencil_state;
  MultisampleStateInfo multisample_state;
  RenderTargetFormatInfo formats;
  InputAssemblyInfo input_assembly;
  std::uint8_t parameter_count;
  std::uint8_t render_target_count;
};

class PipelineState {
protected:
  PipelineState() = default;
};
}
