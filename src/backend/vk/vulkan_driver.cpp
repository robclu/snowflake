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

auto VulkanDriver::create(const VulkanDriver::platform_t& platform)
  -> VulkanDriver* {
  static VulkanDriver driver(platform);
  return &driver;
}

//==--- [con/destruction] --------------------------------------------------==//

VulkanDriver::VulkanDriver(const VulkanDriver::platform_t& platform) {
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
}

VulkanDriver::~VulkanDriver() {}

} // namespace ripple::glow::backend