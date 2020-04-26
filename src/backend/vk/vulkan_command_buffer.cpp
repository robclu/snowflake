//==--- glow/src/backend/vk/vulkan_command_buffer.cpp ------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
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

#include <ripple/glow/backend/vk/vulkan_command_buffer.hpp>
#include <ripple/glow/backend/vk/vulkan_driver.hpp>

namespace ripple::glow::backend {

auto CommandBufferDeleter::operator()(VulkanCommandBuffer* buffer) const
  -> void {
  buffer->_driver->_allocator.cmd_buffer_allocator.destroy(buffer);
}

} // namespace ripple::glow::backend