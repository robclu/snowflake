//==--- src/rendering/backend/vk/vulkan_command_pool.cpp --- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_command_pool.cpp
/// \brief This file defines the implemenation of a vulkan command pool.
//
//==------------------------------------------------------------------------==//

#include <snowflake/rendering/backend/vk/vulkan_command_pool.hpp>
#include <snowflake/rendering/backend/vk/vulkan_driver.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake::backend {

VulkanCommandPool::VulkanCommandPool(
  VulkanCommandPool::DriverPtr driver, uint32_t queue_family_index) noexcept
: driver_(driver) {
  VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  info.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  info.queueFamilyIndex        = queue_family_index;
  auto result = driver_->context().device_table()->vkCreateCommandPool(
    driver_->context().device(), &info, nullptr, &pool_);

  if (result != VK_SUCCESS) {
    wrench::log_error(
      "Failed to create command pool for queue family : {}",
      queue_family_index);
  }
}

VulkanCommandPool::VulkanCommandPool(VulkanCommandPool&& other) noexcept {
  *this = std::move(other);
}

auto VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept
  -> VulkanCommandPool& {
  if (this != &other) {
    const auto* table  = driver_->context().device_table();
    const auto  device = driver_->context().device();

    driver_ = other.driver_;
    if (!buffers_.empty()) {
      table->vkFreeCommandBuffers(
        device, pool_, buffers_.size(), buffers_.data());
    }
    if (pool_ != VK_NULL_HANDLE) {
      table->vkDestroyCommandPool(device, pool_, nullptr);
    }

    pool_ = VK_NULL_HANDLE;
    buffers_.clear();
    std::swap(pool_, other.pool_);
    std::swap(buffers_, other.buffers_);
    index_       = other.index_;
    other.index_ = 0;
  }
  return *this;
}

VulkanCommandPool::~VulkanCommandPool() noexcept {}

auto VulkanCommandPool::destroy() noexcept -> void {
  auto* table  = driver_->context().device_table();
  auto  device = driver_->context().device();
  if (!buffers_.empty()) {
    table->vkFreeCommandBuffers(
      device, pool_, buffers_.size(), buffers_.data());
  }
  if (!secondary_buffers_.empty()) {
    table->vkFreeCommandBuffers(
      device, pool_, secondary_buffers_.size(), secondary_buffers_.data());
  }
  if (pool_ != VK_NULL_HANDLE) {
    table->vkDestroyCommandPool(device, pool_, nullptr);
  }
}

auto VulkanCommandPool::request_secondary_command_buffer() noexcept
  -> VkCommandBuffer {
  // Check if we can get one without allocating:
  if (secondary_index_ < secondary_buffers_.size()) {
    auto buffer = secondary_buffers_[secondary_index_++];
    return buffer;
  }

  // Pool is full, allocate one:
  auto&                       cmd_buffer = secondary_buffers_.emplace_back();
  VkCommandBufferAllocateInfo info       = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandPool        = pool_;
  info.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  info.commandBufferCount = 1;

  driver_->context().device_table()->vkAllocateCommandBuffers(
    driver_->context().device(), &info, &cmd_buffer);
  secondary_index_++;
  return cmd_buffer;
}

auto VulkanCommandPool::request_command_buffer() noexcept -> VkCommandBuffer {
  // Try and get one without allocating:
  if (index_ < buffers_.size()) {
    auto buffer = buffers_[index_++];
    return buffer;
  }

  // Pool is full, have to allocate one:
  auto&                       cmd_buffer = buffers_.emplace_back();
  VkCommandBufferAllocateInfo info       = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandPool        = pool_;
  info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = 1;

  driver_->context().device_table()->vkAllocateCommandBuffers(
    driver_->context().device(), &info, &cmd_buffer);
  index_++;
  return cmd_buffer;
}

auto VulkanCommandPool::reset() noexcept -> void {
  if (index_ > 0 || secondary_index_ > 0) {
    driver_->context().device_table()->vkResetCommandPool(
      driver_->context().device(), pool_, 0);
  }
  index_           = 0;
  secondary_index_ = 0;
}

} // namespace snowflake::backend