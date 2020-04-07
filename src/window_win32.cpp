//==--- ripple/glow/src/window_win32.cpp ------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window_win32.cpp
/// \brief This file defines the implemenation of a windows window.
//
//==------------------------------------------------------------------------==//

#include <SDL_syswm.h>
#include <cassert>
#include <ripple/glow/window/window_win32.hpp>

namespace ripple::glow {

auto Win32Window::create_native_window() -> void {
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  assert(SDL_GetWindowWMInfo(this->_window, &wmi));
  this->_native_window = static_cast<HWND>(wmi.info.win.window);
}

} // namespace ripple::glow