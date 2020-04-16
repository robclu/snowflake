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

#include "frame_data.hpp"
#include "vulkan_context.hpp"
#include "vulkan_surface_context.hpp"
#include "../platform/platform.hpp"

/// Defines the number of frame contexts used by the driver.
static constexpr size_t num_frame_contexts_cx =
#if defined(GLOW_FRAME_CONTEXTS)
  GLOW_FRAME_CONTEXTS;
#else
  3;
#endif

namespace ripple::glow::backend {

//==--- [driver queues] ----------------------------------------------------==//

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

  // clang-format off
  /// Defines the type of the platform for the driver.
  using platform_t        = platform_type_t;
  /// Defines the type of the per frame data container.
  using frame_container_t = std::vector<FrameData>;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Creates the vulkan driver from the platform.
  /// \param platform The platform to create the driver with.
  /// \param threads  The number of threads to use for the driver.
  static auto
  create(const platform_t& platform, uint16_t threads = 1) -> VulkanDriver*;

  /// Destructor to clean up the device resources.
  ~VulkanDriver();

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a const reference to the context.
  auto context() const -> const VulkanContext& {
    return _context;
  }

  /// Returns the number of threads being used by the driver.
  auto num_threads() const -> uint16_t {
    return _num_threads;
  }

  //==--- [frame interface] ------------------------------------------------==//

  /// Begins the frame for the driver. This does the following:
  /// - Gets the next image if it doesn't have one already.
  /// - Updates the driver frame context to the next frame.
  /// - Performes synchronization of the swapchain image
  /// - Polls the platform input for optimal latency.
  ///
  /// If there are any swapchain related errors when trying to acquire the
  /// swapchain image, then the swapchain is recreated.
  ///
  /// This returns true if the image was acquired successfully, otherwise it
  /// returns false, and rendering isn't possible.
  ///
  ///
  /// \param platform The platform for the swapchain.
  auto begin_frame(platform_t& platform) -> bool;

  /// Returns a reference to the current frame.
  auto current_frame() -> FrameData& {
    return _frames[_frame_index];
  }

 private:
  VulkanContext        _context;         //!< Vulkan context.
  VulkanSurfaceContext _surface_context; //!< Surface related context.
  frame_container_t    _frames;          //!< The frames for the engine.
  uint16_t             _num_threads = 1; //!< The number of threads being used.
  uint8_t              _frame_index = 0; //!< Index of the frame.

  // If the swapchain has been acquired.
  bool _acquired_swapchain = false;

  /// Constructor to initialize the device.
  /// \param platform The platform to create the driver for.
  /// \param threads  The number of threads for the driver.
  VulkanDriver(const platform_t& platform, uint16_t threads);

  /// Tries to acquire the next swapchain image, returning true on success, and
  /// false on failure.
  /// \param platform The platform to use to recreate the swapchain if
  ///                 necessary.
  auto acquire_next_image(platform_t& platform) -> bool;

  /// Advances the frame data for the driver to the next frame.
  auto advance_frame_data() -> void;

  /// Creates the frame data.
  auto create_frame_data() -> void;
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_VULKAN_DRIVER_HPP
