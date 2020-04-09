//==--- glow/vk/platform/sdl_platform.hpp ------------------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
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

#ifndef RIPPLE_GLOW_VK_PLATFORM_SDL_PLATFORM_HPP
#define RIPPLE_GLOW_VK_PLATFORM_SDL_PLATFORM_HPP

#include "platform_base.hpp"
#include <SDL.h>
#include <string>

namespace ripple::glow::vk {

/// The SdlPlatform wraps SDL functionality which is common for all platforms
/// which use SDL.
class SdlPlatform : public Platform<SdlPlatform> {
  using window_ptr_t    = SDL_Window*;           //!< Window pointer type.
  using base_platform_t = Platform<SdlPlatform>; //!< Base platform type.
  window_ptr_t _window  = nullptr;               //!< Pointer to the window.

 public:
  //==--- [construction] ---------------------------------------------------==//

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

  /// Initializes the platform with a \p title.
  /// \param title The title to initialize the platform with.
  auto initialize(const std::string& title) -> void;

  /// Initializes the vulkan loader, returning true if the loading was
  /// successul.
  ripple_no_discard auto initialize_vulkan_loader() const -> bool;
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_SDL_PLATFORM_HPP