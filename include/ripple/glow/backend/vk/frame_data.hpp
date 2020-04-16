//==--- glow/backend/vk/frame_data.hpp --------------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  frame_data.hpp
/// \brief Header file for a data and functionality specific to a frame.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_VK_FRAME_DATA_HPP
#define RIPPLE_GLOW_BACKEND_VK_FRAME_DATA_HPP

#include "vulkan_command_pool.hpp"

namespace ripple::glow::backend {

/// Holds the command buffer pools for the frame. Each pool type is stored in a
/// container, where each container should hold as many pools of each type as
/// there are threads. For 4 threads, there would be 12 queues in total, because
/// for each frame there would be a graphics, compute, and transfer queue.
struct FrameCommandPools {
  /// Defines the type of the container for the pool. There should be a pool per
  /// thread, so the pools each have their own container.
  using pool_container_t = std::vector<VulkanCommandPool>;

  pool_container_t graphics; //!< Command pools for graphics.
  pool_container_t compute;  //!< Command pools for compute.
  pool_container_t transfer; //!< Command pools for transfer.

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to create each of the pools with the appropriate queue index.
  /// This will create each pool with the number of threads returned by the
  /// `num_threads()` call from the driver.
  /// \param driver                      The driver for pool creation.
  /// \param graphics_queue_family_index Graphics family index.
  /// \param compute_queue_family_index  Compute family index.
  /// \param transfer_queue_family_index Transfer family index.
  FrameCommandPools(
    VulkanDriver* driver,
    uint32_t      graphics_queue_family_index,
    uint32_t      compute_queue_family_index,
    uint32_t      transfer_queue_family_index);

  /// Resets each of the command pools.
  auto reset() -> void;
};

/// Holds synchronization primitives for the frame. This struct is designed to
/// use Vulkan >= 1.2, which supports timeline semaphores, thus reducing the
/// need for fences in the frame synchronisation.
struct FrameSync {
  // clang-format off
  /// Semaphore for the graphics queue.
  VkSemaphore graphics_timeline_semaphore = VK_NULL_HANDLE;
  /// Semaphore for the compute queue.
  VkSemaphore compute_timeline_semaphore  = VK_NULL_HANDLE;
  /// Semaphore for the transfer queue.
  VkSemaphore transfer_timeline_semaphore = VK_NULL_HANDLE;

  uint64_t graphics_timeline_fence = 0; //!< Graphics fence value.
  uint64_t compute_timeline_fence  = 0; //!< Compute fence value.
  uint64_t transfer_timeline_fence = 0; //!< Transfer fence value.
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Default constructor which just uses the defaults.
  FrameSync() = default;

  /// Returns true if all semaphores are valid.
  auto all_semaphores_valid() const -> bool {
    const auto valid = graphics_timeline_semaphore &&
                       compute_timeline_semaphore &&
                       transfer_timeline_semaphore;
    return valid;
  }
};

/// Holds per frame data for the driver.
struct FrameData {
  FrameCommandPools command_pools; //!< Command pools for the frame.
  FrameSync         sync;          //!< Syncronization for the frame.

  //==--- [constructor] ----------------------------------------------------==//

  /// Constructor to create the frame data with the \p driver for the frame
  /// data, and the indices of the queues for the frame data.
  /// \param driver The driver for creating the frame data resources.
  /// \param graphics_queue_index The index of the graphics queue.
  /// \param compute_queue_index  The index of the compute queue.
  /// \param transfer_queue_index The index of the transfer_queue.
  FrameData(
    VulkanDriver* driver,
    uint32_t      graphics_queue_index,
    uint32_t      compute_queue_index,
    uint32_t      transfer_queue_index);

  /// Resets all data for the frame data.
  auto reset() -> void;

 private:
  VulkanDriver* _driver = nullptr; //!< A pointer to the driver.
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VK_FRAME_DATA_HPP
