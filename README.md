# 🪄 Wand 🪄

**A lightweight C++23 render hardware interface for Direct3D 12.**

Wand takes care of much of the resource management, descriptor bookkeeping, and synchronization involved in using Direct3D 12, while keeping rendering decisions in your hands. It's designed for projects that want low-level GPU control without having to manage every detail of D3D12 directly.

Wand currently targets **Windows and Direct3D 12 only**. It is a work in progress, and its API still exposes D3D12 concepts and types.

## Features

- **Automatic memory allocation:** buffers and textures are allocated using [D3D12 Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator), with allocations released when their owning resources are destroyed.
- **Automatic state tracking:** resource usage is tracked across command lists, with barriers generated for supported operations.
- **Bindless resource binding:** automatically allocated descriptors, directly indexed resource and sampler heaps, and generated root signatures without per-draw descriptor tables.
- **Rendering and compute:** graphics and compute pipelines, draw and dispatch commands, and mesh-shader dispatch.
- **Raytracing API:** raytracing state objects, acceleration-structure builds, and ray dispatch.
- **Presentation and synchronization:** swapchains, command lists, fences, and GPU synchronization helpers.

## Resource binding

Wand uses **bindless resource binding** through D3D12's directly indexed resource and sampler descriptor heaps. The device manages the heaps and allocates descriptor indices for buffer views, textures, and samplers. Pipeline root signatures are created and cached by Wand rather than defined individually by the application.

Each pipeline receives a block of 32-bit root constants for shader parameters. These can hold ordinary draw parameters or indices into the descriptor heaps. Instead of preparing descriptor tables for each draw, you pass resource indices through this parameter block and access the descriptors directly in HLSL.

For example, given a pipeline with two 32-bit parameters, the C++ side can bind a texture and a sampler:

```cpp
cmd.SetShaderResource(0, texture);           // Constant slot 0: texture descriptor index
cmd.SetPipelineParameter(1, sampler.Get());  // Constant slot 1: sampler descriptor index
```

The corresponding HLSL parameters and resource lookup are:

```hlsl
struct DrawParams {
    uint textureIndex;
    uint samplerIndex;
};

ConstantBuffer<DrawParams> g_params : register(b0, space0);

float4 SampleTexture(float2 uv) {
    Texture2D<float4> texture = ResourceDescriptorHeap[g_params.textureIndex];
    SamplerState sampler = SamplerDescriptorHeap[g_params.samplerIndex];
    return texture.Sample(sampler, uv);
}
```

`SetShaderResource`, `SetConstantBuffer`, and `SetUnorderedAccess` also tell Wand how the bound resource is accessed, allowing it to track state and insert necessary barriers. `SetPipelineParameter` and `SetPipelineParameters` can be used for other 32-bit values, including sampler indices.

**Important:** when shaders obtain descriptor indices indirectly (for example, from a material buffer), Wand cannot automatically infer those resource accesses from the shader. Automatic state tracking applies to accesses Wand sees through its commands.

## Getting started

### Requirements

- Windows with a Direct3D 12 capable GPU and driver
- Visual Studio C++23 capable MSVC platform toolset
- Windows SDK and DirectX Agility SDK support
- NuGet

### Build

```sh
git clone https://github.com/leopph/wand.git
```

Open `wand.slnx` in Visual Studio, restore the NuGet packages, and build the **x64 Debug** or **x64 Release** configuration. The Visual Studio project builds Wand as a DLL.

To use Wand in another Visual Studio project, add a reference to `wand/wand.vcxproj` and include headers from `wand/include`. Your application will also need Wand's runtime DLL and any required runtime dependencies.

### Minimal example

```cpp
#include <span>

#include <wand/wand.hpp>


int main() {
    wand::GraphicsDevice device{/*enable_debug=*/true, /*use_sw_rendering=*/false};

    auto src_buf = device.CreateBuffer(wand::BufferDesc{
        .size = 1024, 
        .usage = wand::BufferUsage::kCopySource
    }, wand::CpuAccess::kWrite);

    auto dst_buf = device.CreateBuffer(wand::BufferDesc{
        .size = 1024, 
        .usage = wand::BufferUsage::kCopyDestination
    }, wand::CpuAccess::kNone);

    auto cmd = device.CreateCommandList();
    cmd->Begin(nullptr);
    cmd->CopyBuffer(*dst_buf, *src_buf);
    cmd->End();

    device.ExecuteCommandLists(std::span{cmd.get(), 1});
    device.WaitIdle();
}
```

This example creates two buffers, records a copy, and submits it to the GPU. In a real application, you would populate the source buffer before copying. Wand handles the resources' states and required barriers for this operation.

## Built with Wand

**[Sorcery](https://github.com/leopph/sorcery)** is my hobby game engine that uses Wand as its graphics abstraction. Wand originally grew out of Sorcery's rendering code and was later separated into its own library.

Sorcery includes Wand as a Git submodule and references its Visual Studio project. Its rendering code provides a larger, real-world example of how Wand can be integrated into a renderer.

## Current limitations

- **D3D12 only:** there is no Vulkan or other backend yet.
- **D3D12-specific interface:** some public APIs still expose native D3D12 types and conventions.
- **Single-queue execution:** parallel GPU queues and asynchronous compute are not supported.
- **Incomplete feature coverage:** Wand is not intended to wrap every D3D12 feature, and the interface may change as the library develops.

## Roadmap

Areas I would like to explore include:

- A Vulkan backend and a more API-independent interface
- Indirect draw/dispatch support
- Work graphs and GPU queries
- Explicit resource-state control for cases where automatic tracking is insufficient
- Further improvements to the API and its abstractions
