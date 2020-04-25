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
#include "arena.hpp"
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
/// \tparam Arena             The type of the arena for the allocator.
/// \tparam FallbackAllocator The type of the fallback allocator.
/// \tparam LockingPolicy     The type of the locking policy.
template <
  typename PrimaryAllocator,
  typename Arena             = DefaultStackArena,
  typename FallbackAllocator = AlignedHeapAllocator,
  typename LockingPolicy     = VoidLock>
class Allocator;

// clang-format off
/// Defines an object pool allocator for objects of type T, which is by default
/// not thread safe.
/// \tparam T The type of the objects to allocate from the pool.
template <
  typename T,
  typename Arena        = DefaultStackArena,
  bool     ThreadSafe   = false,
  typename FreelistType =
    std::conditional_t<ThreadSafe, ThreadSafeFreelist, Freelist>>
using ObjectPoolAllocator = Allocator<
  PoolAllocator<
    sizeof(T),
    std::max(alignof(T), alignof(FreelistType)),
    FreelistType
  >,
  Arena
>;
// clang-format on

/// Defines an object pool allocator for objects of type T, which is
/// thread-safe.
/// \tparam T The type of the objects to allocate from the pool.
template <typename T, typename Arena = DefaultStackArena>
using ThreadSafeObjectPoolAllocator = ObjectPoolAllocator<T, Arena, true>;

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
/// \tparam Arena             The type of the arena for the allocator.
/// \tparam FallbackAllocator The type of the fallback allocator.
/// \tparam LockingPolicy     The type of the locking policy.
template <
  typename PrimaryAllocator,
  typename Arena,
  typename FallbackAllocator,
  typename LockingPolicy>
class Allocator {
  static_assert(
    std::is_trivially_constructible_v<FallbackAllocator>,
    "Fallback allocator must be trivially constructible!");

 public:
  //==--- [constants] ------------------------------------------------------==//

  /// Returns true if the arena has a contexpr size.
  static constexpr bool contexpr_arena_size_v = Arena::contexpr_size_v;

  //==--- [aliases] --------------------------------------------------------==//

  /// Defines the type of the lock guard.
  using Guard = std::lock_guard<LockingPolicy>;

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor which sets the \p size of the arena, if the arena requires a
  /// size, and forwards the \p args to the primary allocator.
  ///
  /// If the arena has a constant size, then it will be created with that size,
  /// and \p size will be ignored.
  ///
  /// \param  size  The size of the arena.
  /// \param  args  The arguments fro the primary allocator.
  /// \tparam Args  The types of arguments for the primary allocator.
  template <typename... Args>
  Allocator(size_t size, Args&&... args)
  : _arena(size), _primary(_arena, std::forward<Args>(args)...) {}

  // clang-format off
  /// Move constructor, defaulted.
  /// \param other The other allocator to move into this one.
  Allocator(Allocator&& other) noexcept                    = default;
  /// Move assignment, defaulted.
  /// \param other The other allocator to move into this one.
  auto operator=(Allocator&& other) noexcept -> Allocator& = default;

  //==--- [deleted] --------------------------------------------------------==//

  /// Copy constructor -- deleted, allocator can't be copied.
  Allocator(const Allocator&)      = delete;
  /// Copy assignment -- deleted, allocator can't be copied.
  auto operator=(const Allocator&) = delete;
  // clang-format on

  //==--- [alloc/free interface] -------------------------------------------==//

  /// Allocates \p size bytes of memory with \p align alignment.
  /// \param size      The size of the memory to allocate.
  /// \param alignment The alignment of the allocation.
  auto alloc(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept
    -> void* {
    Guard g(_lock);
    void* ptr = _primary.alloc(size, alignment);
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

    Guard g(_lock);
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

    Guard g(_lock);
    if (_primary.owns(ptr)) {
      _primary.free(ptr, size);
      return;
    }
    _fallback.free(ptr, size);
  }

  /// Resets the primary and fallback allocators.
  auto reset() noexcept -> void {
    Guard g(_lock);
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
    T*               result;
    constexpr size_t size      = sizeof(T);
    constexpr size_t alignment = alignof(T);
    void* const      ptr       = alloc(size, alignment);

    // We know that the allocation never fails, because in the worst case the
    // fallback allocator must allocate the object, even if it's slow.
    return new (ptr) T(std::forward<Args>(args)...);
  }

  /// Destroys the object pointed to by \p ptr.
  /// \param  ptr A pointer to the object to destroy.
  /// \tparam T   The type of the object.
  template <typename T>
  auto destroy(T* ptr) noexcept -> void {
    if (ptr == nullptr) {
      return;
    }

    constexpr size_t size = sizeof(T);
    ptr->~T();
    free(static_cast<void*>(ptr), size);
  }

 private:
  Arena             _arena;    //!< The type of the arena.
  PrimaryAllocator  _primary;  //!< The primary allocator.
  FallbackAllocator _fallback; //!< The fallback allocator.
  LockingPolicy     _lock;     //!< The locking implementation.
};

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_ALLOCATOR_HPP
