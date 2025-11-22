#include "wand/wand.hpp"

#include <dxgidebug.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "wand/format.hpp"
#include "wand/util.hpp"

using Microsoft::WRL::ComPtr;


// Agility SDK exports
extern "C" {
__declspec(dllexport) extern UINT const D3D12SDKVersion{D3D12_SDK_VERSION};
__declspec(dllexport) extern char const* D3D12SDKPath{".\\D3D12\\"};
}


namespace wand {
UINT const GraphicsDevice::rtv_heap_size_{1'000'000};
UINT const GraphicsDevice::dsv_heap_size_{1'000'000};
UINT const GraphicsDevice::res_desc_heap_size_{1'000'000};
UINT const GraphicsDevice::sampler_heap_size_{2048};


namespace {
auto AsD3d12Desc(BufferDesc const& desc) -> D3D12_RESOURCE_DESC1 {
  auto flags{D3D12_RESOURCE_FLAG_NONE};

  if (!desc.shader_resource) {
    flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  if (desc.unordered_access) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  return CD3DX12_RESOURCE_DESC1::Buffer(desc.size, flags);
}


auto AsD3d12Desc(TextureDesc const& desc) -> D3D12_RESOURCE_DESC1 {
  auto flags{D3D12_RESOURCE_FLAG_NONE};

  if (desc.depth_stencil) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  }

  if (desc.render_target) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }

  if (!desc.shader_resource) {
    flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  if (desc.unordered_access) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  switch (desc.dimension) {
  case TextureDimension::k1D: {
    return CD3DX12_RESOURCE_DESC1::Tex1D(desc.format, desc.width, desc.depth_or_array_size, desc.mip_levels, flags);
  }
  case TextureDimension::k2D: [[fallthrough]];
  case TextureDimension::kCube: {
    return CD3DX12_RESOURCE_DESC1::Tex2D(desc.format, desc.width, desc.height, desc.depth_or_array_size,
                                         desc.mip_levels, desc.sample_count, 0, flags);
  }
  case TextureDimension::k3D: {
    return CD3DX12_RESOURCE_DESC1::Tex3D(desc.format, desc.width, desc.height, desc.depth_or_array_size,
                                         desc.mip_levels, flags);
  }
  }

  throw std::runtime_error{"Trying to convert invalid an TextureDesc to D3D12_RESOURCE_DESC1."};
}
}


GraphicsDevice::GraphicsDevice(bool const enable_debug, bool const use_sw_rendering) {
  if (enable_debug) {
    ComPtr<ID3D12Debug6> debug;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "Failed to get D3D12 debug interface.");
    debug->EnableDebugLayer();

    ComPtr<IDXGIInfoQueue> dxgi_info_queue;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)), "Failed to get DXGI debug interface.");
    ThrowIfFailed(dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE),
                  "Failed to set debug break on DXGI error.");
    ThrowIfFailed(
      dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE),
      "Failed to set debug break on DXGI corruption.");
  }

  UINT factory_create_flags{0};

  if (enable_debug) {
    factory_create_flags |= DXGI_CREATE_FACTORY_DEBUG;
  }

  ThrowIfFailed(CreateDXGIFactory2(factory_create_flags, IID_PPV_ARGS(&factory_)), "Failed to create DXGI factory.");

  ComPtr<IDXGIAdapter4> adapter;

  if (use_sw_rendering) {
    ThrowIfFailed(factory_->EnumWarpAdapter(IID_PPV_ARGS(&adapter)), "Failed to get WARP adapter.");
  } else {
    ThrowIfFailed(factory_->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)),
                  "Failed to get high performance adapter.");
  }

  ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)),
                "Failed to create D3D12 device.");

  if (enable_debug) {
    ComPtr<ID3D12InfoQueue> d3d12_info_queue;
    ThrowIfFailed(device_.As(&d3d12_info_queue), "Failed to get D3D12 info queue.");
    ThrowIfFailed(d3d12_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE),
                  "Failed to set debug break on D3D12 error.");
    ThrowIfFailed(d3d12_info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE),
                  "Failed to set debug break on D3D12 corruption.");
  }

  ThrowIfFailed(supported_features_.Init(device_.Get()), "Failed to query D3D12 features.");

  if (supported_features_.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3) {
    throw std::runtime_error{"Resource Bindig Tier 3 is required but not supported."};
  }

  if (supported_features_.HighestShaderModel() < D3D_SHADER_MODEL_6_6) {
    throw std::runtime_error{"Shader Model 6.6 is required but not supported."};
  }

  if (!supported_features_.EnhancedBarriersSupported()) {
    throw std::runtime_error{"Enhanced barriers is required but not supported."};
  }

  if (supported_features_.HighestRootSignatureVersion() < D3D_ROOT_SIGNATURE_VERSION_1_1) {
    throw std::runtime_error{"Root Signature 1.1 is required but no supported."};
  }

  if (!supported_features_.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation()) {
    throw std::runtime_error{
      "Viewport and render target array index outside of geometry shaders is required but no supported."
    };
  }

  if (supported_features_.MeshShaderTier() < D3D12_MESH_SHADER_TIER_1) {
    throw std::runtime_error{"Mesh Shader Tier 1 is required but not supported."};
  }

  D3D12MA::ALLOCATOR_DESC const allocator_desc{D3D12MA::ALLOCATOR_FLAG_NONE, device_.Get(), 0, nullptr, adapter.Get()};
  ThrowIfFailed(CreateAllocator(&allocator_desc, &allocator_), "Failed to create D3D12 memory allocator.");

  D3D12_DESCRIPTOR_HEAP_DESC constexpr rtv_heap_desc{
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtv_heap_size_, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0
  };
  ComPtr<ID3D12DescriptorHeap> rtv_heap;
  ThrowIfFailed(device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)), "Failed to create RTV heap.");
  rtv_heap_ = std::make_unique<details::DescriptorHeap>(std::move(rtv_heap), *device_.Get());

  D3D12_DESCRIPTOR_HEAP_DESC constexpr dsv_heap_desc{
    D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsv_heap_size_, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0
  };
  ComPtr<ID3D12DescriptorHeap> dsv_heap;
  ThrowIfFailed(device_->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap)), "Failed to create DSV heap.");
  dsv_heap_ = std::make_unique<details::DescriptorHeap>(std::move(dsv_heap), *device_.Get());

  D3D12_DESCRIPTOR_HEAP_DESC constexpr res_desc_heap_desc{
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, res_desc_heap_size_, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0
  };
  ComPtr<ID3D12DescriptorHeap> res_desc_heap;
  ThrowIfFailed(device_->CreateDescriptorHeap(&res_desc_heap_desc, IID_PPV_ARGS(&res_desc_heap)),
                "Failed to create resource descriptor heap.");
  res_desc_heap_ = std::make_unique<details::DescriptorHeap>(std::move(res_desc_heap), *device_.Get());

  D3D12_DESCRIPTOR_HEAP_DESC constexpr sampler_heap_desc{
    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, sampler_heap_size_, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0
  };
  ComPtr<ID3D12DescriptorHeap> sampler_heap;
  ThrowIfFailed(device_->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(&sampler_heap)),
                "Failed to create sampler heap.");
  sampler_heap_ = std::make_unique<details::DescriptorHeap>(std::move(sampler_heap), *device_.Get());

  D3D12_COMMAND_QUEUE_DESC constexpr queue_desc{
    D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0
  };
  ThrowIfFailed(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue_)), "Failed to create command queue.");

  if (BOOL allow_tearing; SUCCEEDED(
    factory_->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)
    )) && allow_tearing) {
    swap_chain_flags_ |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    present_flags_ |= DXGI_PRESENT_ALLOW_TEARING;
  }

  idle_fence_ = CreateFence(0);
  execute_barrier_fence_ = CreateFence(0);
}


