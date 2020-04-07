//==--- glow/vk/platform/platform_base.hpp ----------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform_base.hpp
/// \brief This file defines an interface for a platform.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_PLATFORM_PLATFORM_BASE_HPP
#define RIPPLE_GLOW_VK_PLATFORM_PLATFORM_BASE_HPP

#include "platform_fwd.hpp"
#include <string>
#include <type_traits>
#include <vulkan/vulkan.h>

namespace ripple::glow::vk {

/// The Platform type defines an interface for platform-specific
/// window-intefration funcitonality.
///
/// This class uses a static interface since the platform is always known at
/// compile time, and thus, even if the cost of virtual functions is small,
/// there is no point in paying for them.
///
/// Anything that is general for all platforms, is implemented here, and
/// additional functionality can be provided in the implementation class.
///
/// \tparam Impl The type of the implementation of the interface.
template <typename Impl>
class Platform {
  using impl_t = std::decay_t<Impl>; //!< Implementation type.

  //==--- [constants] ------------------------------------------------------==//

  static constexpr auto default_w = 1280; //!< Default widht.
  static constexpr auto default_h = 720;  //!< Default height.

  //==--- [friends] --------------------------------------------------------==//

  /// Allow the implementation to access the class internals.
  friend Impl;

  /// Returns a const pointer to the implementation.
  constexpr auto impl() const -> const impl_t* {
    return static_cast<const impl_t*>(this);
  }

  /// Returns a pointer to the implementation.
  constexpr auto impl() -> impl_t* {
    return static_cast<impl_t*>(this);
  }

 public:
  //==--- [interface] ------------------------------------------------------==//

  /// Constructor for the platform, which sets the \p x and \py dimensions
  /// of the surface for the platform.
  /// \param width  The (width) of the surface  - pixels in the x dimension.
  /// \param height The (height) of the surface - pixels in the y dimension.
  Platform(uint32_t width, uint32_t height) : _width(width), _height(height) {}

  /// Creates a surface for the platform, and returns it.
  /// \param instance The instance to create the surface for.
  /// \param device   The device to create the surface from.
  auto
  create_surface(VkInstance instance, VkPhysicalDevice device) -> VkSurfaceKHR {
    return impl()->create_vulkan_surface(instance, device);
  }

  /// Initializes the platform with a title.
  /// \param title The title for the window.
  auto init(const std::string& title) -> void {
    impl()->initialize(title);
  }

 private:
  uint32_t _width  = default_w; //!< Width of the window.
  uint32_t _height = default_h; //!< Height of the window
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_PLATFORM_HPP