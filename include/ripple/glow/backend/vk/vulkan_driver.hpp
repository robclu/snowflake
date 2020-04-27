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

#include "driver_allocator.hpp"
#include "frame_data.hpp"
#include "vulkan_context.hpp"
#include "vulkan_surface_context.hpp"
#include "../platform/platform.hpp"
#include <array>

/// Defines the number of frame contexts used by the driver.
static constexpr size_t num_frame_contexts_v =
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
  using Platform          = PlatformType;
  /// Defines the type of the per frame data container.
  using FrameContainer    = std::vector<FrameData>;
  /// Defines the type of the command buffer counter.
  using CmdBufferCounter  = std::atomic_uint32_t;
  /// Defines the type of the submission counter.
  using CmdBufferCounters = std::array<CmdBufferCounter, num_frame_contexts_v>;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Creates the vulkan driver from the platform.
  /// \param platform The platform to create the driver with.
  /// \param threads  The number of threads to use for the driver.
  static auto
  create(const Platform& platform, uint16_t threads = 1) -> VulkanDriver*;

  /// Destroys the driver.
  auto destroy() -> void;

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a const reference to the context.
  auto context() const -> const VulkanContext& {
    return _context;
  }

  /// Returns the number of threads being used by the driver.
  auto num_threads() const -> uint16_t {
    return _num_threads;
  }

  /// Sets the presentation mode for the driver.
  auto set_present_mode(PresentMode present_mode) -> void {
    _present_mode = present_mode;
  }

  //==--- [frame interface] ------------------------------------------------==//

  /// Returns a reference to the current frame.
  auto current_frame() -> FrameData& {
    return _frames[_frame_index];
  }

  /// Begins the frame for the driver. This does the following:
  ///
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
  /// \param platform The platform to poll for input and to use to reinitialize
  ///                 the swapchain if necessary.
  auto begin_frame(Platform& platform) -> bool;

  /// Ends the frame for the driver. This does the following:
  ///
  /// - Presents the current swapchain image to the present queue if it was
  ///   rendered to.
  /// - Performs synchronization of the swapchain image
  ///
  /// This returns true if presentation of the image to the queue was
  /// successfull, or if no rendering was done and there was no need to present
  /// to the queue. It returns false otherwise.
  ///
  /// \param platform The platform to use to reintialize the swapchain is
  ///                 necessary.
  auto end_frame(Platform& platform) -> bool;

  //==--- [command buffers] ------------------------------------------------==//

  /// Request a command buffer with CommandBufferKind, returning a handle to the
  /// command buffer.
  template <CommandBufferKind BufferKind>
  auto request_command_buffer(size_t thread_index = 0) -> CommandBufferHandle {
    auto cmd_buffer = current_frame()
                        .get_command_pool<BufferKind>(thread_index)
                        .request_command_buffer();

    VkCommandBufferBeginInfo info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    _context.device_table()->vkBeginCommandBuffer(cmd_buffer, &info);

    current_command_buffer_counter().fetch_add(1, std::memory_order_relaxed);

    return CommandBufferHandle(
      _allocator.cmd_buffer_allocator.create<VulkanCommandBuffer>(
        this, cmd_buffer, BufferKind, thread_index));
  }

  /// Submits the command buffer pointed to by the \p bufer.
  /// \param buffer The buffer to submit.
  auto submit(CommandBufferHandle buffer) -> void;

  /// Waits idly until everything for the current frame is done.
  auto wait_idle() -> void;

 private:
  //==--- [friends] --------------------------------------------------------==//

  /// Allows the deleter to access the allocator for deletion.
  friend CommandBufferDeleter;

  //==--- [memebers] -------------------------------------------------------==//

  VulkanContext        _context;             //!< Vulkan context.
  VulkanSurfaceContext _surface_context;     //!< Surface related context.
  FrameContainer       _frames;              //!< The frames for the engine.
  DriverAllocator      _allocator;           //!< Allocators for the driver.
  CmdBufferCounters    _cmd_buffer_counters; //!< Number of command buffers.
  uint16_t             _num_threads = 1;     //!< Number of driver threads.
  uint8_t              _frame_index = 0;     //!< Index of the frame.

  // clang-format off
  /// Present mode for the driver.
  PresentMode _present_mode       = PresentMode::sync_to_vblank;
  // If the swapchain has been acquired.
  bool        _acquired_swapchain = false;
  /// If the driver has been destroyed.
  bool        _destroyed          = false;
  // clang-format on

  /// Constructor to initialize the device.
  /// \param platform The platform to create the driver for.
  /// \param threads  The number of threads for the driver.
  VulkanDriver(const Platform& platform, uint16_t threads);

  /// Destructor to clean up driver resources.
  ~VulkanDriver();

  /// Tries to acquire the next swapchain image, returning true on success, and
  /// false on failure.
  /// \param platform The platform to use to recreate the swapchain if
  ///                 necessary.
  auto acquire_next_image(Platform& platform) -> bool;

  /// Advances the frame data for the driver to the next frame.
  auto advance_frame_data() -> void;

  /// Creates the frame data.
  auto create_frame_data() -> void;

  /// Returns the current command buffer counter.
  auto current_command_buffer_counter() -> CmdBufferCounter& {
    return _cmd_buffer_counters[_frame_index];
  }

  /// Flushes all pending submissiong.
  auto flush_pending_submissions() -> void;

  //==--- [clean up] -------------------------------------------------------==//

  /// Destroys the device for the driver.
  auto destroy_device() -> void;

  /// Destroys the frame data.
  auto destroy_frame_data() -> void;

  /// Destroys the instance.
  auto destroy_instance() -> void;

  /// Destroys the surface context.
  auto destroy_surface_context() -> void;
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_VULKAN_DRIVER_HPP