auto GraphicsDevice::CreateBuffer(BufferDesc const& desc,
                                  CpuAccess const cpu_access) -> SharedDeviceChildHandle<Buffer> {
  ComPtr<D3D12MA::Allocation> allocation;
  ComPtr<ID3D12Resource2> resource;

  D3D12MA::ALLOCATION_DESC const alloc_desc{
    D3D12MA::ALLOCATION_FLAG_NONE, MakeHeapType(cpu_access), D3D12_HEAP_FLAG_NONE, nullptr, nullptr
  };

  auto const res_desc{AsD3d12Desc(desc)};

  ThrowIfFailed(allocator_->CreateResource3(&alloc_desc, &res_desc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr,
                                            &allocation, IID_PPV_ARGS(&resource)), "Failed to create buffer.");

  UINT cbv;
  UINT srv;
  UINT uav;

  CreateBufferViews(*resource.Get(), desc, cbv, srv, uav);

  global_resource_states_.Record(resource.Get(), {.layout = D3D12_BARRIER_LAYOUT_UNDEFINED});

  return SharedDeviceChildHandle<Buffer>{
    new Buffer{std::move(allocation), std::move(resource), cbv, srv, uav, desc},
    DeviceChildDeleter<Buffer>{*this}
  };
}


auto GraphicsDevice::CreateTexture(TextureDesc const& desc,
                                   CpuAccess const cpu_access,
                                   D3D12_CLEAR_VALUE const* clear_value) -> SharedDeviceChildHandle<Texture> {
  ComPtr<D3D12MA::Allocation> allocation;
  ComPtr<ID3D12Resource2> resource;

  auto res_desc{AsD3d12Desc(desc)};

  // If a depth format is specified, we have to determine the typeless resource format.
  res_desc.Format = MakeDepthTypeless(desc.format);

  D3D12MA::ALLOCATION_DESC const alloc_desc{
    D3D12MA::ALLOCATION_FLAG_NONE, MakeHeapType(cpu_access), D3D12_HEAP_FLAG_NONE, nullptr, nullptr
  };

  constexpr auto initial_layout{D3D12_BARRIER_LAYOUT_UNDEFINED};

  ThrowIfFailed(allocator_->CreateResource3(&alloc_desc, &res_desc, initial_layout, clear_value, 0, nullptr,
                                            &allocation, IID_PPV_ARGS(&resource)), "Failed to create texture.");

  std::vector<UINT> dsvs;
  std::vector<UINT> rtvs;
  std::optional<UINT> srv;
  std::optional<UINT> uav;

  CreateTextureViews(*resource.Get(), desc, dsvs, rtvs, srv, uav);

  global_resource_states_.Record(resource.Get(), {.layout = initial_layout});

  return SharedDeviceChildHandle<Texture>{
    new Texture{std::move(allocation), std::move(resource), std::move(dsvs), std::move(rtvs), srv, uav, desc},
    DeviceChildDeleter<Texture>{*this}
  };
}


auto GraphicsDevice::CreatePipelineState(PipelineDesc const& desc,
                                         std::uint8_t const num_32_bit_params) -> SharedDeviceChildHandle<
  PipelineState> {
  auto root_signature = GetOrCreateRootSignature(num_32_bit_params);

  ComPtr<ID3D12PipelineState> pipeline_state;

  CD3DX12_PIPELINE_STATE_STREAM2 pso_desc;
  pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
  pso_desc.NodeMask = 0;
  pso_desc.pRootSignature = root_signature.Get();
  pso_desc.PrimitiveTopologyType = desc.primitive_topology_type;
  pso_desc.VS = desc.vs;
  pso_desc.GS = desc.gs;
  pso_desc.StreamOutput = desc.stream_output;
  pso_desc.HS = desc.hs;
  pso_desc.DS = desc.ds;
  pso_desc.PS = desc.ps;
  pso_desc.AS = desc.as;
  pso_desc.MS = desc.ms;
  pso_desc.CS = desc.cs;
  pso_desc.BlendState = desc.blend_state;
  pso_desc.DepthStencilState = desc.depth_stencil_state;
  pso_desc.DSVFormat = desc.ds_format;
  pso_desc.RasterizerState = desc.rasterizer_state;
  pso_desc.RTVFormats = desc.rt_formats;
  pso_desc.SampleDesc = desc.sample_desc;
  pso_desc.SampleMask = desc.sample_mask;
  pso_desc.ViewInstancingDesc = desc.view_instancing_desc;

  D3D12_PIPELINE_STATE_STREAM_DESC const stream_desc{sizeof(pso_desc), &pso_desc};
  ThrowIfFailed(device_->CreatePipelineState(&stream_desc, IID_PPV_ARGS(&pipeline_state)),
                "Failed to create pipeline state.");

  return SharedDeviceChildHandle<PipelineState>{
    new PipelineState{
      std::move(root_signature), std::move(pipeline_state), num_32_bit_params, desc.cs.BytecodeLength != 0,
      desc.depth_stencil_state.DepthEnable && desc.depth_stencil_state.DepthWriteMask != D3D12_DEPTH_WRITE_MASK_ZERO
    },
    DeviceChildDeleter<PipelineState>{*this}
  };
}


auto GraphicsDevice::CreateRtStateObject(RtStateObjectDesc& desc,
                                         std::uint8_t const num_32_bit_params) -> SharedDeviceChildHandle<
  RtStateObject> {
  auto root_signature = GetOrCreateRootSignature(num_32_bit_params);
  auto* const global_root_sig_desc = desc.desc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
  global_root_sig_desc->SetRootSignature(root_signature.Get());

  ComPtr<ID3D12StateObject> state_object;
  ThrowIfFailed(device_->CreateStateObject(desc.desc_, IID_PPV_ARGS(&state_object)),
                "Failed to create RT state object.");

  return SharedDeviceChildHandle<RtStateObject>{
    new RtStateObject{std::move(root_signature), std::move(state_object), num_32_bit_params},
    DeviceChildDeleter<RtStateObject>{*this}
  };
}


auto GraphicsDevice::CreateCommandList() -> SharedDeviceChildHandle<CommandList> {
  ComPtr<ID3D12CommandAllocator> allocator;
  ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
                "Failed to create command allocator.");

  ComPtr<ID3D12GraphicsCommandList7> cmd_list;
  ThrowIfFailed(
    device_->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                                IID_PPV_ARGS(&cmd_list)), "Failed to create command list.");

  return SharedDeviceChildHandle<CommandList>{
    new CommandList{
      std::move(allocator), std::move(cmd_list), dsv_heap_.get(), rtv_heap_.get(), res_desc_heap_.get(),
      sampler_heap_.get(), &root_signatures_
    },
    DeviceChildDeleter<CommandList>{*this}
  };
}


auto GraphicsDevice::CreateFence(UINT64 const initial_value) -> SharedDeviceChildHandle<Fence> {
  ComPtr<ID3D12Fence1> fence;
  ThrowIfFailed(device_->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
                "Failed to create fence.");
  return SharedDeviceChildHandle<Fence>{
    new Fence{std::move(fence), initial_value + 1}, DeviceChildDeleter<Fence>{*this}
  };
}


auto GraphicsDevice::CreateSwapChain(SwapChainDesc const& desc,
                                     HWND const window_handle) -> SharedDeviceChildHandle<SwapChain> {
  DXGI_SWAP_CHAIN_DESC1 const dxgi_desc{
    desc.width, desc.height, desc.format, FALSE, {1, 0}, desc.usage, desc.buffer_count, desc.scaling,
    DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_ALPHA_MODE_UNSPECIFIED, swap_chain_flags_
  };

  ComPtr<IDXGISwapChain1> swap_chain1;
  ThrowIfFailed(
    factory_->CreateSwapChainForHwnd(queue_.Get(), window_handle, &dxgi_desc, nullptr, nullptr, &swap_chain1),
    "Failed to create swap chain.");

  ComPtr<IDXGISwapChain4> swap_chain4;
  ThrowIfFailed(swap_chain1.As(&swap_chain4), "Failed to query IDXGISwapChain4 interface.");
  ThrowIfFailed(factory_->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER),
                "Failed to disable swap chain ALT+ENTER behavior.");

  auto const swap_chain{new SwapChain{std::move(swap_chain4), present_flags_}};
  SwapChainCreateTextures(*swap_chain);

  return SharedDeviceChildHandle<SwapChain>{swap_chain, DeviceChildDeleter<SwapChain>{*this}};
}


auto GraphicsDevice::CreateSampler(D3D12_SAMPLER_DESC const& desc) -> UniqueSamplerHandle {
  auto const sampler{sampler_heap_->Allocate()};
  device_->CreateSampler(&desc, sampler_heap_->GetDescriptorCpuHandle(sampler));
  return UniqueSamplerHandle{sampler, *this};
}


