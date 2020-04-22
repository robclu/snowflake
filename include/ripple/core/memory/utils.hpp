//==--- ripple/core/memory/utils.hpp ----------------------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  utils.hpp
/// \brief This file defines a utility functions for memory functionality.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_CORE_MEMORY_UTILS_HPP
#define RIPPLE_CORE_MEMORY_UTILS_HPP

#include <assert>
#include <cstdlib>

namespace ripple::memory {

/// Returns a new ptr offset by \p amount from \p ptr.
///
/// \note This does __not_ ensure alignemt. If the pointer needs to be aligned,
///       then pass the result to `align()`.
///
/// \param ptr    The pointer to offset.
/// \param amount The amount to offset ptr by.
static inline auto offset(void* ptr, uint32_t amount) noexcept -> void* {
  return reinterpret_cast<void*>(uintptr_t(ptr) + amount);
}

/// Returns a pointer with an address aligned to \p alignment. This will fail at
/// runtime if the \p alignemnt is not a power of two.
/// \param ptr       The pointer to align.
/// \param alignment The alignment to ensure.
static inline auto align(void* ptr, size_t alignment) noexcept -> void* {
  assert(
    !(alignment & (alignment - 1)) &&
    "Alignment must be a power of two for linear allocation!");
  return reinterpret_cast<void*>(
    (uintptr_t(ptr) + alignment - 1) & ~(alignment - 1));
}

} // namespace ripple::memory

#endif // RIPPLE_CORE_MEMORY_UTILS_HPP
