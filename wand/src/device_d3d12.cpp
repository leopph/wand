#ifdef _WIN64

#include "device_d3d12.hpp"
#include "buffer_d3d12.hpp"
#include "texture_d3d12.hpp"
#include "pipeline_state_d3d12.hpp"

#include <cassert>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <utility>

namespace {
auto ThrowIfFailed(HRESULT const hr) -> void {
  if (FAILED(hr)) {
    throw std::exception{};
  }
}

[[nodiscard]] auto
FormatToDxgiFormat(wand::Format const format) -> DXGI_FORMAT {
  return static_cast<DXGI_FORMAT>(format);
}

[[nodiscard]] auto LogicOpToD3DLogicOp(
  wand::LogicOp const logic_op) -> D3D12_LOGIC_OP {
  return static_cast<D3D12_LOGIC_OP>(logic_op);
}

[[nodiscard]] auto BlendFactorToD3DBlend(
  wand::BlendFactor const blend_factor) -> D3D12_BLEND {
  return static_cast<D3D12_BLEND>(blend_factor);
}

[[nodiscard]] auto BlendOpToD3DBlendOp(
  wand::BlendOp const blend_op) -> D3D12_BLEND_OP {
  return static_cast<D3D12_BLEND_OP>(blend_op);
}

[[nodiscard]] auto ColorWriteToD3DWriteMask(
  wand::ColorWriteEnable const color_write) -> UINT8 {
  return static_cast<std::uint8_t>(color_write);
}

[[nodiscard]] auto PolygonFillModeToD3DFillMode(
  wand::PolygonFillMode const fill_mode) -> D3D12_FILL_MODE {
  return static_cast<D3D12_FILL_MODE>(fill_mode);
}

[[nodiscard]] auto FaceCullingModeToD3DCullMode(
  wand::FaceCullingMode const mode) -> D3D12_CULL_MODE {
  return static_cast<D3D12_CULL_MODE>(mode);
}

[[nodiscard]] auto CompareOpToD3DComparisonFunc(
  wand::CompareOp const compare_op) -> D3D12_COMPARISON_FUNC {
  return static_cast<D3D12_COMPARISON_FUNC>(compare_op);
}

[[nodiscard]] auto StencilOpToD3D12StencilOp(
  wand::StencilOp const stencil_op) -> D3D12_STENCIL_OP {
  return static_cast<D3D12_STENCIL_OP>(stencil_op);
}

[[nodiscard]] auto PrimitiveTopologyToD3DPrimitiveTopologyType(
  wand::PrimitiveTopology const topology) -> D3D12_PRIMITIVE_TOPOLOGY_TYPE {
  switch (topology) {
    using enum wand::PrimitiveTopology;
  case kPointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  case kLineList: [[fallthrough]];
  case kLineStrip: [[fallthrough]];
  case kLineListAdj: [[fallthrough]];
  case kLineStripAdj: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case kTriangleFan: [[fallthrough]];
  case kTriangleList: [[fallthrough]];
  case kTriangleListAdj: [[fallthrough]];
  case kTriangleStrip: [[fallthrough]];
  case kTriangleStripAdj: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  case kPatchList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
  }
}
}