auto GraphicsDevice::CreateAliasingResources(std::span<BufferDesc const> const buffer_descs,
                                             std::span<AliasedTextureCreateInfo const> const texture_infos,
                                             CpuAccess cpu_access,
                                             std::vector<SharedDeviceChildHandle<Buffer>>* buffers,
                                             std::vector<SharedDeviceChildHandle<Texture>>* textures) -> void {
  D3D12_RESOURCE_ALLOCATION_INFO buf_alloc_info{0, 0};
  D3D12_RESOURCE_ALLOCATION_INFO rt_ds_alloc_info{0, 0};
  D3D12_RESOURCE_ALLOCATION_INFO non_rt_ds_alloc_info{0, 0};

  for (auto const& buffer_desc : buffer_descs) {
    auto const desc{AsD3d12Desc(buffer_desc)};
    auto const alloc_info{device_->GetResourceAllocationInfo2(0, 1, &desc, nullptr)};
    buf_alloc_info.Alignment = std::max(buf_alloc_info.Alignment, alloc_info.Alignment);
    buf_alloc_info.SizeInBytes = std::max(buf_alloc_info.SizeInBytes, alloc_info.SizeInBytes);
  }

  for (auto const& info : texture_infos) {
    auto const& desc{AsD3d12Desc(info.desc)};
    auto const alloc_info{device_->GetResourceAllocationInfo2(0, 1, &desc, nullptr)};
    auto& tex_alloc_info{info.desc.render_target || info.desc.depth_stencil ? rt_ds_alloc_info : non_rt_ds_alloc_info};

    tex_alloc_info.Alignment = std::max(tex_alloc_info.Alignment, alloc_info.Alignment);
    tex_alloc_info.SizeInBytes = std::max(tex_alloc_info.SizeInBytes, alloc_info.SizeInBytes);
  }

  auto const heap_type{MakeHeapType(cpu_access)};

  ComPtr<D3D12MA::Allocation> buf_alloc;
  ComPtr<D3D12MA::Allocation> rt_ds_alloc;
  ComPtr<D3D12MA::Allocation> non_rt_ds_alloc;

  if (allocator_->GetD3D12Options().ResourceHeapTier > D3D12_RESOURCE_HEAP_TIER_1) {
    D3D12MA::ALLOCATION_DESC alloc_desc{
      D3D12MA::ALLOCATION_FLAG_NONE, heap_type, D3D12_HEAP_FLAG_NONE, nullptr, nullptr
    };

    if (buf_alloc_info.SizeInBytes == 0) {
      alloc_desc.ExtraHeapFlags |= D3D12_HEAP_FLAG_DENY_BUFFERS;
    }

    if (rt_ds_alloc_info.SizeInBytes == 0) {
      alloc_desc.ExtraHeapFlags |= D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
    }

    if (non_rt_ds_alloc_info.SizeInBytes == 0) {
      alloc_desc.ExtraHeapFlags |= D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
    }

    D3D12_RESOURCE_ALLOCATION_INFO const alloc_info{
      std::max(std::max(buf_alloc_info.SizeInBytes, rt_ds_alloc_info.SizeInBytes), non_rt_ds_alloc_info.SizeInBytes),
      std::max(std::max(buf_alloc_info.Alignment, rt_ds_alloc_info.Alignment), non_rt_ds_alloc_info.Alignment)
    };

    ThrowIfFailed(allocator_->AllocateMemory(&alloc_desc, &alloc_info, &buf_alloc),
                  "Failed to allocate memory for aliasing resources.");

    rt_ds_alloc = buf_alloc;
    non_rt_ds_alloc = buf_alloc;
  } else {
    if (buf_alloc_info.SizeInBytes > 0) {
      D3D12MA::ALLOCATION_DESC const buf_alloc_desc{
        D3D12MA::ALLOCATION_FLAG_NONE, heap_type, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, nullptr, nullptr
      };

      ThrowIfFailed(allocator_->AllocateMemory(&buf_alloc_desc, &buf_alloc_info, &buf_alloc),
                    "Failed to allocate memory for aliasing buffers.");
    }

    if (rt_ds_alloc_info.SizeInBytes > 0) {
      D3D12MA::ALLOCATION_DESC const rt_ds_alloc_desc{
        D3D12MA::ALLOCATION_FLAG_NONE, heap_type, D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES, nullptr, nullptr
      };

      ThrowIfFailed(allocator_->AllocateMemory(&rt_ds_alloc_desc, &rt_ds_alloc_info, &rt_ds_alloc),
                    "Failed to allocate memory for aliasing RT/DS textures.");
    }

    if (non_rt_ds_alloc_info.SizeInBytes > 0) {
      D3D12MA::ALLOCATION_DESC const non_rt_ds_alloc_desc{
        D3D12MA::ALLOCATION_FLAG_NONE, heap_type, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES, nullptr, nullptr
      };

      ThrowIfFailed(allocator_->AllocateMemory(&non_rt_ds_alloc_desc, &non_rt_ds_alloc_info, &non_rt_ds_alloc),
                    "Failed to allocate memory for aliasing non-RT/DS textures.");
    }
  }

  if (buffers) {
    for (auto const& buf_desc : buffer_descs) {
      auto const desc{AsD3d12Desc(buf_desc)};
      ComPtr<ID3D12Resource2> resource;

      ThrowIfFailed(allocator_->CreateAliasingResource2(buf_alloc.Get(), 0, &desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
                                                        nullptr, 0, nullptr, IID_PPV_ARGS(&resource)),
                    "Failed to create aliasing buffer.");

      UINT cbv;
      UINT srv;
      UINT uav;
      CreateBufferViews(*resource.Get(), buf_desc, cbv, srv, uav);
      buffers->emplace_back(new Buffer{buf_alloc, std::move(resource), cbv, srv, uav, buf_desc},
                            DeviceChildDeleter<Buffer>{*this});
    }
  }

  if (textures) {
    for (auto const& info : texture_infos) {
      auto const desc{AsD3d12Desc(info.desc)};

      auto& alloc{info.desc.render_target || info.desc.depth_stencil ? rt_ds_alloc : non_rt_ds_alloc};

      ComPtr<ID3D12Resource2> resource;
      ThrowIfFailed(allocator_->CreateAliasingResource2(alloc.Get(), 0, &desc, info.initial_layout, info.clear_value, 0,
                                                        nullptr, IID_PPV_ARGS(&resource)),
                    "Failed to create aliasing texture.");

      std::vector<UINT> dsvs;
      std::vector<UINT> rtvs;
      std::optional<UINT> srv;
      std::optional<UINT> uav;
      CreateTextureViews(*resource.Get(), info.desc, dsvs, rtvs, srv, uav);
      textures->emplace_back(new Texture{
                               alloc, std::move(resource), std::move(dsvs), std::move(rtvs), srv, uav, info.desc
                             }, DeviceChildDeleter<Texture>{*this});
    }
  }
}


