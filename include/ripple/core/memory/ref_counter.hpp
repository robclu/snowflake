//==--- ripple/core/memory/ref_counter.hpp ----------------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  ref_counter.hpp
/// \brief This file defines an interface for reference counters.
//
//==------------------------------------------------------------------------==/

#ifndef RIPPLE_CORE_MEMORY_REF_COUNTER_HPP
#define RIPPLE_CORE_MEMORY_REF_COUNTER_HPP

#include <utility>

namespace ripple {

/// The RefCounter class defines an interface for reference counting, which can
/// be specialized for different contexts.
/// \tparam Impl The implementation of the interface.
template <typename Impl>
class RefCounter {
  /// Defines the type of the implementation.
  using impl_t = Impl;

  /// Returns a pointer to the implementation.
  auto impl() -> impl_t* {
    return static_cast<impl_t*>(this);
  }

  /// Returns a const pointer to the implementation.
  auto impl() const -> const impl_t* {
    return static_cast<const impl_t*>(this);
  }

 public:
  /// Adds a reference to the count.
  auto add_reference() -> void {
    impl()->add_reference_impl();
  }

  /// Decrements the referene count, returning true if the count is zero, and
  /// the resource can be released. If this returns true, then the resource
  /// should be destroyed through a call to `destroy()`.
  auto release() -> bool {
    return impl()->release_impl();
  }

  /// Destroys the resource \p resource, using the \p deleter, which should
  /// have a signature of:
  /// ~~~~~~~~~~~~~~~~~~~~
  /// auto deleter = [] (auto* resource) -> void {
  /// // delete the resource
  /// }
  /// ~~~~~~~~~~~~~~~~~~~~
  /// \param  resource The resource to destroy.
  /// \param  deleter  The deleter for the resource.
  /// \tparam T        The type of the resource.
  /// \tparam Deleter  The type of the deleter.
  template <typename T, typename Deleter>
  auto destroy_resource(T* resource, Deleter&& deleter) -> void {
    impl()->destroy_resource_impl(resource, std::forward<Deleter>(deleter));
  }
};

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_REF_COUNTER_HPP
