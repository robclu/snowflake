//==--- .../rendering/backend/platform/sdl_platform.hpp ---- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  sdl_platform.hpp
/// \brief This file defines functionality for SDL which can be used by
///        platforms which use SDL.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERING_BACKEND_PLATFORM_SDL_PLATFORM_HPP
#define SNOWFLAKE_RENDERING_BACKEND_PLATFORM_SDL_PLATFORM_HPP

#include "platform_base.hpp"
#include <snowflake/util/portability.hpp>
#include <SDL.h>

namespace snowflake::backend {

/// The SdlPlatform wraps SDL functionality which is common for all platforms
/// which use SDL.
class SdlPlatform : public Platform<SdlPlatform> {
  using WindowPtr    = SDL_Window*;           //!< Window pointer type.
  using BasePlatform = Platform<SdlPlatform>; //!< Base platform type.

 public:
  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to create a default initialized platform.
  SdlPlatform() noexcept;

  /// Constructor to initialize the platform with a \p title, and a \p width and
  /// \p height.
  SdlPlatform(const char* title, uint32_t width, uint32_t height) noexcept;

  // clang-format off
  /// Copy constructor, deleted.
  SdlPlatform(const SdlPlatform&)     = delete;
  /// Move constructor, defaulted.
  SdlPlatform(SdlPlatform&&) noexcept = default;

  /// Destroys SDL.
  ~SdlPlatform() noexcept;

  //==--- [operator overloads] ---------------------------------------------==//

  /// Copy assignment, deleted.
  auto operator=(const SdlPlatform&) -> SdlPlatform&     = delete;
  /// Move assignment, defaulted.
  auto operator=(SdlPlatform&&) noexcept -> SdlPlatform& = default;
  // clang-format on

  //==--- [interface] ------------------------------------------------------==//

  /// Creates a surface for vulkan.
  /// \param instance The instance to create the surface for.
  /// \param device   The device to create the surface from.
  auto create_vulkan_surface(VkInstance instance, VkPhysicalDevice device) const
    noexcept -> VkSurfaceKHR;

  /// Gets the vulkan device extensions for the platform.
  auto get_device_extensions() const noexcept -> ExtVector;

  /// Gets the vulkan instance extensions for the platform.
  auto get_instance_extensions() const noexcept -> ExtVector;

  /// Returns true if the platform is still alive.
  auto is_alive_impl() const noexcept -> bool;

  /// Sets the title of the platform.
  /// \param title The title for the platform.
  auto set_title_impl(const char* title) noexcept -> void;

  /// Resizes the platform surface.
  auto resize_impl() noexcept -> void;

  /// Polls the input.
  /// \todo change this so that the events go somewhere.
  auto poll_input_impl() noexcept -> void;

 private:
  WindowPtr window_   = nullptr; //!< Pointer to the window.
  bool      is_alive_ = true;    //!< If the platform is alive.

  /// Initializes the vulkan loader, returning true if the loading was
  /// successul.
  snowflake_nodiscard auto initialize_vulkan_loader() const noexcept -> bool;

  /// Initializes the platform.
  auto initialize() noexcept -> void;
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_PLATFORM_SDL_PLATFORM_HPP
