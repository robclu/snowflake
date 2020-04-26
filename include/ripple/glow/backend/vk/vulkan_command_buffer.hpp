//==--- ripple/glow/backend/vulkan_command_buffer.hpp ------ -*- C++ -*- ---==//
//
//                            Ripple - Glow
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

#ifndef RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP
#define RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP

#include "vulkan_context.hpp"
#include <ripple/core/memory/intrusive_pointer.hpp>

namespace ripple::glow {

//==--- [alias and forward declarations] -----------------------------------==//

namespace backend {

/// Forward definition of the driver allocator, which also destroys the the
/// allocated resources.
struct DriverAllocator;

/// Forward declaration for a vulkan command buffer.
class VulkanCommandBuffer;

/// Forward declaration of the Vulkan driver.
class VulkanDriver;

/// Deleter for vulkan command buffers.
struct CommandBufferDeleter {
  /// Overload of call operator to delete the \p buffer.
  auto operator()(VulkanCommandBuffer* buffer) const -> void;
};

} // namespace backend

/// Defines the type of the handle for a command buffer.
using CommandBufferHandle = IntrusivePtr<backend::VulkanCommandBuffer>;

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
: public IntrusivePtrEnabled<VulkanCommandBuffer, CommandBufferDeleter> {
  /// Allow the command buffer deleter to access the internals for destruction.
  friend CommandBufferDeleter;

 public:
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
    size_t            thread_index = 0)
  : _driver(driver),
    _cmd_buffer(buffer),
    _kind(kind),
    _thread_index(thread_index) {}

  /// Returns the thread index for the buffer, which is the index of the thread
  /// on which the buffer was created.
  auto thread_index() const -> size_t {
    return _thread_index;
  }

 private:
  VulkanDriver*     _driver;           //!< The driver which created the buffer.
  VkCommandBuffer   _cmd_buffer;       //!< The vulkan command buffer.
  CommandBufferKind _kind;             //!< The kind of the command buffer.
  size_t            _thread_index = 0; //!< Thread index for the buffer.
};

} // namespace backend

} // namespace ripple::glow

#endif // RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_BUFFER_HPP