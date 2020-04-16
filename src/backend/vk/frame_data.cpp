//==--- glow/src/backend/vk/frame_data.cpp ----------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
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

#include <ripple/glow/backend/vk/frame_data.hpp>
#include <ripple/glow/backend/vk/vulkan_driver.hpp>
#include <array>

namespace ripple::glow::backend {

FrameData::FrameData(
  VulkanDriver* driver,
  uint32_t      graphics_queue_index,
  uint32_t      compute_queue_index,
  uint32_t      transfer_queue_index)
: _driver(driver),
  command_pools(
    driver, graphics_queue_index, compute_queue_index, transfer_queue_index) {}

auto FrameData::reset() -> void {
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

    _driver->context().device_table()->vkWaitSemaphoresKHR(
      _driver->context().device(), &info, std::numeric_limits<uint64_t>::max());
  }

  // Reset all frame resources:
  command_pools.reset();
}

//==--- [frame command pools] ----------------------------------------------==//

FrameCommandPools::FrameCommandPools(
  VulkanDriver* driver,
  uint32_t      graphics_queue_family_index,
  uint32_t      compute_queue_family_index,
  uint32_t      transfer_queue_family_index) {
  for (uint16_t i = 0; i < driver->num_threads(); ++i) {
    graphics.emplace_back(driver, graphics_queue_family_index);
    compute.emplace_back(driver, compute_queue_family_index);
    transfer.emplace_back(driver, transfer_queue_family_index);
  }
}

auto FrameCommandPools::reset() -> void {
  // clang-format off
  for (auto& pool : graphics) { pool.reset(); }
  for (auto& pool : compute)  { pool.reset(); }
  for (auto& pool : transfer) { pool.reset(); }
}

} // namespace ripple::glow::backend