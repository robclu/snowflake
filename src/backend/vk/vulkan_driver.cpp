//==--- glow/src/backend/vk/vulkan_driver.cpp -------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_driver.cpp
/// \brief This file defines the implemenation for the vulkan driver.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/backend/vk/vulkan_driver.hpp>

#ifndef _WIN32
  #include <dlfcn.h>
#elif defined(_WIN32)
  #include <windows.h>
#endif

namespace ripple::glow::backend {

//==--- [interface] --------------------------------------------------------==//

auto VulkanDriver::create(
  const VulkanDriver::Platform& platform, uint16_t threads) -> VulkanDriver* {
  static VulkanDriver driver(platform, threads);
  return &driver;
}

//==--- [con/destruction] --------------------------------------------------==//

VulkanDriver::VulkanDriver(
  const VulkanDriver::Platform& platform, uint16_t threads)
: _num_threads(threads) {
  for (auto& cmd_buffer_counter : _cmd_buffer_counters) {
    cmd_buffer_counter.store(0, std::memory_order_relaxed);
  }

  auto ins_extensions = platform.instance_extensions();
  auto dev_extensions = platform.device_extensions();
  if (!_context.create_instance_and_device(
        ins_extensions.data(),
        ins_extensions.size(),
        dev_extensions.data(),
        dev_extensions.size())) {
    logger_t::logger().flush();
    assert(false && "VulkanDriver could not create VulkanContext");
  }
  log_info("Created driver vulkan context.");

  _surface_context.surface() =
    platform.create_surface(_context.instance(), _context.physical_device());

  if (_surface_context.surface() == VK_NULL_HANDLE) {
    assert(false && "Failed to create vulkan surface.");
  }

  if (!_surface_context.init(
        _context, _present_mode, platform.width(), platform.height())) {
    assert(false && "Failed to create the surface context.");
  }

  create_frame_data();
}

VulkanDriver::~VulkanDriver() {}

//==--- [frame interface implementation] -----------------------------------==//

auto VulkanDriver::begin_frame(VulkanDriver::Platform& platform) -> bool {
  advance_frame_data();

  // Try and acquire the next image, if this fails, we lost the swapchain, or
  // something like that, and we can't continue.
  if (!acquire_next_image(platform)) {
    return false;
  }
  return true;
}

auto VulkanDriver::end_frame(VulkanDriver::Platform& platform) -> bool {
  // TODO: Add call to reset_frame_data() when there is frame data which
  // required resetting.
  _acquired_swapchain = false;

  if (!_surface_context.present(_context, current_command_buffer_counter())) {
    // Reset the swapchain ...
    return false;
  }

  // Managed to present, check if the swapchain changed:
  if (_present_mode != _surface_context.present_mode()) {
    if (!_surface_context.reinit(
          _context, _present_mode, platform.width(), platform.height())) {
      log_error("Failed to reinitialize surface context during presentation.");
    };
  }
  return true;
}

//==--- [private] ----------------------------------------------------------==//

auto VulkanDriver::acquire_next_image(VulkanDriver::Platform& platform)
  -> bool {
  if (_acquired_swapchain) {
    return true;
  }

  VkResult result = VK_SUCCESS;
  do {
    result = _context.device_table()->vkAcquireNextImageKHR(
      _context.device(),
      _surface_context.swapchain(),
      std::numeric_limits<uint64_t>::max(),
      _surface_context.image_available_semaphore(),
      VK_NULL_HANDLE,
      &_surface_context.current_swap_index());

#ifdef ANDROID
    // With the pre-transform for mobile, on adroid this might return
    // suboptimal, which is fine, and treated as success.
    if (result == VK_SUBOPTIMAL_KHR) {
      result = VK_SUCCESS;
    }
#endif

    if (result == VK_SUCCESS) {
      _acquired_swapchain = true;

      // Poll the platform, so that we get good latency:
      platform.poll_input();

      return true;
    }

    // Some errors with the swapchain:
    const auto swapchain_error = result == VK_SUBOPTIMAL_KHR ||
                                 result == VK_ERROR_OUT_OF_DATE_KHR;
    if (swapchain_error) {
      // Need to recreate the swapchain ...
      _surface_context.reinit(
        _context, _present_mode, platform.width(), platform.height());
      continue;
    }
    return false;
  } while (result != VK_SUCCESS);
  return true;
}

auto VulkanDriver::advance_frame_data() -> void {
  // Flush the frame, incase there are pending operations ...

  if (_frames.empty()) {
    log_error("No frame data for driver!");
  }

  _frame_index = (_frame_index + 1) % num_frame_contexts_v;
  current_frame().reset();
}

auto VulkanDriver::create_frame_data() -> void {
  for (uint8_t i = 0; i < num_frame_contexts_v; ++i) {
    _frames.emplace_back(
      this,
      _context.graphics_queue_family_index(),
      _context.compute_queue_family_index(),
      _context.transfer_queue_family_index());
  }
}

} // namespace ripple::glow::backend