namespace wand {
auto DeviceD3D12::SignalAndWaitFence(ID3D12Fence* fence,
                                     UINT64 const signal_value,
                                     UINT64 const wait_value) const noexcept ->
  void {
  ThrowIfFailed(direct_queue_->Signal(fence, signal_value));
  if (fence->GetCompletedValue() < wait_value) {
    ThrowIfFailed(fence->SetEventOnCompletion(wait_value, nullptr));
  }
}

auto DeviceD3D12::WaitForAllFrames() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

auto DeviceD3D12::WaitForInFlightFrameLimit() noexcept -> void {
  auto const signal_value{++frame_fence_value_};
  auto const wait_value{signal_value - max_frames_in_flight_ + 1};
  SignalAndWaitFence(frame_fence_.Get(), signal_value, wait_value);
}

auto DeviceD3D12::CreateConstantBufferView(
  D3D12_CONSTANT_BUFFER_VIEW_DESC const& cbv_desc) -> ResViewIdxType {
  resource_desc_heap_index_mutex_.lock();
  auto const cbv_idx{resource_descriptor_heap_free_indices_.back()};
  resource_descriptor_heap_free_indices_.pop_back();
  resource_desc_heap_index_mutex_.unlock();
  device_->CreateConstantBufferView(&cbv_desc, CD3DX12_CPU_DESCRIPTOR_HANDLE{
                                      resource_descriptor_heap_->
                                      GetCPUDescriptorHandleForHeapStart(),
                                      static_cast<INT>(cbv_idx),
                                      device_->GetDescriptorHandleIncrementSize(
                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                                    });
  return cbv_idx;
}

auto DeviceD3D12::ReleaseConstantBufferView(ResViewIdxType const idx) -> void {
  std::unique_lock const lock{resource_desc_heap_index_mutex_};
  resource_descriptor_heap_free_indices_.push_back(idx);
}

auto DeviceD3D12::CreateShaderResourceView(ID3D12Resource* const resource,
                                           D3D12_SHADER_RESOURCE_VIEW_DESC const
                                           & srv_desc) -> ResViewIdxType {
  resource_desc_heap_index_mutex_.lock();
  auto const srv_idx{resource_descriptor_heap_free_indices_.back()};
  resource_descriptor_heap_free_indices_.pop_back();
  resource_desc_heap_index_mutex_.unlock();
  device_->CreateShaderResourceView(resource, &srv_desc,
                                    CD3DX12_CPU_DESCRIPTOR_HANDLE{
                                      resource_descriptor_heap_->
                                      GetCPUDescriptorHandleForHeapStart(),
                                      static_cast<INT>(srv_idx),
                                      device_->GetDescriptorHandleIncrementSize(
                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                                    });
  return srv_idx;
}

auto DeviceD3D12::ReleaseShaderResourceView(ResViewIdxType const idx) -> void {
  std::unique_lock const lock{resource_desc_heap_index_mutex_};
  resource_descriptor_heap_free_indices_.push_back(idx);
}

auto DeviceD3D12::CreateUnorderedAccessView(ID3D12Resource* const resource,
                                            D3D12_UNORDERED_ACCESS_VIEW_DESC
                                            const& uav_desc) -> ResViewIdxType {
  resource_desc_heap_index_mutex_.lock();
  auto const uav_idx{resource_descriptor_heap_free_indices_.back()};
  resource_descriptor_heap_free_indices_.pop_back();
  resource_desc_heap_index_mutex_.unlock();
  device_->CreateUnorderedAccessView(resource, nullptr, &uav_desc,
                                     CD3DX12_CPU_DESCRIPTOR_HANDLE{
                                       resource_descriptor_heap_->
                                       GetCPUDescriptorHandleForHeapStart(),
                                       static_cast<INT>(uav_idx),
                                       device_->
                                       GetDescriptorHandleIncrementSize(
                                         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                                     });
  return uav_idx;
}

auto DeviceD3D12::ReleaseUnorderedAccessView(ResViewIdxType const idx) -> void {
  std::unique_lock const lock{resource_desc_heap_index_mutex_};
  resource_descriptor_heap_free_indices_.push_back(idx);
}

auto DeviceD3D12::CreateRenderTargetView(ID3D12Resource* const resource,
                                         D3D12_RENDER_TARGET_VIEW_DESC const&
                                         rtv_desc) -> RtvIdxType {
  rtv_heap_index_mutex_.lock();
  auto const rtv_idx{rtv_heap_free_indices_.back()};
  rtv_heap_free_indices_.pop_back();
  rtv_heap_index_mutex_.unlock();
  device_->CreateRenderTargetView(resource, &rtv_desc,
                                  CD3DX12_CPU_DESCRIPTOR_HANDLE{
                                    rtv_heap_->
                                    GetCPUDescriptorHandleForHeapStart(),
                                    static_cast<INT>(rtv_idx),
                                    device_->GetDescriptorHandleIncrementSize(
                                      D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
                                  });
  return rtv_idx;
}

auto DeviceD3D12::ReleaseRenderTargetView(RtvIdxType const idx) -> void {
  std::unique_lock const lock{rtv_heap_index_mutex_};
  rtv_heap_free_indices_.push_back(idx);
}

auto DeviceD3D12::CreateDepthStencilView(ID3D12Resource* resource,
                                         D3D12_DEPTH_STENCIL_VIEW_DESC const&
                                         dsv_desc) -> DsvIdxType {
  dsv_heap_index_mutex_.lock();
  auto const dsv_idx{dsv_heap_free_indices_.back()};
  dsv_heap_free_indices_.pop_back();
  dsv_heap_index_mutex_.unlock();
  device_->CreateDepthStencilView(resource, &dsv_desc,
                                  CD3DX12_CPU_DESCRIPTOR_HANDLE{
                                    dsv_heap_->
                                    GetCPUDescriptorHandleForHeapStart(),
                                    static_cast<INT>(dsv_idx),
                                    device_->GetDescriptorHandleIncrementSize(
                                      D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
                                  });
  return dsv_idx;
}

auto DeviceD3D12::ReleaseDepthStencilView(DsvIdxType const idx) -> void {
  std::unique_lock const lock{dsv_heap_index_mutex_};
  dsv_heap_free_indices_.push_back(idx);
}

DeviceD3D12::DeviceD3D12(HWND const hwnd) {
  using Microsoft::WRL::ComPtr;

#ifndef NDEBUG
  ComPtr<ID3D12Debug5> debug;
  ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
  debug->EnableDebugLayer();
#endif

  UINT factory_create_flags{0};
#ifndef NDEBUG
  factory_create_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  ThrowIfFailed(
    CreateDXGIFactory2(factory_create_flags, IID_PPV_ARGS(&factory_)));
  ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                  IID_PPV_ARGS(&device_)));

#ifndef NDEBUG
  ComPtr<ID3D12InfoQueue> info_queue;
  ThrowIfFailed(device_.As(&info_queue));
  ThrowIfFailed(
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
  ThrowIfFailed(
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
#endif

  CD3DX12FeatureSupport feature_support;
  ThrowIfFailed(feature_support.Init(device_.Get()));

  if (feature_support.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3 ||
    feature_support.HighestShaderModel() < D3D_SHADER_MODEL_6_6) {
    throw std::runtime_error{"Unsuppported hardware."};
  }

  ComPtr<IDXGIAdapter> adapter;
  ThrowIfFailed(
    factory_->EnumAdapterByLuid(device_->GetAdapterLuid(),
                                IID_PPV_ARGS(&adapter)));

  D3D12MA::ALLOCATOR_DESC const allocator_desc{
    .Flags = D3D12MA::ALLOCATOR_FLAG_NONE, .pDevice = device_.Get(),
    .PreferredBlockSize = 0, .pAllocationCallbacks = nullptr,
    .pAdapter = adapter.Get()
  };

  ThrowIfFailed(CreateAllocator(&allocator_desc, &allocator_));

  D3D12_COMMAND_QUEUE_DESC constexpr direct_queue_desc{
    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE, .NodeMask = 0
  };

  ThrowIfFailed(
    device_->CreateCommandQueue(&direct_queue_desc,
                                IID_PPV_ARGS(&direct_queue_)));

  for (auto i{0}; i < max_frames_in_flight_; i++) {
    ThrowIfFailed(device_->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      IID_PPV_ARGS(&direct_command_allocators_[i])));
    ThrowIfFailed(device_->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              D3D12_COMMAND_LIST_FLAG_NONE,
                                              IID_PPV_ARGS(
                                                &direct_command_lists_[i])));
  }

  DXGI_SWAP_CHAIN_DESC1 constexpr swap_chain_desc{
    .Width = 0, .Height = 0, .Format = swap_chain_format_, .Stereo = FALSE,
    .SampleDesc = {.Count = 1, .Quality = 0}, .BufferUsage = 0,
    .BufferCount = swap_chain_buffer_count_, .Scaling = DXGI_SCALING_NONE,
    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED, .Flags = 0
  };

  ComPtr<IDXGISwapChain1> swap_chain;
  ThrowIfFailed(factory_->CreateSwapChainForHwnd(
    direct_queue_.Get(), hwnd, &swap_chain_desc, nullptr, nullptr,
    &swap_chain));
  ThrowIfFailed(swap_chain.As(&swap_chain_));

  ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(
                                       frame_fence_.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr resource_descriptor_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = res_desc_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0
  };

  ThrowIfFailed(device_->CreateDescriptorHeap(&resource_descriptor_heap_desc,
                                              IID_PPV_ARGS(
                                                &resource_descriptor_heap_)));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr rtv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = rtv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0
  };

  ThrowIfFailed(
    device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_)));

  D3D12_DESCRIPTOR_HEAP_DESC constexpr dsv_heap_desc{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV, .NumDescriptors = dsv_heap_size_,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0
  };

  ThrowIfFailed(
    device_->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap_)));

  resource_descriptor_heap_free_indices_.reserve(res_desc_heap_size_);
  for (auto i{0}; i < res_desc_heap_size_; i++) {
    resource_descriptor_heap_free_indices_.emplace_back(i);
  }

  rtv_heap_free_indices_.reserve(rtv_heap_size_);
  for (auto i{0}; i < rtv_heap_size_; i++) {
    rtv_heap_free_indices_.emplace_back(i);
  }

  dsv_heap_free_indices_.reserve(dsv_heap_size_);
  for (auto i{0}; i < dsv_heap_size_; i++) {
    dsv_heap_free_indices_.emplace_back(i);
  }
}

