//==--- src/rendering/backend/vk/frame_data.cpp ------------ -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  frame_data.cpp
/// \brief This file defines the implemenation for frame data.
//
//==------------------------------------------------------------------------==//

#include <snowflake/rendering/backend/vk/frame_data.hpp>
#include <snowflake/rendering/backend/vk/vulkan_driver.hpp>
#include <array>

namespace snowflake::backend {

FrameData::FrameData(
  VulkanDriver* driver,
  uint32_t      graphics_queue_index,
  uint32_t      compute_queue_index,
  uint32_t      transfer_queue_index) noexcept
: driver_(driver),
  command_pools(
    driver, graphics_queue_index, compute_queue_index, transfer_queue_index) {}

auto FrameData::reset() noexcept -> void {
  // Wait on the semaphores and fences:
  if (sync.all_semaphores_valid()) {
    // clang-format off
    VkSemaphoreWaitInfo info   = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR };
    const auto          semas  = std::array<VkSemaphore, 3>{
      sync.graphics_timeline_semaphore,
      sync.compute_timeline_semaphore,
      sync.transfer_timeline_semaphore
    };
    const auto          values = std::array<uint64_t, 3>{
      sync.graphics_timeline_fence,
      sync.compute_timeline_fence,
      sync.transfer_timeline_fence
    };
    // clang-format on
    info.pSemaphores    = semas.data();
    info.pValues        = values.data();
    info.semaphoreCount = semas.size();

    driver_->context().device_table()->vkWaitSemaphoresKHR(
      driver_->context().device(), &info, std::numeric_limits<uint64_t>::max());
  }

  // Reset all frame resources:
  command_pools.reset();
}

auto FrameData::destroy() noexcept -> void {
  // clang-format off
  // Make sure that these can't be waited on
  sync.graphics_timeline_fence = 0;
  sync.compute_timeline_fence  = 0;
  sync.transfer_timeline_fence = 0;
  // clang-format on

  auto* dev_table = driver_->context().device_table();
  auto  device    = driver_->context().device();
  if (sync.graphics_timeline_semaphore != VK_NULL_HANDLE) {
    dev_table->vkDestroySemaphore(
      device, sync.graphics_timeline_semaphore, nullptr);
    sync.graphics_timeline_semaphore = VK_NULL_HANDLE;
  }
  if (sync.compute_timeline_semaphore != VK_NULL_HANDLE) {
    dev_table->vkDestroySemaphore(
      device, sync.compute_timeline_semaphore, nullptr);
    sync.compute_timeline_semaphore = VK_NULL_HANDLE;
  }
  if (sync.transfer_timeline_semaphore != VK_NULL_HANDLE) {
    dev_table->vkDestroySemaphore(
      device, sync.transfer_timeline_semaphore, nullptr);
    sync.transfer_timeline_semaphore = VK_NULL_HANDLE;
  }

  // clang-format off
  for (auto& pool : command_pools.graphics) { pool.destroy(); }
  for (auto& pool : command_pools.compute)  { pool.destroy(); }
  for (auto& pool : command_pools.transfer) { pool.destroy(); }
  // clang-format on
}

//==--- [frame command pools] ----------------------------------------------==//

FrameCommandPools::FrameCommandPools(
  VulkanDriver* driver,
  uint32_t      graphics_queue_family_index,
  uint32_t      compute_queue_family_index,
  uint32_t      transfer_queue_family_index) noexcept {
  for (uint16_t i = 0; i < driver->num_threads(); ++i) {
    graphics.emplace_back(driver, graphics_queue_family_index);
    compute.emplace_back(driver, compute_queue_family_index);
    transfer.emplace_back(driver, transfer_queue_family_index);
  }
}

auto FrameCommandPools::reset() noexcept -> void {
  // clang-format off
  for (auto& pool : graphics) { pool.reset(); }
  for (auto& pool : compute)  { pool.reset(); }
  for (auto& pool : transfer) { pool.reset(); }
  // clang-format on
}

} // namespace snowflake::backend