#pragma once

#include <memory>
#include <type_traits>


namespace wand {
template<class T> auto CommandList::SetPipelineParameter(
  ParamField<T> const field,
  std::type_identity_t<T> const& value
) const -> void {
  SetPipelineParameters(field.index, sizeof(value) / 4, std::addressof(value));
}
}
