//==--- glow/backend/vk/vulkan_command_pool.hpp ------------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_command_pool.hpp
/// \brief Header file for a Vulkan command pool.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_POOL_HPP
#define RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_POOL_HPP

#include "vulkan_headers.hpp"
#include <ripple/core/util/portability.hpp>
#include <vector>

namespace ripple::glow::backend {

/// Forward declaration of the vulkan driver.
class VulkanDriver;

/// This is a wrapper around a VkCommandPool which makes its use simpler.
class VulkanCommandPool {
 public:
  /// Defines the type of the stored driver.
  using driver_ptr = VulkanDriver*;
  /// Defines the type of the stored buffers.
  using buffers_t = std::vector<VkCommandBuffer>;

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to initialize the command pool with the \p driver which can be
  /// used to create the pool, and the index of the queue assosciated with the
  /// pool.
  /// \param driver             The driver which can be used to create the pool.
  /// \param queue_family_index The index of the queue family for the pool.
  VulkanCommandPool(driver_ptr driver, uint32_t queue_family_index);

  /// Destructor to clean up the command pool.
  ~VulkanCommandPool();

  /// Move constructor to move the \p other pool to this one.
  /// \param other The other pool to move.
  VulkanCommandPool(VulkanCommandPool&&) noexcept;

  /// Copy constructor -- deleted because copying of the pool is not allowed.
  VulkanCommandPool(const VulkanCommandPool&) = delete;

  //==--- [operator overloads] ---------------------------------------------==//

  /// Move assignment operator to move the \p other pool into this one.
  /// \param other The other pool to move.
  auto operator=(VulkanCommandPool&&) noexcept -> VulkanCommandPool&;

  /// Copy assignment operator -- deleted because moving is not allowed.
  auto operator=(const VulkanCommandPool&) -> VulkanCommandPool& = delete;

  //==--- [interface] ------------------------------------------------------==//

  /// Requests a command buffer from the pool.
  auto request_command_buffer() -> VkCommandBuffer;

  /// Requests a secondary command buffer from the pool.
  auto request_secondary_command_buffer() -> VkCommandBuffer;

  /// Resets the command pool.
  auto reset() -> void;

 private:
  buffers_t     _buffers;                          //!< Primary buffers.
  buffers_t     _secondary_buffers;                //!< Secondary buffers.
  driver_ptr    _driver          = nullptr;        //!< The driver.
  VkCommandPool _pool            = VK_NULL_HANDLE; //!< Command pool.
  unsigned      _index           = 0;              //!< Primary index.
  unsigned      _secondary_index = 0;              //!< Secondary index.
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_VULKAN_COMMAND_POOL_HPP