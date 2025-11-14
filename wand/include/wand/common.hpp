#pragma once

#include <string_view>

#include <wand/platforms/d3d12.hpp>

// Returns the index of the member if all the pipeline parameters are considered a single buffer of the specified type.
#define PIPELINE_PARAM_INDEX(BufferType, MemberName) static_cast<UINT>(offsetof(BufferType, MemberName) / 4)

namespace wand {
auto constexpr kInvalidResourceIndex{static_cast<UINT>(-1)};

auto ThrowIfFailed(HRESULT hr, std::string_view msg) -> void;
}
