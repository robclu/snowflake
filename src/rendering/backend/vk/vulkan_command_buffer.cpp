//==--- src/rendering/backend/vk/vulkan_command_buffer.cpp -- -*- C++ -*----==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_command_buffer.cpp
/// \brief Implementation for vulkan command buffers.
//
//==------------------------------------------------------------------------==//

#include <snowflake/rendering/backend/vk/vulkan_command_buffer.hpp>
#include <snowflake/rendering/backend/vk/vulkan_driver.hpp>

namespace snowflake::backend {

auto CommandBufferDeleter::operator()(VulkanCommandBuffer* buffer) const
  noexcept -> void {
  buffer->driver_->allocator_.cmd_buffer_allocator.recycle(buffer);
}

} // namespace snowflake::backend