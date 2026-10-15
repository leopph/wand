#include "wand/command_list_helpers.hpp"

#include "wand/command_list.hpp"


namespace wand {
ScopedEvent::ScopedEvent(CommandList& cmd, std::wstring_view const name) :
  cmd_{&cmd} {
  cmd_->BeginEvent(name);
}


ScopedEvent::~ScopedEvent() {
  cmd_->EndEvent();
}
}
