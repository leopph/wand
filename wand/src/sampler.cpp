#include "wand/sampler.hpp"

#include "wand/wand.hpp"

namespace wand {
UniqueSamplerHandle::UniqueSamplerHandle(UINT const resource, GraphicsDevice& device) :
  resource_{resource},
  device_{&device} {
}


UniqueSamplerHandle::UniqueSamplerHandle(UniqueSamplerHandle&& other) noexcept :
  resource_{other.resource_},
  device_{other.device_} {
  other.resource_ = kInvalidResourceIndex;
  other.device_ = nullptr;
}


UniqueSamplerHandle::~UniqueSamplerHandle() {
  InternalDestruct();
}


auto UniqueSamplerHandle::operator=(UniqueSamplerHandle&& other) noexcept -> UniqueSamplerHandle& {
  if (this != &other) {
    InternalDestruct();
    resource_ = other.resource_;
    device_ = other.device_;
    other.resource_ = kInvalidResourceIndex;
    other.device_ = nullptr;
  }
  return *this;
}


auto UniqueSamplerHandle::Get() const -> UINT {
  return resource_;
}


auto UniqueSamplerHandle::IsValid() const -> bool {
  return resource_ != kInvalidResourceIndex;
}


auto UniqueSamplerHandle::InternalDestruct() const -> void {
  if (device_) {
    device_->DestroySampler(resource_);
  }
}
}
