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

#include <SDL.h>
#include <string>

namespace ripple::glow::vk {

/// The SdlPlatform wraps SDL functionality which is common for all platforms
/// which use SDL.
struct SdlPlatform {
  using window_ptr_t  = SDL_Window*; //!< Type of the window pointer.
  window_ptr_t window = nullptr;     //!< Pointer to the window.

  /// Initializes SDL.
  SdlPlatform() {
    assert(SDL_Init(SDL_INIT_EVENTS) == 0);
  }

  /// Initializes the SDL window with a \p title, a \p width, and a \p height.
  /// \param title  The title for the window.
  /// \param width  The width of the window.
  /// \param height The height of the window.
  auto init_window(const std::string& title, uint32_t width, uint32_t height)
    -> void {
    const int center_x = SDL_WINDOWPOS_CENTERED;
    const int center_y = SDL_WINDOWPOS_CENTERED;
    uint32_t  flags    = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

    // If using a paltform with a resizable window:
    if constexpr (GLOW_RESIZABLE_WINDOW) {
      flags |= SDL_WINDOW_RESIZABLE;
    }

    window = SDL_CreateWindow(
      title.c_str(),
      center_x,
      center_y,
      static_cast<int>(width),
      static_cast<int>(height),
      flags);
  }

  /// Deestroys SDL.
  ~SdlPlatform() {
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_SDL_HELPER_HPP