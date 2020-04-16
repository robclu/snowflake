//==--- glow/src/backend/vk/vulkan_command_pool.cpp -------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_command_pool.cpp
/// \brief This file defines the implemenation of a vulkan command pool.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/backend/vk/vulkan_command_pool.hpp>
#include <ripple/glow/backend/vk/vulkan_driver.hpp>

namespace ripple::glow::backend {

VulkanCommandPool::VulkanCommandPool(
  VulkanCommandPool::driver_ptr driver, uint32_t queue_family_index)
: _driver(driver) {
  VkCommandPoolCreateInfo info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  info.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  info.queueFamilyIndex        = queue_family_index;
  auto result = _driver->context().device_table()->vkCreateCommandPool(
    _driver->context().device(), &info, nullptr, &_pool);

  if (result != VK_SUCCESS) {
    log_error(
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
    const auto& table  = _driver->context().device_table();
    const auto& device = _driver->context().device();

    _driver = other._driver;
    if (!_buffers.empty()) {
      table->vkFreeCommandBuffers(
        device, _pool, _buffers.size(), _buffers.data());
    }
    if (_pool != VK_NULL_HANDLE) {
      table->vkDestroyCommandPool(device, _pool, nullptr);
    }

    _pool = VK_NULL_HANDLE;
    _buffers.clear();
    std::swap(_pool, other._pool);
    std::swap(_buffers, other._buffers);
    _index       = other._index;
    other._index = 0;
  }
  return *this;
}

VulkanCommandPool::~VulkanCommandPool() {
  const auto& table  = _driver->context().device_table();
  const auto& device = _driver->context().device();
  if (!_buffers.empty()) {
    table->vkFreeCommandBuffers(
      device, _pool, _buffers.size(), _buffers.data());
  }
  if (!_secondary_buffers.empty()) {
    table->vkFreeCommandBuffers(
      device, _pool, _secondary_buffers.size(), _secondary_buffers.data());
  }
  if (_pool != VK_NULL_HANDLE) {
    table->vkDestroyCommandPool(device, _pool, nullptr);
  }
}

auto VulkanCommandPool::request_secondary_command_buffer() -> VkCommandBuffer {
  // Check if we can get one without allocating:
  if (_secondary_index < _secondary_buffers.size()) {
    auto buffer = _secondary_buffers[_secondary_index++];
    return buffer;
  }

  // Pool is full, allocate one:
  auto&                       cmd_buffer = _secondary_buffers.emplace_back();
  VkCommandBufferAllocateInfo info       = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandPool        = _pool;
  info.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  info.commandBufferCount = 1;

  _driver->context().device_table()->vkAllocateCommandBuffers(
    _driver->context().device(), &info, &cmd_buffer);
  _secondary_index++;
  return cmd_buffer;
}

auto VulkanCommandPool::request_command_buffer() -> VkCommandBuffer {
  // Try and get one without allocating:
  if (_index < _buffers.size()) {
    auto buffer = _buffers[_index++];
    return buffer;
  }

  // Pool is full, have to allocate one:
  auto&                       cmd_buffer = _buffers.emplace_back();
  VkCommandBufferAllocateInfo info       = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  info.commandPool        = _pool;
  info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = 1;

  _driver->context().device_table()->vkAllocateCommandBuffers(
    _driver->context().device(), &info, &cmd_buffer);
  _index++;
  return cmd_buffer;
}

auto VulkanCommandPool::reset() -> void {
  if (_index > 0 || _secondary_index > 0) {
    _driver->context().device_table()->vkResetCommandPool(
      _driver->context().device(), _pool, 0);
  }
  _index           = 0;
  _secondary_index = 0;
}

} // namespace ripple::glow::backend