auto DeviceD3D12::CreateBuffer(
  Buffer::Desc const& desc) -> std::unique_ptr<Buffer> {
  D3D12MA::ALLOCATION_DESC const allocation_desc{
    .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
    .HeapType = desc.mappable
                  ? D3D12_HEAP_TYPE_UPLOAD
                  : D3D12_HEAP_TYPE_DEFAULT,
    .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE, .CustomPool = nullptr,
    .pPrivateData = nullptr
  };

  auto const buffer_desc{CD3DX12_RESOURCE_DESC1::Buffer(desc.width)};

  Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
  ThrowIfFailed(allocator_->CreateResource2(&allocation_desc, &buffer_desc,
                                            D3D12_RESOURCE_STATE_COMMON,
                                            nullptr, &allocation, IID_NULL,
                                            nullptr));

  std::uint32_t cbv_idx{invalid_descriptor_idx_};
  std::uint32_t srv_idx{invalid_descriptor_idx_};
  std::uint32_t uav_idx{invalid_descriptor_idx_};

  if (desc.usage & Buffer::kUsageConstantBuffer) {
    D3D12_CONSTANT_BUFFER_VIEW_DESC const cbv_desc{
      .BufferLocation = allocation->GetResource()->GetGPUVirtualAddress(),
      .SizeInBytes = desc.width
    };
    cbv_idx = CreateConstantBufferView(cbv_desc);
  }

  if (desc.usage & Buffer::kUsageShaderResource) {
    D3D12_SHADER_RESOURCE_VIEW_DESC const srv_desc{
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Buffer = {
        .FirstElement = 0, .NumElements = desc.width / desc.stride,
        .StructureByteStride = desc.stride, .Flags = D3D12_BUFFER_SRV_FLAG_NONE
      }
    };
    srv_idx = CreateShaderResourceView(allocation->GetResource(), srv_desc);
  }

  if (desc.usage & Buffer::kUsageUnorderedAccess) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC const uav_desc{
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
      .Buffer = {
        .FirstElement = 0, .NumElements = desc.width / desc.stride,
        .StructureByteStride = desc.stride, .CounterOffsetInBytes = 0,
        .Flags = D3D12_BUFFER_UAV_FLAG_NONE
      }
    };
    uav_idx = CreateUnorderedAccessView(allocation->GetResource(), uav_desc);
  }

  return std::make_unique<BufferD3D12>(desc, this, std::move(allocation),
                                       cbv_idx, srv_idx, uav_idx);
}

