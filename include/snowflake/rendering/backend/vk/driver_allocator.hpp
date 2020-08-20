//==--- snowflake/rendering/backend/vk/driver_allocator.hpp  -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  driver_allocator.hpp
/// \brief Header file for vulkan driver allocator.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERING_BACKEND_VK_DRIVER_ALLOCATOR_HPP
#define SNOWFLAKE_RENDERING_BACKEND_VK_DRIVER_ALLOCATOR_HPP

#include "vulkan_command_buffer.hpp"
#include <wrench/memory/allocator.hpp>

namespace snowflake::backend {

/// This struct holds the allocators for the vulkan driver.
struct DriverAllocator {
  //==--- [traits] ---------------------------------------------------------==//

  /// Defines the size of the arenas for the allocators.
  static constexpr size_t arena_size = 2048;

  /// The type of the arena for the alocators.
  using Arena = wrench::HeapArena;
  /// The type of the lock for the allocator.
  using Lock = wrench::VoidLock;

  /// The command buffer allocator needs to be thread safe, since the
  /// buffer might be allocated on one thread, but then recorded/submitted in
  /// another, at which point the handle w
  using CommandBufferAllocator =
    wrench::ObjectPoolAllocator<VulkanCommandBuffer, Lock, Arena>;

  //==--- [members] --------------------------------------------------------==//

  CommandBufferAllocator cmd_buffer_allocator; //!< Command buffer allocator.

  //==--- [construction] ---------------------------------------------------==//

  /// Default constructor which creates the allocators using the default arena
  /// sizes.
  DriverAllocator() noexcept : cmd_buffer_allocator(arena_size) {}

  /// Constructor which creates the allocator using the \p size for the arenas
  /// for the allocator.
  /// \param size The size of the arenas for the allocators.
  DriverAllocator(size_t size) noexcept : cmd_buffer_allocator(size) {}

  //==--- [deleted] --------------------------------------------------------==//

  // clang-format off
  /// Copy constructor -- deleted.
  DriverAllocator(const DriverAllocator&) = delete;
  /// Move constructor -- deleted.
  DriverAllocator(DriverAllocator&&)      = delete;
  /// Copy assignment -- deleted.
  auto operator=(const DriverAllocator&)  = delete;
  /// Move assignment -- deleted.
  auto operator=(DriverAllocator&&)       = delete;
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_VK_DRIVER_ALLOCATOR_HPP
