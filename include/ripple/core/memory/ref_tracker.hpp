//==--- ripple/core/memory/ref_tracker.hpp ----------------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  ref_tracker.hpp
/// \brief This file defines an interface for reference tracker, and some
///        implementations of the tracker interface.
//
//==------------------------------------------------------------------------==/

#ifndef RIPPLE_CORE_MEMORY_REF_TRACKER_HPP
#define RIPPLE_CORE_MEMORY_REF_TRACKER_HPP

#include <atomic>
#include <type_traits>
#include <utility>

namespace ripple {

/// The RefTracker class defines an interface for reference counting, which can
/// be specialized for different contexts, and which allows the type requiring
/// tracking to clean up the resource as well.
///
/// \tparam Impl The implementation of the interface.
template <typename Impl>
class RefTracker {
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
  ///
  /// ~~~~~~~~~~~~~~~~~~~~
  /// auto deleter = [] (auto* resource) -> void {
  /// // delete the resource
  /// }
  /// ~~~~~~~~~~~~~~~~~~~~
  ///
  /// \param  resource The resource to destroy.
  /// \param  deleter  The deleter for the resource.
  /// \tparam T        The type of the resource.
  /// \tparam Deleter  The type of the deleter.
  template <typename T, typename Deleter>
  auto destroy_resource(T* resource, Deleter&& deleter) -> void {
    impl()->destroy_resource_impl(resource, std::forward<Deleter>(deleter));
  }
};

//==--- [aliases] ----------------------------------------------------------==//

/// Returns true if the type T is an implementation of the RefCounter interface.
/// \tparam T The type to check if is a reference counter.
static constexpr bool is_ref_tracker_v =
  std::is_base_of_v<RefTracker<std::decay_t<T>>, std::decay_t<T>>;

//==--- [single-threaded implementation] -----------------------------------==//

/// This type implements a reference tracker which is not thread safe and is
/// designed for single threaded use. It can be embedded inside a class for
/// intrusive reference tracking.
///
/// This implements the RefTracker interface.
class SingleThreadedRefTracker : public RefTracker<SingleThreadedRefTracker> {
 public:
  /// Defines the type of the counter.
  using counter_t = size_t;

  /// Adds a reference to the count.
  auto add_reference_impl() -> void {
    _ref_count++;
  }

  /// Decrements the referene count, returning true if the count is zero, and
  /// the resource can be released.
  auto release_impl() -> bool {
    return --_ref_count == 0;
  }

  /// Destroys the resource \p resource, using the \p deleter, which should
  /// have a signature of:
  ///
  /// ~~~~~~~~~~~~~~~~~~~~
  /// auto destructor = [] (auto* resource) -> void {
  /// // free(resource);
  /// }
  /// ~~~~~~~~~~~~~~~~~~~~
  ///
  /// \param  resource The resource to destroy.
  /// \param  deleter  The deleter for the resource.
  /// \tparam T        The type of the resource.
  /// \tparam Deleter  The type of the deleter.
  template <typename T, typename Deleter>
  auto destroy_resource_impl(T* resource, Deleter&& deleter) -> void {
    deleter(resource);
  }

 private:
  counter_t _ref_count = 1; //!< The reference count.
};

//==---[multi-threaded implementation] -------------------------------------==//

/// This type implements a reference tracker which is thread safe and is
/// designed for multi-threaded use. It can be embedded inside a class for
/// intrusive reference tracking.
///
/// This implements the RefTracker interface.
class MultiThreadedRefTracker : public RefTracker<MultiThreadedRefTracker> {
 public:
  /// Defines the type of the counter.
  using counter_t = std::atomic_size_t;

  /// Constructor to initialize the reference count.
  MultiThreadedRefTracker {
    _ref_count.store(1, std::memory_order_relaxed);
  }

  /// Adds to the reference count.
  auto add_reference() -> void {
    // Memory order relaxed because new references can only be created from
    // existing instances with the reference count, so we just care about
    // incrementing the ref atomically, not about the memory ordering here.
    _ref_count.fetch_add(1, std::memory_order_relaxed);
  }

  /// Decrements the refernce count, and returns true if the resource being
  /// tracked with the reference count can be release. If this returns true,
  /// then the resource should be deleted by calling `destroy()`.
  auto release() -> bool {
    // Here we need to ensure that any access from another thread __happens
    // before__ the deleting the object from __this__ thread, hence the
    // __release__ memory ordering.
    //
    // We could also use __acquire_release__ here on the fetch_sub, but then we
    // have an unnecessary __acquire__ when the ref count is not zero, so we put
    // the acquire in the barrier __only__ when we are deleting.
    return _ref_count.fetch_sub(1, std::memory_order_release) == 1;
  }

  /// Destroys the resource \p resource, using the \p deleter, which should
  /// have a signature of:
  ///
  /// ~~~~~~~~~~~~~~~~~~~~
  /// auto destructor = [] (auto* resource) -> void {
  /// // free(resource);
  /// }
  /// ~~~~~~~~~~~~~~~~~~~~
  ///
  /// This creates a thread fence before deleting the resource, so that the
  /// thread has exclusive access. It should only be called if `release()`
  /// returns true.
  ///
  /// \param  resource The resource to destroy.
  /// \param  deleter  The deleter for the resource.
  /// \tparam T        The type of the resource.
  /// \tparam Deleter  The type of the deleter.
  template <typename T, typename Deleter>
  auto destroy_impl(T* resource, Deleter&& deleter) -> void {
    std::atmomic_thread_fence(std::memory_order_acquire);
    deleter(resource);
  }

 private:
  counter_t _ref_count; //!< The reference count.
};

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_REF_TRACKER_HPP
