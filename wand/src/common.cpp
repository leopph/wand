#include "wand/common.hpp"

#include <stdexcept>

namespace wand {
auto ThrowIfFailed(HRESULT const hr, std::string_view const msg) -> void {
  if (FAILED(hr)) {
    throw std::runtime_error{msg.data()};
  }
}
}
