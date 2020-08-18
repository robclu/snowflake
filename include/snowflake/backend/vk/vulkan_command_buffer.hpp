//==--- snowflake/backend/vulkan_command_buffer.hpp -------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_command_buffer.hpp
/// \brief This file defines a command buffer interface.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP
#define SNOWFLAKE_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP

#include "vulkan_context.hpp"
#include <snowflake/util/portability.hpp>
#include <wrench/memory/intrusive_ptr.hpp>

namespace snowflake {

//==--- [alias and forward declarations] -----------------------------------==//

namespace backend {

// Forward declaration of the vulkan driver.
class VulkanDriver;

// Forward declaration of the vulkan command buffer.
class VulkanCommandBuffer;

/// Deleter for vulkan command buffers.
struct CommandBufferDeleter {
  /// Overload of call operator to delete the \p buffer.
  auto operator()(VulkanCommandBuffer* buffer) const noexcept -> void;
};

} // namespace backend

/// Defines the type of the handle for a command buffer.
using CommandBufferHandle = wrench::IntrusivePtr<backend::VulkanCommandBuffer>;

/// Defines the types of command buffers.
enum class CommandBufferKind : uint8_t {
  graphics = 0, //!< Graphics command buffer.
  compute  = 1, //!< Compute command buffer.
  transfer = 2  //!< Transfer command buffer
};

//==--- [implementation] ---------------------------------------------------==//

namespace backend {

/// The VulkanCommandBuffer is a wrapper around VkCommandBuffer, which provides
/// some utility over the raw implementation.
class VulkanCommandBuffer
: public wrench::
    IntrusivePtrEnabled<VulkanCommandBuffer, CommandBufferDeleter> {
  /// Allow the command buffer deleter to access the internals for destruction.
  friend CommandBufferDeleter;

  using Intrusive =
    IntrusivePtrEnabled<VulkanCommandBuffer, CommandBufferDeleter>;

 public:
  using Intrusive::Intrusive;

  //==--- [construction] ---------------------------------------------------==//

  /// Constructors to initialize the command buffer.
  /// \param driver       The Vulkan driver from which the buffer was created.
  /// \param buffer       The Vulkan command buffer being wrapped.
  /// \param kind         The kind of the command buffer.
  /// \param thread_index The index of the thread the buffer was created on.
  VulkanCommandBuffer(
    VulkanDriver*     driver,
    VkCommandBuffer   buffer,
    CommandBufferKind kind,
    size_t            thread_index = 0) noexcept
  : driver_(driver),
    cmd_buffer_(buffer),
    kind_(kind),
    thread_index_(thread_index) {}

  /// Returns the thread index for the buffer, which is the index of the thread
  /// on which the buffer was created.
  snowflake_nodiscard auto thread_index() const noexcept -> size_t {
    return thread_index_;
  }

 private:
  VulkanDriver*     driver_;           //!< The driver which created the buffer.
  VkCommandBuffer   cmd_buffer_;       //!< The vulkan command buffer.
  CommandBufferKind kind_;             //!< The kind of the command buffer.
  size_t            thread_index_ = 0; //!< Thread index for the buffer.
};

} // namespace backend
} // namespace snowflake

#endif // SNOWFLAKE_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP
