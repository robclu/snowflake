//==--- snowflake/window/window.hpp ------------------------ -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window.hpp
/// \brief This file defines a window.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_WINDOW_WINDOW_HPP
#define SNOWFLAKE_WINDOW_WINDOW_HPP

#include <snowflake/engine.hpp>
#include <snowflake/backend/platform/platform.hpp>
#include <snowflake/util/portability.hpp>

namespace snowflake {

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
    EnginePtr   engine,
    const char* title,
    uint32_t    width,
    uint32_t    height) noexcept;

  // clang-format off
  /// Defaulted destructor.
  ~Window() noexcept        = default;
  /// Move constructor.
  Window(Window&&) noexcept = default;

  //==--- [deleted] --------------------------------------------------------==//

  /// Deleted copy constructor -- can't copy window.
  Window(const Window&)         = delete;
  /// Deleted copy assignment operator -- can't copy window.
  auto operator=(const Window&) = delete;
  // clang-format on

  //===--- [operator overloads] --------------------------------------------==//

  /// Move assignment operator.
  auto operator=(Window&&) noexcept -> Window& = default;

  //==--- [interface] ------------------------------------------------------==//

  /// Returns true of the window is alive.
  snowflake_nodiscard auto is_alive() const noexcept -> bool;

  /// Polls the event loop of the window, returning an event.
  auto poll_input() noexcept -> void;

 private:
  EnginePtr engine_; //!< Pointer to the engine.

  /// Initializes the window by creating the engine and the initializing the
  /// engine platform.  Returns true if everything was created successfully.
  /// \param title  The title of the window.
  /// \param width  The (width) of the window  - pixels in the x dimension.
  /// \param height The (height) of the window - pixels in the y dimension.
  auto
  init(const char* title, uint32_t width, uint32_t height) noexcept -> bool;
};

} // namespace snowflake

#endif // SNOWFLAKE_WINDOW_WINDOW_HPP