auto GraphicsDevice::DestroyBuffer(Buffer const* const buffer) const -> void {
  if (buffer) {
    if (buffer->cbv_) {
      res_desc_heap_->Release(*buffer->cbv_);
    }

    if (buffer->srv_) {
      res_desc_heap_->Release(*buffer->srv_);
    }

    if (buffer->uav_) {
      res_desc_heap_->Release(*buffer->uav_);
    }

    delete buffer;
  }
}


auto GraphicsDevice::DestroyTexture(Texture const* const texture) const -> void {
  if (texture) {
    std::ranges::for_each(texture->dsvs_, [this](UINT const dsv) {
      dsv_heap_->Release(dsv);
    });

    std::ranges::for_each(texture->rtvs_, [this](UINT const rtv) {
      rtv_heap_->Release(rtv);
    });

    if (texture->srv_) {
      res_desc_heap_->Release(*texture->srv_);
    }

    if (texture->uav_) {
      res_desc_heap_->Release(*texture->uav_);
    }

    delete texture;
  }
}


auto GraphicsDevice::DestroyPipelineState(PipelineState const* const pipeline_state) const -> void {
  delete pipeline_state;
}


auto GraphicsDevice::DestroyCommandList(CommandList const* const command_list) const -> void {
  delete command_list;
}


auto GraphicsDevice::DestroyFence(Fence const* const fence) const -> void {
  delete fence;
}


auto GraphicsDevice::DestroySwapChain(SwapChain const* const swap_chain) const -> void {
  delete swap_chain;
}


auto GraphicsDevice::DestroySampler(UINT const sampler) const -> void {
  sampler_heap_->Release(sampler);
}


auto GraphicsDevice::WaitFence(Fence const& fence, UINT64 const wait_value) const -> void {
  ThrowIfFailed(queue_->Wait(fence.fence_.Get(), wait_value), "Failed to wait fence from GPU queue.");
}


auto GraphicsDevice::SignalFence(Fence& fence) const -> void {
  auto const new_fence_val{fence.next_val_.load()};
  ThrowIfFailed(queue_->Signal(fence.fence_.Get(), new_fence_val), "Failed to signal fence from GPU queue.");
  fence.next_val_ = new_fence_val + 1;
}


auto GraphicsDevice::ExecuteCommandLists(std::span<CommandList const> const cmd_lists) -> void {
  std::vector<D3D12_TEXTURE_BARRIER> pending_tex_barriers;

  for (auto const& cmd_list : cmd_lists) {
    for (auto const& pending_barrier : cmd_list.pending_barriers_) {
      auto const global_state{global_resource_states_.Get(pending_barrier.resource)};
      auto layout_before{global_state ? global_state->layout : D3D12_BARRIER_LAYOUT_UNDEFINED};

      pending_tex_barriers.emplace_back(D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                        D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                        layout_before, pending_barrier.layout, pending_barrier.resource,
                                        D3D12_BARRIER_SUBRESOURCE_RANGE{
                                          .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0,
                                          .NumArraySlices = 0,
                                          .FirstPlane = 0, .NumPlanes = 0
                                        }, D3D12_TEXTURE_BARRIER_FLAG_NONE);
    }

    for (auto const& [res, state] : cmd_list.local_resource_states_) {
      global_resource_states_.Record(res, {.layout = state.layout});
    }
  }

  D3D12_BARRIER_GROUP const pending_barrier_group{
    .Type = D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = ClampCast<UINT32>(pending_tex_barriers.size()),
    .pTextureBarriers = pending_tex_barriers.data()
  };

  auto& pending_barrier_cmd{AcquirePendingBarrierCmdList()};

  pending_barrier_cmd.Begin(nullptr);
  pending_barrier_cmd.cmd_list_->Barrier(1, &pending_barrier_group);
  pending_barrier_cmd.End();
  queue_->ExecuteCommandLists(1,
                              std::array{static_cast<ID3D12CommandList*>(pending_barrier_cmd.cmd_list_.Get())}.data());
  SignalFence(*execute_barrier_fence_);

  std::vector<ID3D12CommandList*> submit_list;
  submit_list.reserve(cmd_lists.size());
  std::ranges::transform(cmd_lists, std::back_inserter(submit_list), [](CommandList const& cmd_list) {
    return cmd_list.cmd_list_.Get();
  });
  queue_->ExecuteCommandLists(static_cast<UINT>(submit_list.size()), submit_list.data());
}


auto GraphicsDevice::WaitIdle() const -> void {
  auto const fence_val{idle_fence_->GetNextValue()};
  SignalFence(*idle_fence_);
  idle_fence_->Wait(fence_val);
}


auto GraphicsDevice::ResizeSwapChain(SwapChain& swap_chain, UINT const width, UINT const height) -> void {
  swap_chain.textures_.clear();
  ThrowIfFailed(swap_chain.swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, swap_chain_flags_),
                "Failed to resize swap chain buffers.");
  SwapChainCreateTextures(swap_chain);
}


auto GraphicsDevice::Present(SwapChain const& swap_chain) -> void {
  auto const cur_tex{swap_chain.GetCurrentTexture().resource_.Get()};
  auto const state{global_resource_states_.Get(cur_tex)};
  auto const layout_before{state ? state->layout : D3D12_BARRIER_LAYOUT_UNDEFINED};

  if (!state || state->layout != D3D12_BARRIER_LAYOUT_PRESENT) {
    global_resource_states_.Record(cur_tex, {.layout = D3D12_BARRIER_LAYOUT_PRESENT});

    D3D12_TEXTURE_BARRIER const barrier{
      D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
      D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
      layout_before, D3D12_BARRIER_LAYOUT_PRESENT,
      swap_chain.GetCurrentTexture().resource_.Get(),
      {
        .IndexOrFirstMipLevel = 0xffffffff, .NumMipLevels = 0, .FirstArraySlice = 0, .NumArraySlices = 0,
        .FirstPlane = 0, .NumPlanes = 0
      },
      D3D12_TEXTURE_BARRIER_FLAG_NONE
    };

    D3D12_BARRIER_GROUP const barrier_group{
      .Type = D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = 1, .pTextureBarriers = &barrier
    };

    auto& cmd_list{AcquirePendingBarrierCmdList()};
    cmd_list.Begin(nullptr);
    cmd_list.cmd_list_->Barrier(1, &barrier_group);
    cmd_list.End();

    queue_->ExecuteCommandLists(1, std::array{static_cast<ID3D12CommandList*>(cmd_list.cmd_list_.Get())}.data());
    SignalFence(*execute_barrier_fence_);
  }

  ThrowIfFailed(swap_chain.swap_chain_->Present(swap_chain.GetSyncInterval(), present_flags_),
                "Failed to present swap chain.");
}


