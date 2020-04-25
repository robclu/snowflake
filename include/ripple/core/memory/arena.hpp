//==--- ripple/core/memory/arena.hpp ----------------------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  arena.hpp
/// \brief This file defines memory arena implementations.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_CORE_MEMORY_ARENA_HPP
#define RIPPLE_CORE_MEMORY_ARENA_HPP

#include <cstdlib>
#include <type_traits>

namespace ripple {

/// Defines a stack-based memory arena of a specific size.
/// \tparam Size The size of the stack for the arena.
template <size_t Size>
class StackArena {
  /// Defines the size of the stack for the arena.
  static constexpr size_t stack_size_v = Size;

 public:
  //==--- [traits] ---------------------------------------------------------==//

  /// Returns that the allocator has a constexpr size.
  static constexpr bool contexpr_size_v = true;

  using Ptr      = void*;       //!< Pointer type.
  using ConstPtr = const void*; //!< Const pointer type.

  //==--- [constructor] ----------------------------------------------------==//

  /// Constructor which takes the size of the arena. This is provided to arenas
  /// have the same interface.
  StackArena(size_t size = 0) {}

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a pointer to the beginning of the arena.
  [[nodiscard]] auto begin() const -> ConstPtr {
    return static_cast<ConstPtr>(&_buffer[0]);
  }

  /// Returns a pointer to the end of the arena.
  [[nodiscard]] auto end() const -> ConstPtr {
    return static_cast<ConstPtr>(&_buffer[stack_size_v]);
  }

  /// Returns the size of the arena.
  [[nodiscard]] constexpr auto size() const -> size_t {
    return stack_size_v;
  }

 private:
  char _buffer[stack_size_v]; //!< The buffer for the stack.
};

/// Defines a heap-based arena.
struct HeapArena {
 public:
  //==--- [traits] ---------------------------------------------------------==//

  /// Returns that the allocator does not have a constexpr size.
  static constexpr bool constexpr_size_v = false;

  using Ptr = void*; //!< Pointer type.

  //==--- [construction] ---------------------------------------------------==//

  /// Initializes the arena with a specific size.
  /// \param size The size of the arena.
  HeapArena(size_t size) : _size(size) {
    _ptr = malloc(_size);
  }

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a pointer to the beginning of the arena.
  [[nodiscard]] auto begin() const -> Ptr {
    return _ptr;
  }

  /// Returns a pointer to the end of the arena.
  [[nodiscard]] auto end() const -> Ptr {
    return reinterpret_cast<Ptr>(uintptr_t(_ptr) + _size);
  }

  /// Returns the size of the arena.
  [[nodiscard]] constexpr auto size() const -> size_t {
    return _size;
  }

 private:
  size_t _size = 0;       //!< Size of the arena.
  void*  _ptr  = nullptr; //!< Pointer to the heap data.
};

//==--- [aliases] ----------------------------------------------------------==//

/// Defines the default size for a stack arena.
static constexpr size_t default_stack_arena_size_v = 4096;

/// Defines the type for a default stack arena.
using DefaultStackArena = StackArena<default_stack_arena_size_v>;

/// Defines a valid type if the Arena has a contexpr size.
/// \tparam Arena The arena to base the enable on.
template <typename Arena>
using arena_constexpr_size_enable_t =
  std::enable_if_t<std::decay_t<Arena>::contexpr_size_v, int>;

/// Defines a valid type if the Arena does not have a contexpr size.
/// \tparam Arena The arena to base the enable on.
template <typename Arena>
using arena_non_constexpr_size_enable_t =
  std::enable_if_t<!std::decay_t<Arena>::contexpr_size_v, int>;

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_ARENA_HPP
