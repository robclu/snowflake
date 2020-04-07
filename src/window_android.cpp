//==--- ripple/glow/src/window_android.cpp ----------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window_android.cpp
/// \brief This file defines the implemenation of an Android window.
//
//==------------------------------------------------------------------------==//

#include <SDL_syswm.h>
#include <cassert>
#include <ripple/glow/window/window_android.hpp>

namespace ripple::glow {

auto AndroidWindow::create_native_window() -> void {
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  assert(SDL_GetWindowWMInfo(this->_window, &wmi));
  this->_native_window = static_cast<ANativeWindow*>(wmi.info.android.window);
}

} // namespace ripple::glow