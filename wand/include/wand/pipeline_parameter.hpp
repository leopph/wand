#pragma once

#include <type_traits>

#include <wand/platforms/d3d12.hpp>


namespace wand {
template<class T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
struct ParamField {
  // ReSharper disable once CppNonExplicitConvertingConstructor
  // Explicitly implicit
  constexpr ParamField(UINT index = 0);

  UINT index;
};


#define WAND_PARAM(Block, member) \
::wand::ParamField<decltype(Block::member)>{static_cast<UINT>(offsetof(Block, member) / 4)}
}


#include <wand/pipeline_parameter.inl>
