//==--- glow/backend/vk/driver_allocator.hpp --------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
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

#ifndef RIPPLE_GLOW_BACKEND_VK_DRIVER_ALLOCATOR_HPP
#define RIPPLE_GLOW_BACKEND_VK_DRIVER_ALLOCATOR_HPP

#include "vulkan_command_buffer.hpp"
#include <ripple/core/memory/allocator.hpp>

namespace ripple::glow::backend {

//==--- [implementation] ---------------------------------------------------==//

/// This struct holds the allocators for the vulkan driver.
struct DriverAllocator {
  //==--- [traits] ---------------------------------------------------------==//

  /// Defines the arena sizes for the allocators.
  static constexpr size_t default_stack_arena_size_v = 2048;

  /// The type of the arena for the alocators.
  using ArenaType = StackArena<default_stack_arena_size_v>;

  /// The command buffer allocator needs to be thread safe, since the
  /// buffer might be allocated on one thread, but then recorded/submitted in
  /// another, at which point the handle w
  using CommandBufferAllocator =
    ObjectPoolAllocator<VulkanCommandBuffer, ArenaType>;

  //==--- [members] --------------------------------------------------------==//

  CommandBufferAllocator cmd_buffer_allocator; //!< Command buffer allocator.

  //==--- [construction] ---------------------------------------------------==//

  /// Default constructor which creates the allocators.
  DriverAllocator() : cmd_buffer_allocator(default_stack_arena_size_v) {}
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_DRIVER_ALLOCATOR_HPP
