//==--- glow/backend/platform/platform_base.hpp ------------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform_base.hpp
/// \brief This file defines an interface for a platform.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_BASE_HPP
#define RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_BASE_HPP

#include "platform_fwd.hpp"
#include "../vk/vulkan_headers.hpp"
#include <ripple/core/util/portability.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace ripple::glow::backend {

/// Alias for the type of the extension vector.
using ext_vector_t = std::vector<const char*>;

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

  /// Constructor to create a platform with a default size.
  Platform() = default;

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

  /// Gets the vulkan device extensions for the platform.
  auto device_extensions() const -> ext_vector_t {
    return impl()->get_device_extensions();
  }

  /// Gets the vulkan instance extensions for the platform.
  auto instance_extensions() const -> ext_vector_t {
    return impl()->get_instance_extensions();
  }

  /// Returns true if the platform is still alive.
  auto is_alive() const -> bool {
    return impl()->is_alive_impl();
  }

  /// Polls the platform for input events, handling any events which happen.
  auto poll_input() -> void {
    impl()->poll_input_impl();
  }

  /// Resizes the platform surface.
  /// \param width  The width to resize to.
  /// \param height The height to resize to.
  auto resize(uint32_t width, uint32_t height) -> void {
    _width  = width;
    _height = height;
    impl()->resize_impl();
  }

  /// Sets the width of the platform surface to \p width.
  /// \param width The width to set the platform to.
  auto set_surface_width(uint32_t width) -> void {
    _width = width;
  }

  /// Sets the height of the platform surface  to \p height.
  /// \param height The height to set the platform to.
  auto set_surface_height(uint32_t height) -> void {
    _height = height;
  }

  /// Sets the title of the platform.
  /// \param title The title for the platform.
  auto set_title(const std::string& title) -> void {
    impl()->set_title_impl(title);
  }

 private:
  uint32_t _width  = default_w; //!< Width of the window.
  uint32_t _height = default_h; //!< Height of the window
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_HPP