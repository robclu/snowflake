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

#ifndef RIPPLE_GLOW_WINDOW_WINDOW_HPP
#define RIPPLE_GLOW_WINDOW_WINDOW_HPP

#include "../engine/engine.hpp"
#include "../backend/platform/platform.hpp"
#include <ripple/core/util/portability.hpp>

namespace ripple::glow {

/// The Window type defines the generic functionality for a window. This can
/// be a native window for the platform, which is the _common_ case, in which
/// case rendering will per performed on the native window, but it could also
/// be rendering to a swapchain which is not displayed, such as in _headless_
/// mode.
class Window {
 public:
  /// Defines the type of the driver pointer.
  using EnginePtr = Engine*;

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor for the window, which sets the \p title and the \p width and
  /// \p height as the dimensions of the window.
  /// \param engine A pointer to the engine to run the window.
  /// \param title  The title of the window.
  /// \param width  The (width) of the window  - pixels in the x dimension.
  /// \param height The (height) of the window - pixels in the y dimension.
  Window(
    EnginePtr          engine,
    const std::string& title,
    uint32_t           width,
    uint32_t           height);

  /// Defaulted destructor.
  ~Window() = default;
  /// Move constructor.
  Window(Window&&) = default;

  //==--- [deleted] --------------------------------------------------------==//

  /// Deleted copy constructor -- can't copy window.
  Window(const Window&) = delete;
  /// Deleted copy assignment operator -- can't copy window.
  auto operator=(const Window&) -> Window& = delete;

  //===--- [operator overloads] --------------------------------------------==//

  /// Move assignment operator.
  auto operator=(Window&&) -> Window& = default;

  //==--- [interface] ------------------------------------------------------==//

  /// Returns true of the window is alive.
  ripple_no_discard auto is_alive() const -> bool;

  /// Polls the event loop of the window, returning an event.
  auto poll_input() -> void;

 private:
  EnginePtr _engine; //!< Pointer to the engine.

  /// Initializes the window by creating the engine and the initializing the
  /// engine platform.  Returns true if everything was created successfully.
  /// \param title  The title of the window.
  /// \param width  The (width) of the window  - pixels in the x dimension.
  /// \param height The (height) of the window - pixels in the y dimension.
  auto init(const std::string& title, uint32_t width, uint32_t height) -> bool;
};

} // namespace ripple::glow

#endif // RIPPLE_GLOW_WINDOW_WINDOW_HPP