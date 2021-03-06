//==--- snowflake/rendering/backend/vk/frame_data.hpp ------ -*- C++ -*- ---==//
//
//                                Snowflake
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

#ifndef SNOWFLAKE_RENDERING_BACKEND_VK_FRAME_DATA_HPP
#define SNOWFLAKE_RENDERING_BACKEND_VK_FRAME_DATA_HPP

#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"

namespace snowflake::backend {

/// Holds the command buffer pools for the frame. Each pool type is stored in a
/// container, where each container should hold as many pools of each type as
/// there are threads. For 4 threads, there would be 12 queues in total, because
/// for each frame there would be a graphics, compute, and transfer queue.
struct FrameCommandPools {
  /// Defines the type of the container for the pool. There should be a
  /// pool per thread, so the pools each have their own container.
  using PoolContainer = std::vector<VulkanCommandPool>;

  PoolContainer graphics; //!< Command pools for graphics.
  PoolContainer compute;  //!< Command pools for compute.
  PoolContainer transfer; //!< Command pools for transfer.

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
    uint32_t      transfer_queue_family_index) noexcept;

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a reference to the command pools for BufferKind command buffers.
  template <CommandBufferKind BufferKind>
  auto get_pools() noexcept -> PoolContainer& {
    if constexpr (BufferKind == CommandBufferKind::graphics) {
      return graphics;
    } else if (BufferKind == CommandBufferKind::compute) {
      return compute;
    } else if (BufferKind == CommandBufferKind::transfer) {
      return transfer;
    }
  }

  /// Resets each of the command pools.
  auto reset() noexcept -> void;
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

  /// Returns true if all semaphores are valid.
  auto all_semaphores_valid() const noexcept -> bool {
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

  //==--- [construction] ---------------------------------------------------==//

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
    uint32_t      transfer_queue_index) noexcept;

  /// Destroys the frame data.
  auto destroy() noexcept -> void;

  /// Resets all data for the frame data. This will wait on the semaphores if
  /// they are all valid.
  auto reset() noexcept -> void;

  /// Returns the command pool of the command buffer kind specified by
  /// BufferKind, for the \p thread_index.
  /// \param  thread_id   The thread index of the pool to get.
  /// \tparam BufferKind  The kind of the command buffers in the pool.
  template <CommandBufferKind BufferKind>
  auto get_command_pool(size_t thread_id = 0) noexcept -> VulkanCommandPool& {
    return command_pools.get_pools<BufferKind>()[thread_id];
  }

 private:
  VulkanDriver* driver_ = nullptr; //!< A pointer to the driver.
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_VK_FRAME_DATA_HPP
