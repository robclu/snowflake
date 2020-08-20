//==--- ../rendering/backend/platform/platform_base.hpp ---  -*- C++ -*- ---==//
//
//                              Snowflake
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

#ifndef SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_BASE_HPP
#define SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_BASE_HPP

#include "platform_fwd.hpp"
#include <snowflake/rendering/backend/vk/vulkan_headers.hpp>
#include <snowflake/util/portability.hpp>
#include <type_traits>
#include <vector>

namespace snowflake::backend {

/// Alias for the type of the extension vector.
using ExtVector = std::vector<const char*>;

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
  /// Defines the size type used by the platform.
  using SizeType = uint32_t;

  //==--- [constants] ------------------------------------------------------==//

  static constexpr SizeType default_w = 1280; //!< Default widht.
  static constexpr SizeType default_h = 720;  //!< Default height.

  //==--- [friends] --------------------------------------------------------==//

  /// Allow the implementation to access the class internals.
  friend Impl;

  /// Returns a const pointer to the implementation.
  constexpr auto impl() const noexcept -> const Impl* {
    return static_cast<const Impl*>(this);
  }

  /// Returns a pointer to the implementation.
  constexpr auto impl() noexcept -> Impl* {
    return static_cast<Impl*>(this);
  }

 public:
  SizeType width  = default_w; //!< Width of the window.
  SizeType height = default_h; //!< Height of the window

  //==--- [interface] ------------------------------------------------------==//

  /// Constructor to create a platform with a default size.
  Platform() noexcept = default;

  /// Constructor for the platform, which sets the \p x and \py dimensions
  /// of the surface for the platform.
  /// \param width  The (width) of the surface  - pixels in the x dimension.
  /// \param height The (height) of the surface - pixels in the y dimension.
  Platform(SizeType width, SizeType height) noexcept
  : width(width), height(height) {}

  /// Creates a surface for the platform, and returns it.
  /// \param instance The instance to create the surface for.
  /// \param device   The device to create the surface from.
  auto create_surface(VkInstance instance, VkPhysicalDevice device) const
    noexcept -> VkSurfaceKHR {
    return impl()->create_vulkan_surface(instance, device);
  }

  /// Gets the vulkan device extensions for the platform.
  auto device_extensions() const noexcept -> ExtVector {
    return impl()->get_device_extensions();
  }

  /// Gets the vulkan instance extensions for the platform.
  auto instance_extensions() const noexcept -> ExtVector {
    return impl()->get_instance_extensions();
  }

  /// Returns true if the platform is still alive.
  auto is_alive() const noexcept -> bool {
    return impl()->is_alive_impl();
  }

  /// Polls the platform for input events, handling any events which happen.
  auto poll_input() noexcept -> void {
    impl()->poll_input_impl();
  }

  /// Resizes the platform surface.
  /// \param w  The width to resize to.
  /// \param h The height to resize to.
  auto resize(SizeType w, SizeType h) noexcept -> void {
    width  = w;
    height = h;
    impl()->resize_impl();
  }

  /// Sets the title of the platform.
  /// \param title The title for the platform.
  auto set_title(const char* title) -> void {
    impl()->set_title_impl(title);
  }

 private:
  SizeType width_  = default_w; //!< Width of the window.
  SizeType height_ = default_h; //!< Height of the window
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_HPP
