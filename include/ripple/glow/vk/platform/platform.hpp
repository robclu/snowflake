//==--- glow/vk/platform/platform.hpp ---------------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform.hpp
/// \brief This file defines an includes the appropriate platform.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_PLATFORM_PLATFORM_HPP
#define RIPPLE_GLOW_VK_PLATFORM_PLATFORM_HPP

#if defined(GLOW_HEADLESS_PLATFORM)

#elif defined(__APPLE__)
#  include "apple_cocoa_platform.hpp"
#elif defined(__LINUX__)

#elif defined(__ANDROID__)

#elif defined(_WIN32) || defined(WIN32)

#else

#endif

namespace ripple::glow::vk {

/// Alias for the type of the platform to create. The platform is specific to
/// what we are running on, unless another flag has been defined in the build
/// to specify the type of the platform. If nothing matches, the platform is a
/// headless platform.
using platform_type_t =
#if defined(GLOW_HEADLESS_PLATFORM)
  HeadlessPlatform;
#elif defined(__APPLE__)
  AppleCocoaPlatform;
#elif defined(__LINUX__)
  LinuxPlatform;
#elif defined(__ANDROID__)
  AndroidPlatform;
#elif defined(_WIN32) || defined(WIN32)
  WindowsPlatform;
#else
  HeadlessPlatform;
#endif

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_PLATFORM_HPP