auto GraphicsDevice::GetCopyableFootprints(TextureDesc const& desc, UINT const first_subresource,
                                           UINT const subresource_count, UINT64 const base_offset,
                                           D3D12_PLACED_SUBRESOURCE_FOOTPRINT* const layouts, UINT* const row_counts,
                                           UINT64* const row_sizes, UINT64* const total_size) const -> void {
  auto const tex_desc{AsD3d12Desc(desc)};
  return device_->GetCopyableFootprints1(&tex_desc, first_subresource, subresource_count, base_offset, layouts,
                                         row_counts, row_sizes, total_size);
}


auto GraphicsDevice::GetRtAccelerationStructurePrebuildInfo(
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS const& inputs) const ->
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO {
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
  device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
  return info;
}


auto GraphicsDevice::SwapChainCreateTextures(SwapChain& swap_chain) -> void {
  DXGI_SWAP_CHAIN_DESC1 desc;
  ThrowIfFailed(swap_chain.swap_chain_->GetDesc1(&desc), "Failed to retrieve swap chain desc.");

  TextureDesc const tex_desc{
    TextureDimension::k2D, desc.Width, desc.Height, 1, 1, desc.Format, 1, false,
    (desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) != 0, (desc.BufferUsage & DXGI_USAGE_SHADER_INPUT) != 0, false
  };

  for (UINT i{0}; i < desc.BufferCount; i++) {
    ComPtr<ID3D12Resource2> buf;
    ThrowIfFailed(swap_chain.swap_chain_->GetBuffer(i, IID_PPV_ARGS(&buf)), "Failed to retrieve swap chain buffer.");

    std::vector<UINT> dsvs;
    std::vector<UINT> rtvs;
    std::optional<UINT> srv;
    std::optional<UINT> uav;

    CreateTextureViews(*buf.Get(), tex_desc, dsvs, rtvs, srv, uav);

    global_resource_states_.Record(buf.Get(), {.layout = D3D12_BARRIER_LAYOUT_COMMON});

    swap_chain.textures_.emplace_back(new Texture{
                                        nullptr, std::move(buf), {}, std::move(rtvs), srv,
                                        kInvalidResourceIndex, tex_desc
                                      }, DeviceChildDeleter<Texture>{*this});
  }
}


auto GraphicsDevice::CreateBufferViews(ID3D12Resource2& buffer, BufferDesc const& desc, UINT& cbv, UINT& srv,
                                       UINT& uav) const -> void {
  if (desc.constant_buffer) {
    cbv = res_desc_heap_->Allocate();
    D3D12_CONSTANT_BUFFER_VIEW_DESC const cbv_desc{buffer.GetGPUVirtualAddress(), static_cast<UINT>(desc.size)};
    device_->CreateConstantBufferView(&cbv_desc, res_desc_heap_->GetDescriptorCpuHandle(cbv));
  } else {
    cbv = kInvalidResourceIndex;
  }

  if (desc.shader_resource) {
    srv = res_desc_heap_->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC const srv_desc{
      .Format = desc.stride == 1 ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER, .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Buffer = {
        0, static_cast<UINT>(desc.size / (desc.stride == 1 ? 4 : desc.stride)), desc.stride == 1 ? 0 : desc.stride,
        desc.stride == 1 ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE
      }
    };
    device_->CreateShaderResourceView(&buffer, &srv_desc, res_desc_heap_->GetDescriptorCpuHandle(srv));
  } else {
    srv = kInvalidResourceIndex;
  }

  if (desc.unordered_access) {
    uav = res_desc_heap_->Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC const uav_desc{
      .Format = desc.stride == 1 ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_UAV_DIMENSION_BUFFER, .Buffer = {
        .FirstElement = 0, .NumElements = static_cast<UINT>(desc.size / (desc.stride == 1 ? 4 : desc.stride)),
        .StructureByteStride = desc.stride == 1 ? 0 : desc.stride, .CounterOffsetInBytes = 0,
        .Flags = desc.stride == 1 ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE
      }
    };
    device_->CreateUnorderedAccessView(&buffer, nullptr, &uav_desc, res_desc_heap_->GetDescriptorCpuHandle(uav));
  } else {
    uav = kInvalidResourceIndex;
  }
}


