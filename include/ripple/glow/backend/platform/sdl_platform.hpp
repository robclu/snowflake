//==--- glow/backend/platform/sdl_platform.hpp ------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
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

#ifndef RIPPLE_GLOW_BACKEND_PLATFORM_SDL_PLATFORM_HPP
#define RIPPLE_GLOW_BACKEND_PLATFORM_SDL_PLATFORM_HPP

#include "platform_base.hpp"
#include <SDL.h>
#include <string>

namespace ripple::glow::backend {

/// The SdlPlatform wraps SDL functionality which is common for all platforms
/// which use SDL.
class SdlPlatform : public Platform<SdlPlatform> {
  using window_ptr_t    = SDL_Window*;           //!< Window pointer type.
  using base_platform_t = Platform<SdlPlatform>; //!< Base platform type.
  window_ptr_t _window  = nullptr;               //!< Pointer to the window.

 public:
  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to create a default initialized platform.
  SdlPlatform();

  /// Constructor to initialize the platform with a \p title, and a \p width and
  /// \p height.
  SdlPlatform(const std::string& title, uint32_t width, uint32_t height);

  // clang-format off

  /// Copy constructor, deleted.
  SdlPlatform(const SdlPlatform&)     = delete;
  /// Move constructor, defaulted.
  SdlPlatform(SdlPlatform&&) noexcept = default;

  /// Destroys SDL.
  ~SdlPlatform();

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
  auto create_vulkan_surface(VkInstance instance, VkPhysicalDevice device)
    -> VkSurfaceKHR;

  /// Gets the vulkan device extensions for the platform.
  auto get_device_extensions() const -> ext_vector_t;

  /// Gets the vulkan instance extensions for the platform.
  auto get_instance_extensions() const -> ext_vector_t;

  /// Returns true if the platform is still alive.
  auto is_alive_impl() const -> bool;

  /// Sets the title of the platform.
  /// \param title The title for the platform.
  auto set_title_impl(const std::string& title) -> void;

  /// Resizes the platform surface.
  auto resize_impl() -> void;

  /// Polls the input.
  /// \todo change this so that the events go somewhere.
  auto poll_input_impl() -> void;

 private:
  bool _is_alive = true; //!< If the platform is alive.

  /// Initializes the vulkan loader, returning true if the loading was
  /// successul.
  ripple_no_discard auto initialize_vulkan_loader() const -> bool;

  /// Initializes the platform.
  auto initialize() -> void;
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_VK_PLATFORM_SDL_PLATFORM_HPP