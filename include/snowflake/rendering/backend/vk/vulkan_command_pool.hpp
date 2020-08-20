//==--- ../rendering/backend/vk/vulkan_command_pool.hpp ---- -*- C++ -*- ---==//
//
//                                Snowflake
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

#ifndef SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_COMMAND_POOL_HPP
#define SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_COMMAND_POOL_HPP

#include "vulkan_headers.hpp"
#include <ripple/core/util/portability.hpp>
#include <vector>

namespace snowflake::backend {

/// Forward declaration of the vulkan driver.
class VulkanDriver;

/// This is a wrapper around a VkCommandPool which makes its use simpler.
class VulkanCommandPool {
 public:
  // clang-format off
  /// Defines the type of the stored driver.
  using DriverPtr       = VulkanDriver*;
  /// Defines the type of the stored buffers.
  using BufferContainer = std::vector<VkCommandBuffer>;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to initialize the command pool with the \p driver which can be
  /// used to create the pool, and the index of the queue assosciated with the
  /// pool.
  /// \param driver             The driver which can be used to create the pool.
  /// \param queue_family_index The index of the queue family for the pool.
  VulkanCommandPool(DriverPtr driver, uint32_t queue_family_index) noexcept;

  /// Destructor to clean up the command pool.
  ~VulkanCommandPool() noexcept;

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
  auto operator=(const VulkanCommandPool&) = delete;

  //==--- [interface] ------------------------------------------------------==//

  /// Destroys the command pool.
  auto destroy() noexcept -> void;

  /// Requests a command buffer from the pool.
  auto request_command_buffer() noexcept -> VkCommandBuffer;

  /// Requests a secondary command buffer from the pool.
  auto request_secondary_command_buffer() noexcept -> VkCommandBuffer;

  /// Resets the command pool.
  auto reset() noexcept -> void;

 private:
  BufferContainer buffers_;                          //!< Primary buffers.
  BufferContainer secondary_buffers_;                //!< Secondary buffers.
  DriverPtr       driver_          = nullptr;        //!< The driver.
  VkCommandPool   pool_            = VK_NULL_HANDLE; //!< Command pool.
  unsigned        index_           = 0;              //!< Primary index.
  unsigned        secondary_index_ = 0;              //!< Secondary index.
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_COMMAND_POOL_HPP
