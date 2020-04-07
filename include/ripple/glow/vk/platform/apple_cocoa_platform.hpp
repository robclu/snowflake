//==--- glow/vk/platform/apple_cocoa_platform.hpp ---------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  apple_cocoa_platform.hpp
/// \brief This file defines functionality for an apple platform with cocoa.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_PLATFORM_APPLE_COCOA_PLATFORM_HPP
#define RIPPLE_GLOW_VK_PLATFORM_APPLE_COCOA_PLATFORM_HPP

#include "platform_base.hpp"
#include "sdl_platform.hpp"

namespace ripple::glow::vk {

/// Defines a platform for Apple using Cocoa.
class AppleCocoaPlatform : public Platform<AppleCocoaPlatform>, SdlPlatform {
  /// Defines the type of the base platform.
  using base_platform_t = Platform<AppleCocoaPlatform>; //!< Base paltform type.
  using sdl_platform_t  = SdlPlatform;                  //!< SDL platfomr type.

 public:
  /// Inherit the constructors from the base platform.
  using base_platform_t::base_platform_t;

  /// Initializes the platform with a \p title.
  /// \param title The title to initialize the platform with.
  auto initialize(const std::string& title) -> void;

  /// Creates a surface for vulkan.
  /// \param instance The instance to create the surface for.
  /// \param device   The device to create the surface from.
  auto create_vulkan_surface(VkInstance instance, VkPhysicalDevice device)
    -> VkSurfaceKHR;

 private:
  /// Gets a pointer to the cocoa NSView.
  auto get_view_ptr() const;
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_APPLE_COCOA_PLATFORM_HPP
