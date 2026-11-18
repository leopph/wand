#pragma once

#include <string_view>

#include <wand/platforms/d3d12.hpp>


namespace wand {
auto constexpr kInvalidResourceIndex{static_cast<UINT>(-1)};

auto ThrowIfFailed(HRESULT hr, std::string_view msg) -> void;
}
