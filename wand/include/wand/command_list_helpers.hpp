#pragma once

#include <string_view>

#include <wand/wandapi.hpp>


namespace wand {
class CommandList;


class [[nodiscard]] ScopedEvent {
public:
  WANDAPI ScopedEvent(CommandList& cmd, std::wstring_view name);
  ScopedEvent(ScopedEvent const& other) = delete;
  ScopedEvent(ScopedEvent&& other) noexcept = delete;

  WANDAPI
  ~ScopedEvent();

  auto operator=(ScopedEvent const& other) -> ScopedEvent& = delete;
  auto operator=(ScopedEvent&& other) noexcept -> ScopedEvent& = delete;

private:
  CommandList* cmd_;
};
}