auto DeviceD3D12::CreateTexture(
  Texture::Desc const& desc) -> std::unique_ptr<Texture> {
  D3D12MA::ALLOCATION_DESC constexpr allocation_desc{
    .Flags = D3D12MA::ALLOCATION_FLAG_NONE, .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE, .CustomPool = nullptr,
    .pPrivateData = nullptr
  };

  auto const texture_flags{
    [&desc] {
      auto ret{D3D12_RESOURCE_FLAG_NONE};
      if (desc.usage & Texture::kUsageUnorderedAccess) {
        ret |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      }
      if (desc.usage & Texture::kUsageRenderTarget) {
        ret |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
      }
      if (desc.usage & Texture::kUsageDepthStencil) {
        ret |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (!(desc.usage & Texture::kUsageShaderResource)) {
          ret |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
      }
      return ret;
    }()
  };

  auto const texture_desc{
    [&desc, texture_flags] {
      CD3DX12_RESOURCE_DESC1 ret{};
      if (desc.dimension == Texture::Dimension::k1D) {
        ret = CD3DX12_RESOURCE_DESC1::Tex1D(FormatToDxgiFormat(desc.format),
                                            desc.width,
                                            desc.depth_or_array_size,
                                            desc.mip_count, texture_flags);
      } else if (desc.dimension == Texture::Dimension::k2D) {
        ret = CD3DX12_RESOURCE_DESC1::Tex2D(FormatToDxgiFormat(desc.format),
                                            desc.width, desc.height,
                                            desc.depth_or_array_size,
                                            desc.mip_count, desc.sample_count,
                                            desc.sample_quality, texture_flags);
      } else if (desc.dimension == Texture::Dimension::k3D || desc.dimension ==
        Texture::Dimension::kCube) {
        ret = CD3DX12_RESOURCE_DESC1::Tex3D(FormatToDxgiFormat(desc.format),
                                            desc.width, desc.height,
                                            desc.depth_or_array_size,
                                            desc.mip_count, texture_flags);
      }
      return ret;
    }()
  };

  auto const clear_value{
    [&desc] {
      D3D12_CLEAR_VALUE ret;
      ret.Format = FormatToDxgiFormat(desc.format);
      if (desc.usage & Texture::kUsageRenderTarget) {
        std::memcpy(ret.Color, desc.render_target_clear_value.data(), 16);
      } else if (desc.usage & Texture::kUsageDepthStencil) {
        ret.DepthStencil.Depth = desc.depth_clear_value;
        ret.DepthStencil.Stencil = desc.stencil_clear_value;
      }
      return ret;
    }()
  };

  auto const p_clear_value{
    (desc.usage & Texture::kUsageRenderTarget) || (desc.usage &
      Texture::kUsageDepthStencil)
      ? &clear_value
      : nullptr
  };

  Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
  ThrowIfFailed(allocator_->CreateResource2(&allocation_desc, &texture_desc,
                                            D3D12_RESOURCE_STATE_COMMON,
                                            p_clear_value, &allocation,
                                            IID_NULL, nullptr));

  std::uint32_t srv_idx{invalid_descriptor_idx_};
  std::uint32_t uav_idx{invalid_descriptor_idx_};
  std::uint32_t rtv_idx{invalid_descriptor_idx_};
  std::uint32_t dsv_idx{invalid_descriptor_idx_};

  if (desc.usage & Texture::kUsageShaderResource) {
    auto const srv_desc{
      [&desc] {
        D3D12_SHADER_RESOURCE_VIEW_DESC ret{};
        ret.Format = FormatToDxgiFormat(desc.format);
        ret.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (desc.dimension == Texture::Dimension::k1D) {
          if (desc.depth_or_array_size > 1) {
            ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            ret.Texture1DArray = {
              .MostDetailedMip = 0, .MipLevels = desc.mip_count,
              .FirstArraySlice = 0, .ArraySize = desc.depth_or_array_size,
              .ResourceMinLODClamp = 0
            };
          } else {
            ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            ret.Texture1D = {
              .MostDetailedMip = 0, .MipLevels = desc.mip_count,
              .ResourceMinLODClamp = 0
            };
          }
        } else if (desc.dimension == Texture::Dimension::k2D) {
          if (desc.sample_count > 1) {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
              ret.Texture2DMSArray = {
                .FirstArraySlice = 0, .ArraySize = desc.depth_or_array_size
              };
            } else {
              ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
          } else {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
              ret.Texture2DArray = {
                .MostDetailedMip = 0, .MipLevels = desc.mip_count,
                .FirstArraySlice = 0, .ArraySize = desc.depth_or_array_size,
                .PlaneSlice = 0, .ResourceMinLODClamp = 0
              };
            } else {
              ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
              ret.Texture2D = {
                .MostDetailedMip = 0, .MipLevels = desc.mip_count,
                .PlaneSlice = 0, .ResourceMinLODClamp = 0
              };
            }
          }
        } else if (desc.dimension == Texture::Dimension::k3D) {
          ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
          ret.Texture3D = {
            .MostDetailedMip = 0, .MipLevels = desc.mip_count,
            .ResourceMinLODClamp = 0
          };
        } else if (desc.dimension == Texture::Dimension::kCube) {
          ret.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
          ret.TextureCube = {
            .MostDetailedMip = 0, .MipLevels = desc.mip_count,
            .ResourceMinLODClamp = 0
          };
        }
        return ret;
      }()
    };
    srv_idx = CreateShaderResourceView(allocation->GetResource(), srv_desc);
  }

  if (desc.usage & Texture::kUsageUnorderedAccess) {
    auto const uav_desc{
      [&desc] {
        D3D12_UNORDERED_ACCESS_VIEW_DESC ret{};
        ret.Format = FormatToDxgiFormat(desc.format);
        if (desc.dimension == Texture::Dimension::k1D) {
          if (desc.depth_or_array_size > 1) {
            ret.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            ret.Texture1DArray = {
              .MipSlice = 0, .FirstArraySlice = 0, .ArraySize = desc.mip_count
            };
          } else {
            ret.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            ret.Texture1D = {.MipSlice = 0};
          }
        } else if (desc.dimension == Texture::Dimension::k2D) {
          if (desc.depth_or_array_size > 1) {
            ret.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            ret.Texture2DArray = {
              .MipSlice = 0, .FirstArraySlice = 0,
              .ArraySize = desc.depth_or_array_size, .PlaneSlice = 0
            };
          } else {
            ret.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            ret.Texture2D = {.MipSlice = 0, .PlaneSlice = 0};
          }
        } else if (desc.dimension == Texture::Dimension::k3D) {
          ret.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
          ret.Texture3D = {
            .MipSlice = 0, .FirstWSlice = 0, .WSize = desc.depth_or_array_size
          };
        }
        return ret;
      }()
    };
    uav_idx = CreateUnorderedAccessView(allocation->GetResource(), uav_desc);
  }

  if (desc.usage & Texture::kUsageRenderTarget) {
    auto const rtv_desc{
      [&desc] {
        D3D12_RENDER_TARGET_VIEW_DESC ret{};
        ret.Format = FormatToDxgiFormat(desc.format);
        if (desc.dimension == Texture::Dimension::k1D) {
          if (desc.depth_or_array_size > 1) {
            ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            ret.Texture1DArray = {
              .MipSlice = 0, .FirstArraySlice = 0, .ArraySize = desc.mip_count
            };
          } else {
            ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            ret.Texture1D = {.MipSlice = 0};
          }
        } else if (desc.dimension == Texture::Dimension::k2D) {
          if (desc.sample_count > 1) {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
              ret.Texture2DMSArray = {
                .FirstArraySlice = 0, .ArraySize = desc.depth_or_array_size,
              };
            } else {
              ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
          } else {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
              ret.Texture2DArray = {
                .MipSlice = 0, .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_size, .PlaneSlice = 0
              };
            } else {
              ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
              ret.Texture2D = {.MipSlice = 0, .PlaneSlice = 0};
            }
          }
        } else if (desc.dimension == Texture::Dimension::k3D) {
          ret.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
          ret.Texture3D = {
            .MipSlice = 0, .FirstWSlice = 0, .WSize = desc.depth_or_array_size
          };
        }
        return ret;
      }()
    };
    rtv_idx = CreateRenderTargetView(allocation->GetResource(), rtv_desc);
  }

  if (desc.usage & Texture::kUsageDepthStencil) {
    auto const dsv_desc{
      [&desc] {
        D3D12_DEPTH_STENCIL_VIEW_DESC ret{};
        ret.Format = FormatToDxgiFormat(desc.format);
        ret.Flags = D3D12_DSV_FLAG_NONE;
        if (desc.dimension == Texture::Dimension::k1D) {
          if (desc.depth_or_array_size > 1) {
            ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            ret.Texture1DArray = {
              .MipSlice = 0, .FirstArraySlice = 0, .ArraySize = desc.mip_count
            };
          } else {
            ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            ret.Texture1D = {.MipSlice = 0};
          }
        } else if (desc.dimension == Texture::Dimension::k2D) {
          if (desc.sample_count > 1) {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
              ret.Texture2DMSArray = {
                .FirstArraySlice = 0, .ArraySize = desc.depth_or_array_size,
              };
            } else {
              ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
          } else {
            if (desc.depth_or_array_size > 1) {
              ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
              ret.Texture2DArray = {
                .MipSlice = 0, .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_size
              };
            } else {
              ret.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
              ret.Texture2D = {.MipSlice = 0};
            }
          }
        }
        return ret;
      }()
    };
    dsv_idx = CreateDepthStencilView(allocation->GetResource(), dsv_desc);
  }

  return std::make_unique<TextureD3D12>(desc, this, std::move(allocation),
                                        srv_idx, uav_idx, rtv_idx, dsv_idx,
                                        false);
}

