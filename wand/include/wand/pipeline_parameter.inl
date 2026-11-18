#pragma once

namespace wand {
template<class T> requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
constexpr ParamField<T>::ParamField(UINT const index) :
  index{index} {}
}
