//==--- glow/backend/vk/vulkan_driver.hpp ------------------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_driver.hpp
/// \brief Header file for a Vulkan driver.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_VK_VULKAN_DRIVER_HPP
#define RIPPLE_GLOW_BACKEND_VK_VULKAN_DRIVER_HPP

#include "vulkan_context.hpp"
#include "vulkan_surface_context.hpp"
#include "../platform/platform.hpp"

namespace ripple::glow::backend {

/// The VulkanDriver type is the main interface to Vulkan, and allows resource
/// to be accessed through it.
///
/// It owns a vulkan context, which itself has an instance and a device. The
/// engine is the interface through which resources can be allocated.
///
/// A Driver holds per frame resource contexts, where the context for each
/// frame references a swapchain which can be rendered to.
class VulkanDriver {
 public:
  //==--- [aliases] --------------------------------------------------------==//

  /// Defines the type of the platform for the driver.
  using platform_t = platform_type_t;

  //==--- [construction] ---------------------------------------------------==//

  /// Destructor to clean up the device resources.
  ~VulkanDriver();

  //==--- [interface] ------------------------------------------------------==//

  /// Creates the vulkan driver from the platform.
  /// \param platform The platform to create the driver with.
  static auto create(const platform_t& platform) -> VulkanDriver*;

 private:
  VulkanContext        _context;         //!< Vulkan context.
  VulkanSurfaceContext _surface_context; //!< Surface related context.

  VkQueue _graphics_queue = VK_NULL_HANDLE; //!< Graphics queue.
  VkQueue _compute_queue  = VK_NULL_HANDLE; //!< Compute queue.
  VkQueue _transfer_queue = VK_NULL_HANDLE; //!< Transfer queue.

  /// Constructor to initialize the device.
  /// \param platform The platform to create the driver for.
  VulkanDriver(const platform_t& platform);
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_VULKAN_DRIVER_HPP
