//==--- ripple/core/memory/allocator.hpp ------------------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  allocator.hpp
/// \brief This file defines a composable allocator.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_CORE_MEMORY_ALLOCATOR_HPP
#define RIPPLE_CORE_MEMORY_ALLOCATOR_HPP

#include "aligned_heap_allocator.hpp"
#include "pool_allocator.hpp"
#include <mutex>

namespace ripple {

/// Default locking implementation which does no locking.
struct VoidLock {
  /// Does nothing when `lock()` is called.
  auto lock() noexcept -> void {}
  /// Does nothing when `unlock()` is called.
  auto unlock() noexcept -> void {}
};

//==--- [forward declarations & aliases] -----------------------------------==//

/// The Allocator type is a simple implementation which allows an allocator to
/// be composed of other allocators, to create allocators with useful properties
/// for different contexts.
///
/// The allocator will always try to allocate from the primary allocator, unless
/// the primary allocation fails, in which case it will allocate from the
/// fallback allocator.
///
/// All allocation and free operations are locked, using the locking
/// policy provided. The default locking policy is to not lock.
///
/// \tparam PrimaryAllocator  The type of the primary allocator.
/// \tparam FallbackAllocator The type of the fallback allocator.
/// \tparam LockingPolicy     The type of the locking policy.
template <
  typename PrimaryAllocator,
  typename FallbackAllocator = AlignedHeapAllocator,
  typename LockingPolicy     = VoidLock>
class Allocator;

// clang-format off
/// Defines an object pool allocator for objects of type T, which is by default
/// not thread safe.
/// \tparam T The type of the objects to allocate from the pool.
template <
  typename T,
  bool     ThreadSafe   = false,
  typename FreelistType =
    std::conditional_t<ThreadSafe, ThreadSafeFreelist, Freelist>>
using ObjectPoolAllocator = Allocator<
  PoolAllocator<
    sizeof(T),
    std::max(alignof(T), alignof(FreelistType)),
    FreelistType
  >
>;
// clang-format on

/// Defines an object pool allocator for objects of type T, which is
/// thread-safe.
/// \tparam T The type of the objects to allocate from the pool.
template <typename T>
using ThreadSafeObjectPoolAllocator = ObjectPoolAllocator<T, true>;

//==--- [implementation] ---------------------------------------------------==//

/// The Allocator type is a simple implementation which allows an allocator to
/// be composed of other allocators, to create allocators with useful
/// properties for different contexts.
///
/// The allocator will always try to allocate from the primary allocator,
/// unless the primary allocation fails, in which case it will allocate from
/// the fallback allocator.
///
/// All allocation and free operations are locked, using the locking
/// policy provided. The default locking policy is to not lock.
///
/// \tparam PrimaryAllocator  The type of the primary allocator.
/// \tparam FallbackAllocator The type of the fallback allocator.
/// \tparam LockingPolicy     The type of the locking policy.
template <
  typename PrimaryAllocator,
  typename FallbackAllocator,
  typename LockingPolicy>
class Allocator {
 public:
  //==--- [aliases] --------------------------------------------------------==//

  // clang-format off
  /// Defines the type of the primary allocator.
  using primary_allocator_t  = PrimaryAllocator;
  /// Defines the type of the fallback allocator.
  using fallback_allocator_t = FallbackAllocator;
  /// Defines the type of the locking implementation.
  using locking_policy_t     = LockingPolicy;
  /// Defines the type of the lock guard.
  using guard_t              = std::lock_guard<locking_policy_t>;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor, which forwards the \p arena and the \p args to the primary
  /// allocator for construction.,
  /// \param arena  The arena for the primary allocation.
  /// \param args   The arguments fro the primary allocator.
  /// \tparam Arena The type of the arena.
  /// \tparam Args  The types of arguments for the primary allocator.
  template <typename Arena, typename... Args>
  Allocator(const Arena& arena, Args&&... args)
  : _primary(arena, std::forward<Args>(args)...) {}

  //==--- [alloc/free interface] -------------------------------------------==//

  /// Allocates \p size bytes of memory with \p align alignment.
  /// \param size      The size of the memory to allocate.
  /// \param alignment The alignment of the allocation.
  auto alloc(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept
    -> void* {
    guard_t g(_lock);
    void*   ptr = _primary.alloc(size, alignment);
    if (ptr == nullptr) {
      ptr = _fallback.alloc(size, alignment);
    }
    return ptr;
  }

  /// Frees the memory pointed to by ptr.
  /// \param ptr The pointer to the memory to free.
  auto free(void* ptr) noexcept -> void {
    if (ptr == nullptr) {
      return;
    }

    guard_t g(_lock);
    if (_primary.owns(ptr)) {
      _primary.free(ptr);
      return;
    }
    _fallback.free(ptr);
  }

  /// Frees the memory pointed to by \ptr, with a size of \p size.
  /// \param ptr  The pointer to the memory to free.
  /// \param size The size of the memory to free.
  auto free(void* ptr, size_t size) noexcept -> void {
    if (ptr == nullptr) {
      return;
    }

    guard_t g(_lock);
    if (_primary.owns(ptr)) {
      _primary.free(ptr, size);
      return;
    }
    _fallback.free(ptr, size);
  }

  /// Resets the primary and fallback allocators.
  auto reset() noexcept -> void {
    guard_t g(_lock);
    _primary.reset();
    _fallback.reset();
  }

  //==--- [create/destroy interface] ---------------------------------------==//

  /// Allocates and constructs an object of type T. If this is used, then
  /// destroy should be used to destruct and free the object, rather than free.
  /// \tparam args The arguments for constructing the objects.
  /// \tparam T    The type of the object to allocate.
  /// \tparam Args The types of the arguments for constructing T.
  template <typename T, typename... Args>
  auto create(Args&&... args) noexcept -> T* {
    constexpr size_t size      = sizeof(T);
    constexpr size_t alignment = alignof(T);
    void* const      ptr       = alloc(size, alignment);

    // We know that the allocation never fails, because in the worst case the
    // fallback allocator must allocate the object, even if it's slow.
    new (ptr) T(std::forward<Args>(args)...);
    return ptr;
  }

  /// Destroys the object pointed to by \p ptr.
  /// \param  ptr A pointer to the object to destroy.
  /// \tparam T   The type of the object.
  template <typenmame T>
  auto destroy(T* ptr) noexcept -> void {
    if (ptr == nullptr) {
      return;
    }

    constexpr size_t size = sizeof(T);
    ptr->~T();
    free(static_cast<void*>(ptr), size);
  }

 private:
  primary_allocator_t  _primary;  //!< The primary allocator.
  fallback_allocator_t _fallback; //!< The fallback allocator.
  locking_impl_t       _lock;     //!< The locking implementation.
};

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_ALLOCATOR_HPP
