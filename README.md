# 🪄 wand 🪄

## Introduction
Wand is Render Hardware Interface (RHI) that implements abstractions over low-level graphics APIs. Currently only D3D12 on Windows is supported.

## Features
Key features include:
- Automatic memory management
- Automatic resource state tracking
- Automatic barrier placement
- Automatic resource descriptor handling
- Easy-to-use bindless resource system
- Slightly less verbose than the underlying APIs

## Limitations
- Currently only D3D12 and Windows supported
- No parallel GPU queues
- Leaks the underlying APIs quite a lot
- Doesn't cover every modern (and less modern) feature of the covered APIs

## Why create wand?
Wand was originally created to make D3D12 usage easier in my hobby game engine, [Sorcery](https://github.com/leopph/sorcery). Initially it was only a thin layer over the standard D3D12 calls (it still kind of is), but over time it grew and now I use it in my other projects as well. I plan to keep it updated, refine its interface and add support for new features as I learn how to use them efficiently.

## Planned development
Some of the bigger things I've already planned:
- Less API leakage. More and better abstractions to completely hide the underlying graphics API. This might come with virtual calls, or some compile time shenanigans, I haven't decided yet.
- Support for ray tracing.
- Support for indirect draws.
- Support for work graphs.
- Support for queries.
- Support for manual state transitions (for special cases where automatic mechanisms might fail or be insufficient).
- Support for Vulkan.