auto DeviceD3D12::CreatePipelineState(
  GraphicsPipelineStateInfo const& info) -> std::unique_ptr<PipelineState> {
  D3D12_ROOT_PARAMETER const root_param{
    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
    .Constants = {
      .ShaderRegister = 0, .RegisterSpace = 0,
      .Num32BitValues = info.parameter_count
    },
    .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
  };

  D3D12_ROOT_SIGNATURE_DESC const root_signature_desc{
    .NumParameters = 1, .pParameters = &root_param, .NumStaticSamplers = 0,
    .pStaticSamplers = nullptr,
    .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
    D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
  };

  Microsoft::WRL::ComPtr<ID3DBlob> root_signature_blob;
  Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
  ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc,
                                            D3D_ROOT_SIGNATURE_VERSION_1,
                                            &root_signature_blob, &error_blob));

  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
  ThrowIfFailed(device_->CreateRootSignature(
    0, root_signature_blob->GetBufferPointer(),
    root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{
    root_signature.Get(),
    CD3DX12_SHADER_BYTECODE{
      info.vertex_shader.data(), info.vertex_shader.size_bytes()
    },
    CD3DX12_SHADER_BYTECODE{
      info.pixel_shader.data(), info.pixel_shader.size_bytes()
    },
    CD3DX12_SHADER_BYTECODE{
      info.domain_shader.data(), info.domain_shader.size_bytes()
    },
    CD3DX12_SHADER_BYTECODE{
      info.hull_shader.data(), info.hull_shader.size_bytes()
    },
    CD3DX12_SHADER_BYTECODE{
      info.geometry_shader.data(), info.geometry_shader.size_bytes()
    },
    {},
    {
      info.multisample_state.alpha_to_coverage_enable,
      info.blend_state.logic_op ? FALSE : TRUE, {}
    },
    0xFFFFFFFF,
    {
      PolygonFillModeToD3DFillMode(info.rasterizer_state.fill_mode),
      FaceCullingModeToD3DCullMode(info.rasterizer_state.cull_mode),
      info.rasterizer_state.front_face == FrontFace::kCounterClockwise
        ? TRUE
        : FALSE,
      static_cast<INT>(info.rasterizer_state.depth_bias),
      info.rasterizer_state.depth_bias_clamp,
      info.rasterizer_state.depth_bias_slope, TRUE, FALSE, FALSE, 0,
      D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    },
    {
      info.depth_stencil_state.depth_test_enable,
      info.depth_stencil_state.depth_write_enable
        ? D3D12_DEPTH_WRITE_MASK_ALL
        : D3D12_DEPTH_WRITE_MASK_ZERO,
      CompareOpToD3DComparisonFunc(info.depth_stencil_state.depth_compare_op),
      info.depth_stencil_state.stencil_test_enable,
      info.depth_stencil_state.stencil_read_mask,
      info.depth_stencil_state.stencil_write_mask,
      {
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_front_face.fail_op),
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_front_face.depth_fail_op),
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_front_face.pass_op),
        CompareOpToD3DComparisonFunc(
          info.depth_stencil_state.stencil_front_face.compare_op)
      },
      {
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_back_face.fail_op),
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_back_face.depth_fail_op),
        StencilOpToD3D12StencilOp(
          info.depth_stencil_state.stencil_back_face.pass_op),
        CompareOpToD3DComparisonFunc(
          info.depth_stencil_state.stencil_back_face.compare_op)
      }
    },
    {},
    info.input_assembly.primitive_restart_enable
      ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF
      : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
    PrimitiveTopologyToD3DPrimitiveTopologyType(
      info.input_assembly.primitive_topology),
    info.render_target_count, {},
    FormatToDxgiFormat(info.formats.depth_stencil),
    {info.multisample_state.sample_count, 0}, 0, {},
    D3D12_PIPELINE_STATE_FLAG_NONE
  };

  for (auto i{0}; i < info.render_target_count; i++) {
    desc.BlendState.RenderTarget[i].BlendEnable = info.blend_state.
      render_targets[i].blend_enable;
    desc.BlendState.RenderTarget[i].BlendOp = BlendOpToD3DBlendOp(
      info.blend_state.render_targets[i].color_blend_op);
    desc.BlendState.RenderTarget[i].BlendOpAlpha = BlendOpToD3DBlendOp(
      info.blend_state.render_targets[i].alpha_blend_op);
    desc.BlendState.RenderTarget[i].SrcBlend = BlendFactorToD3DBlend(
      info.blend_state.render_targets[i].src_color_blend_factor);
    desc.BlendState.RenderTarget[i].SrcBlendAlpha = BlendFactorToD3DBlend(
      info.blend_state.render_targets[i].src_alpha_blend_factor);
    desc.BlendState.RenderTarget[i].DestBlend = BlendFactorToD3DBlend(
      info.blend_state.render_targets[i].dst_color_blend_factor);
    desc.BlendState.RenderTarget[i].DestBlendAlpha = BlendFactorToD3DBlend(
      info.blend_state.render_targets[i].dst_alpha_blend_factor);
    desc.BlendState.RenderTarget[i].LogicOp = info.blend_state.logic_op
                                                ? LogicOpToD3DLogicOp(
                                                  *info.blend_state.logic_op)
                                                : D3D12_LOGIC_OP{};
    desc.BlendState.RenderTarget[i].LogicOpEnable = info.blend_state.logic_op.
      has_value();
    desc.BlendState.RenderTarget[i].RenderTargetWriteMask =
      ColorWriteToD3DWriteMask(
        info.blend_state.render_targets[i].color_write_mask);
  }

  for (auto i{0}; i < info.render_target_count; i++) {
    desc.RTVFormats[i] = FormatToDxgiFormat(info.formats.render_targets[i]);
  }

  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
  ThrowIfFailed(
    device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));

  return std::make_unique<PipelineStateD3D12>(std::move(root_signature),
                                              std::move(pso));
}
}

#endif
