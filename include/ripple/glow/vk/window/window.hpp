//==--- ripple/glow/vk/window/window.hpp ------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window.hpp
/// \brief This file defines a window.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_WINDOW_WINDOW_BASE_HPP
#define RIPPLE_GLOW_VK_WINDOW_WINDOW_BASE_HPP

#include "../platform/platform.hpp"

namespace ripple::glow::vk {

/// The Window type defines the generic functionality for a window.
class Window {
  using platform_t = platform_type_t; //!< Defines the type of the platform.

 public:
  //==--- [construction] ---------------------------------------------------==//

  /// Constructor for the window, which sets the \p title and the \p x and \p y
  /// dimensions of the window.
  /// \param title  The title of the window.
  /// \param width  The (width) of the window  - pixels in the x dimension.
  /// \param height The (height) of the window - pixels in the y dimension.
  Window(const std::string& title, uint32_t width, uint32_t height)
  : _platform(width, height) {
    _platform.init(title);
  }

  ~Window()             = default; //!< Default destructor.
  Window(const Window&) = delete;  //!< Delector copy constructor.
  Window(Window&&)      = delete;  //!< Deleted move constructor.

  //===--- [operator overloads] --------------------------------------------==//

  auto operator=(const Window&) = delete; //!< Deleted copy assignment operaotr.
  auto operator=(Window&&) = delete;      //!< Deleted move assignment operator.

 private:
  platform_t _platform; //!< The platform with the native window.
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_WINDOW_WINDOW_BASE_HPP