auto GraphicsDevice::CreateTextureViews(ID3D12Resource2& texture, TextureDesc const& desc, std::vector<UINT>& dsvs,
                                        std::vector<UINT>& rtvs, std::optional<UINT>& srv,
                                        std::optional<UINT>& uav) const -> void {
  DXGI_FORMAT dsv_format;
  DXGI_FORMAT rtv_srv_uav_format;

  // If a depth format is specified, we have to determine the rtv/srv/uav format.
  if (desc.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
    dsv_format = desc.format;
    rtv_srv_uav_format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
  } else if (desc.format == DXGI_FORMAT_D32_FLOAT) {
    dsv_format = desc.format;
    rtv_srv_uav_format = DXGI_FORMAT_R32_FLOAT;
  } else if (desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT) {
    dsv_format = desc.format;
    rtv_srv_uav_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  } else if (desc.format == DXGI_FORMAT_D16_UNORM) {
    dsv_format = desc.format;
    rtv_srv_uav_format = DXGI_FORMAT_R16_UNORM;
  } else {
    dsv_format = desc.format;
    rtv_srv_uav_format = desc.format;
  }

  auto const actual_mip_levels{GetActualMipLevels(desc)};

  if (desc.depth_stencil) {
    dsvs.reserve(actual_mip_levels);

    for (UINT16 i{0}; i < actual_mip_levels; ++i) {
      D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{.Format = dsv_format, .Flags = D3D12_DSV_FLAG_NONE};

      if (desc.dimension == TextureDimension::k1D) {
        if (desc.depth_or_array_size == 1) {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
          dsv_desc.Texture1D.MipSlice = i;
        } else {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
          dsv_desc.Texture1DArray.MipSlice = i;
          dsv_desc.Texture1DArray.FirstArraySlice = 0;
          dsv_desc.Texture1DArray.ArraySize = desc.depth_or_array_size;
        }
      } else if (desc.dimension == TextureDimension::k2D && desc.depth_or_array_size == 1) {
        if (desc.sample_count == 1) {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
          dsv_desc.Texture2D.MipSlice = i;
        } else {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
      } else if ((desc.dimension == TextureDimension::k2D && desc.depth_or_array_size > 1) || desc.dimension ==
        TextureDimension::kCube) {
        if (desc.sample_count == 1) {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
          dsv_desc.Texture2DArray.MipSlice = i;
          dsv_desc.Texture2DArray.FirstArraySlice = 0;
          dsv_desc.Texture2DArray.ArraySize = desc.depth_or_array_size;
        } else {
          dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
          dsv_desc.Texture2DMSArray.FirstArraySlice = 0;
          dsv_desc.Texture2DMSArray.ArraySize = desc.depth_or_array_size;
        }
      } else {
        throw std::runtime_error{"Cannot create depth stencil view for texture."};
      }

      dsvs.emplace_back(dsv_heap_->Allocate());
      device_->CreateDepthStencilView(&texture, &dsv_desc, dsv_heap_->GetDescriptorCpuHandle(dsvs.back()));
    }
  }

  if (desc.render_target) {
    rtvs.reserve(actual_mip_levels);

    for (UINT16 i{0}; i < actual_mip_levels; ++i) {
      D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{.Format = rtv_srv_uav_format};

      if (desc.dimension == TextureDimension::k1D) {
        if (desc.depth_or_array_size == 1) {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
          rtv_desc.Texture1D.MipSlice = i;
        } else {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
          rtv_desc.Texture1DArray.MipSlice = i;
          rtv_desc.Texture1DArray.FirstArraySlice = 0;
          rtv_desc.Texture1DArray.ArraySize = desc.depth_or_array_size;
        }
      } else if (desc.dimension == TextureDimension::k2D && desc.depth_or_array_size == 1) {
        if (desc.sample_count == 1) {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
          rtv_desc.Texture2D.MipSlice = i;
          rtv_desc.Texture2D.PlaneSlice = 0;
        } else {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
      } else if ((desc.dimension == TextureDimension::k2D && desc.depth_or_array_size > 1) || desc.dimension ==
        TextureDimension::kCube) {
        if (desc.sample_count == 1) {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
          rtv_desc.Texture2DArray.MipSlice = i;
          rtv_desc.Texture2DArray.FirstArraySlice = 0;
          rtv_desc.Texture2DArray.ArraySize = desc.depth_or_array_size;
          rtv_desc.Texture2DArray.PlaneSlice = 0;
        } else {
          rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
          rtv_desc.Texture2DMSArray.FirstArraySlice = 0;
          rtv_desc.Texture2DMSArray.ArraySize = desc.depth_or_array_size;
        }
      } else if (desc.dimension == TextureDimension::k3D) {
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtv_desc.Texture3D.MipSlice = i;
        rtv_desc.Texture3D.FirstWSlice = 0;
        rtv_desc.Texture3D.WSize = static_cast<UINT>(-1);
      } else {
        throw std::runtime_error{"Cannot create render target view for texture."};
      }

      rtvs.emplace_back(rtv_heap_->Allocate());
      device_->CreateRenderTargetView(&texture, &rtv_desc, rtv_heap_->GetDescriptorCpuHandle(rtvs.back()));
    }
  }

  if (desc.shader_resource) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{
      .Format = rtv_srv_uav_format, .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
    };
    if (desc.dimension == TextureDimension::k1D) {
      if (desc.depth_or_array_size == 1) {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        srv_desc.Texture1D.MostDetailedMip = 0;
        srv_desc.Texture1D.MipLevels = static_cast<UINT>(-1);
        srv_desc.Texture1D.ResourceMinLODClamp = 0.0f;
      } else {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        srv_desc.Texture1DArray.MostDetailedMip = 0;
        srv_desc.Texture1DArray.MipLevels = static_cast<UINT>(-1);
        srv_desc.Texture1DArray.FirstArraySlice = 0;
        srv_desc.Texture1DArray.ArraySize = desc.depth_or_array_size;
        srv_desc.Texture1DArray.ResourceMinLODClamp = 0.0f;
      }
    } else if (desc.dimension == TextureDimension::k2D) {
      if (desc.depth_or_array_size == 1) {
        if (desc.sample_count == 1) {
          srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
          srv_desc.Texture2D.MostDetailedMip = 0;
          srv_desc.Texture2D.MipLevels = static_cast<UINT>(-1);
          srv_desc.Texture2D.PlaneSlice = 0;
          srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
        } else {
          srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
      } else {
        if (desc.sample_count == 1) {
          srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
          srv_desc.Texture2DArray.MostDetailedMip = 0;
          srv_desc.Texture2DArray.MipLevels = static_cast<UINT>(-1);
          srv_desc.Texture2DArray.FirstArraySlice = 0;
          srv_desc.Texture2DArray.ArraySize = desc.depth_or_array_size;
          srv_desc.Texture2DArray.PlaneSlice = 0;
          srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        } else {
          srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
          srv_desc.Texture2DMSArray.FirstArraySlice = 0;
          srv_desc.Texture2DMSArray.ArraySize = desc.depth_or_array_size;
        }
      }
    } else if (desc.dimension == TextureDimension::k3D) {
      srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
      srv_desc.Texture3D.MostDetailedMip = 0;
      srv_desc.Texture3D.MipLevels = static_cast<UINT>(-1);
      srv_desc.Texture3D.ResourceMinLODClamp = 0.0f;
    } else if (desc.dimension == TextureDimension::kCube) {
      if (desc.depth_or_array_size == 6) {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srv_desc.TextureCube.MostDetailedMip = 0;
        srv_desc.TextureCube.MipLevels = static_cast<UINT>(-1);
        srv_desc.TextureCube.ResourceMinLODClamp = 0.0f;
      } else {
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srv_desc.TextureCubeArray.MostDetailedMip = 0;
        srv_desc.TextureCubeArray.MipLevels = static_cast<UINT>(-1);
        srv_desc.TextureCubeArray.First2DArrayFace = 0;
        srv_desc.TextureCubeArray.NumCubes = desc.depth_or_array_size / 6;
        srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
      }
    } else {
      throw std::runtime_error{"Cannot create shader resource view for texture."};
    }
    srv = res_desc_heap_->Allocate();
    device_->CreateShaderResourceView(&texture, &srv_desc, res_desc_heap_->GetDescriptorCpuHandle(*srv));
  }

  if (desc.unordered_access) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{.Format = rtv_srv_uav_format};
    if (desc.dimension == TextureDimension::k1D) {
      if (desc.depth_or_array_size == 1) {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        uav_desc.Texture1D.MipSlice = 0;
      } else {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        uav_desc.Texture1DArray.MipSlice = 0;
        uav_desc.Texture1DArray.FirstArraySlice = 0;
        uav_desc.Texture1DArray.ArraySize = desc.depth_or_array_size;
      }
    } else if (desc.dimension == TextureDimension::k2D && desc.depth_or_array_size == 1) {
      if (desc.sample_count == 1) {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Texture2D.MipSlice = 0;
        uav_desc.Texture2D.PlaneSlice = 0;
      } else {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
      }
    } else if ((desc.dimension == TextureDimension::k2D && desc.depth_or_array_size > 1) || desc.dimension ==
      TextureDimension::k3D) {
      if (desc.sample_count == 1) {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uav_desc.Texture2DArray.MipSlice = 0;
        uav_desc.Texture2DArray.FirstArraySlice = 0;
        uav_desc.Texture2DArray.ArraySize = desc.depth_or_array_size;
        uav_desc.Texture2DArray.PlaneSlice = 0;
      } else {
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
        uav_desc.Texture2DMSArray.FirstArraySlice = 0;
        uav_desc.Texture2DMSArray.ArraySize = desc.depth_or_array_size;
      }
    } else if (desc.dimension == TextureDimension::k3D) {
      uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
      uav_desc.Texture3D.MipSlice = 0;
      uav_desc.Texture3D.FirstWSlice = 0;
      uav_desc.Texture3D.WSize = static_cast<UINT>(-1);
    } else {
      throw std::runtime_error{"Cannot create unordered access view for texture."};
    }
    uav = res_desc_heap_->Allocate();
    device_->CreateUnorderedAccessView(&texture, nullptr, &uav_desc, res_desc_heap_->GetDescriptorCpuHandle(*uav));
  }
}


auto GraphicsDevice::AcquirePendingBarrierCmdList() -> CommandList& {
  std::unique_lock const lock{execute_barrier_mutex_};

  auto const completed_fence_val{execute_barrier_fence_->GetCompletedValue()};
  auto const next_fence_val{execute_barrier_fence_->GetNextValue()};

  for (std::size_t i{0}; i < execute_barrier_cmd_lists_.size(); i++) {
    if (execute_barrier_cmd_lists_[i].fence_completion_val <= completed_fence_val) {
      execute_barrier_cmd_lists_[i].fence_completion_val = next_fence_val;
      return *execute_barrier_cmd_lists_[i].cmd_list;
    }
  }

  return *execute_barrier_cmd_lists_.emplace_back(CreateCommandList(), next_fence_val).cmd_list;
}


auto GraphicsDevice::MakeHeapType(CpuAccess const cpu_access) const -> D3D12_HEAP_TYPE {
  switch (cpu_access) {
  case CpuAccess::kNone:
    return D3D12_HEAP_TYPE_DEFAULT;
  case CpuAccess::kRead:
    return D3D12_HEAP_TYPE_READBACK;
  case CpuAccess::kWrite:
    return supported_features_.GPUUploadHeapSupported() ? D3D12_HEAP_TYPE_GPU_UPLOAD : D3D12_HEAP_TYPE_UPLOAD;
  }

  throw std::runtime_error{"Failed to make D3D12 heap type: unknown CPU access type."};
}


auto GraphicsDevice::GetOrCreateRootSignature(
  std::uint8_t const num_32_bit_params) -> ComPtr<ID3D12RootSignature> {
  auto root_signature{root_signatures_.Get(num_32_bit_params)};

  if (!root_signature) {
    std::array<CD3DX12_ROOT_PARAMETER1, 2> root_params;
    root_params[0].InitAsConstants(num_32_bit_params, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    // 2 params: base vertex and base instance. Make sure this aligns with the shader code!
    root_params[1].InitAsConstants(2, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC const root_signature_desc{
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
      .Desc_1_1 = {
        static_cast<UINT>(root_params.size()), root_params.data(), 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
      }
    };

    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> error_blob;

    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob),
                  "Failed to serialize root signature.");
    ThrowIfFailed(device_->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
                                               root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)),
                  "Failed to create root signature.");

    root_signature = root_signatures_.Add(num_32_bit_params, std::move(root_signature));
  }

  return root_signature;
}
}
