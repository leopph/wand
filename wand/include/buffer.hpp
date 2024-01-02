#pragma once

namespace wand {
  class Buffer {
  protected:
    Buffer() = default;

  public:
    Buffer(Buffer const& other) = delete;
    Buffer(Buffer&& other) = delete;

    virtual ~Buffer() = default;

    auto operator=(Buffer const& other) -> void = delete;
    auto operator=(Buffer&& other) -> void = delete;
  };
}