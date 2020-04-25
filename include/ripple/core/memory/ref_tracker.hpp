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

//==--- [forward declarations & aliases] -----------------------------------==//

/// Forward declaration of reference tracking interface.
/// \tparam Impl The implementation of the interface.
template <typename Impl>
class RefTracker;

/// Forward declaration of a single-threaded reference tracker.
class SingleThreadedRefTracker;

/// Forward declaration of a multi-threaded reference tracker.
class MultiThreadedRefTracker;

/// Defines the type of the default reference tracker. The tracker is multi
/// threaded unless ripple is explicitly compiler for single threaded use.
using DefaultRefTracker =
#if defined(RIPPLE_SINGLE_THREADED)
  SingleThreadedRefTracker;
#else
  MultiThreadedRefTracker;
#endif

/// Returns true if the type T is an implementation of the RefCounter
/// interface.
/// \tparam T The type to check if is a reference counter.
template <typename T>
static constexpr bool is_ref_tracker_v =
  std::is_base_of_v<RefTracker<std::decay_t<T>>, std::decay_t<T>>;

//==--- [implementation] ---------------------------------------------------==//

/// The RefTracker class defines an interface for reference counting, which can
/// be specialized for different contexts, and which allows the type requiring
/// tracking to clean up the resource as well.
///
/// \tparam Impl The implementation of the interface.
template <typename Impl>
class RefTracker {
  /// Returns a pointer to the implementation.
  auto impl() -> Impl* {
    return static_cast<Impl*>(this);
  }

  /// Returns a const pointer to the implementation.
  auto impl() const -> const Impl* {
    return static_cast<const Impl*>(this);
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

//==--- [single-threaded implementation] -----------------------------------==//

/// This type implements a reference tracker which is not thread safe and is
/// designed for single threaded use. It can be embedded inside a class for
/// intrusive reference tracking.
///
/// This implements the RefTracker interface.
class SingleThreadedRefTracker : public RefTracker<SingleThreadedRefTracker> {
 public:
  /// Defines the type of the counter.
  using Counter = size_t;

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
  Counter _ref_count = 1; //!< The reference count.
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
  using Counter = std::atomic_size_t;

  /// Constructor to initialize the reference count.
  MultiThreadedRefTracker() {
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
    // before__ the deleting the object, though a call to `destroy` __if__ this
    // returns true.
    //
    // To ensure this, no reads/or write can be reordered to be after the
    // `fetch_sub` (i.e they happen before). Another thread might hold the last
    // reference, and before deleting, the `fetch_sub` needs to happen on
    // __this__ thread __before__ that thread deletes, which is done with
    // `memory_order_release`.
    //
    // Note the delete needs a `memory_order_acquire` before it, to prevent the
    // opposite case. We could use `memory_order_acq_release` here, but that
    // wastes as aquire for each decrement, when it's only required before
    // deleting. Hence, we require a call to `destroy` to correctly destroy the
    // object.
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
  auto destroy_resource_impl(T* resource, Deleter&& deleter) -> void {
    // Here we need to ensure that no read or write is ordered before the
    // `fetch_sub` in the `release` call. Otherwise another thread might could
    // see a destroyed object before the reference count is zero. This is done
    // with the barrier with `memory_order_acquire`.
    std::atomic_thread_fence(std::memory_order_acquire);
    deleter(resource);
  }

 private:
  Counter _ref_count; //!< The reference count.
};

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_REF_TRACKER_HPP
