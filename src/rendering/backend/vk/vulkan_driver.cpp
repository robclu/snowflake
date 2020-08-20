//==--- src/rendering/backend/vk/vulkan_driver.cpp --------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_driver.cpp
/// \brief This file defines the implemenation for the vulkan driver.
//
//==------------------------------------------------------------------------==//

#include <snowflake/rendering/backend/vk/vulkan_driver.hpp>
#include <wrench/log/logger.hpp>

#ifndef _WIN32
  #include <dlfcn.h>
#elif defined(_WIN32)
  #include <windows.h>
#endif

namespace snowflake ::backend {

//==--- [interface]
//--------------------------------------------------------==//

auto VulkanDriver::create(
  const VulkanDriver::Platform& platform, uint16_t threads) noexcept
  -> VulkanDriver* {
  static VulkanDriver driver(platform, threads);
  return &driver;
}

auto VulkanDriver::destroy() noexcept -> void {
  if (destroyed_) {
    return;
  }

  wait_idle();

  destroy_frame_data();
  destroy_surface_context();
  context_.destroy();

  // Instance cleans up the rest on destruction
  destroyed_ = true;
}

//==--- [con/destruction] --------------------------------------------------==//

VulkanDriver::VulkanDriver(
  const VulkanDriver::Platform& platform, uint16_t threads) noexcept
: num_threads_(threads), destroyed_(false) {
  for (auto& cmd_buffer_counter : cmd_buffer_counters_) {
    cmd_buffer_counter.store(0, std::memory_order_relaxed);
  }

  auto ins_extensions = platform.instance_extensions();
  auto dev_extensions = platform.device_extensions();
  if (!context_.create_instance_and_device(
        ins_extensions.data(),
        ins_extensions.size(),
        dev_extensions.data(),
        dev_extensions.size(),
        surface_context_.surface())) {
    wrench::Log::logger().flush();
    assert(false && "VulkanDriver could not create VulkanContext");
  }
  wrench::log_info("Created driver vulkan context.");

  surface_context_.surface() =
    platform.create_surface(context_.instance(), context_.physical_device());

  if (surface_context_.surface() == VK_NULL_HANDLE) {
    assert(false && "Failed to create vulkan surface.");
  }

  if (!surface_context_.init(
        context_, present_mode_, platform.width, platform.height)) {
    assert(false && "Failed to create the surface context.");
  }

  create_frame_data();
}

VulkanDriver::~VulkanDriver() noexcept {
  destroy();
}

//==--- [interface] --------------------------------------------------------==//

auto VulkanDriver::submit(CommandBufferHandle buffer) noexcept -> void {
  current_command_buffer_counter().fetch_sub(1, std::memory_order_relaxed);
}

//==--- [frame interface implementation] -----------------------------------==//

auto VulkanDriver::begin_frame(VulkanDriver::Platform& platform) noexcept
  -> bool {
  advance_frame_data();

  // Try and acquire the next image, if this fails, we lost the swapchain, or
  // something like that, and we can't continue.
  if (!acquire_next_image(platform)) {
    return false;
  }
  return true;
}

auto VulkanDriver::end_frame(VulkanDriver::Platform& platform) noexcept
  -> bool {
  // TODO: Add call to reset_frame_data() when there is frame data which
  // required resetting.
  acquired_swapchain_ = false;

  if (!surface_context_.present(context_, current_command_buffer_counter())) {
    // TODO: Reset the swapchain ...
    return false;
  }

  // Managed to present, check if the swapchain changed:
  if (present_mode_ != surface_context_.present_mode()) {
    if (!surface_context_.reinit(
          context_, present_mode_, platform.width, platform.height)) {
      wrench::log_error(
        "Failed to reinitialize surface context during presentation.");
    };
  }
  return true;
}

auto VulkanDriver::flush_pending_submissions() noexcept -> void {
  // Check submission queues, and submit them ...
}

auto VulkanDriver::wait_idle() noexcept -> void {
  // End frame ...
  flush_pending_submissions();
  if (context_.device() != VK_NULL_HANDLE) {
    auto result = vkDeviceWaitIdle(context_.device());
    if (result != VK_SUCCESS) {
      wrench::log_error("Failed to idle device : {}", result);
    }
  }

  // Clear all waiting semaphores ...

  // Free memory

  // Reset descriptor set allocators

  // Reset all frame data:
  for (auto& frame : frames_) {
    frame.reset();
  }
}

//==--- [private]
//----------------------------------------------------------==//

auto VulkanDriver::acquire_next_image(VulkanDriver::Platform& platform) noexcept
  -> bool {
  if (acquired_swapchain_) {
    return true;
  }

  VkResult result = VK_SUCCESS;
  do {
    result = context_.device_table()->vkAcquireNextImageKHR(
      context_.device(),
      surface_context_.swapchain(),
      std::numeric_limits<uint64_t>::max(),
      surface_context_.image_available_semaphore(),
      VK_NULL_HANDLE,
      &surface_context_.current_swap_index());

#ifdef ANDROID
    // With the pre-transform for mobile, on adroid this might return
    // suboptimal, which is fine, and treated as success.
    if (result == VK_SUBOPTIMAL_KHR) {
      result = VK_SUCCESS;
    }
#endif

    if (result == VK_SUCCESS) {
      acquired_swapchain_ = true;

      // Poll the platform, so that we get good latency:
      platform.poll_input();

      return true;
    }

    // Some errors with the swapchain:
    const auto swapchain_error = result == VK_SUBOPTIMAL_KHR ||
                                 result == VK_ERROR_OUT_OF_DATE_KHR;
    if (swapchain_error) {
      // Need to recreate the swapchain ...
      surface_context_.reinit(
        context_, present_mode_, platform.width, platform.height);
      continue;
    }
    return false;
  } while (result != VK_SUCCESS);
  return true;
}

auto VulkanDriver::advance_frame_data() noexcept -> void {
  // Flush the frame, incase there are pending operations ...

  if (frames_.empty()) {
    wrench::log_error("No frame data for driver!");
  }

  frame_index_ = (frame_index_ + 1) % num_frame_contexts;
  current_frame().reset();
}

auto VulkanDriver::create_frame_data() noexcept -> void {
  for (uint8_t i = 0; i < num_frame_contexts; ++i) {
    frames_.emplace_back(
      this,
      context_.graphics_queue_family_index(),
      context_.compute_queue_family_index(),
      context_.transfer_queue_family_index());
  }
}

//==--- [destruction] ------------------------------------------------------==//

auto VulkanDriver::destroy_device() noexcept -> void {
  vkDestroyDevice(context_.device(), nullptr);
}

auto VulkanDriver::destroy_frame_data() noexcept -> void {
  for (auto& frame : frames_) {
    frame.destroy();
  }
}

auto VulkanDriver::destroy_instance() noexcept -> void {
  vkDestroyInstance(context_.instance(), nullptr);
}

auto VulkanDriver::destroy_surface_context() noexcept -> void {
  surface_context_.destroy(context_);
}

} // namespace snowflake::backend