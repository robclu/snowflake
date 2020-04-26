//==--- glow/src/engine/engine.cpp ------------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  engine.cpp
/// \brief This file defines the implemenation for the engine.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/engine/engine.hpp>

namespace ripple::glow {

//==--- [public static] ----------------------------------------------------==//

auto Engine::create() -> Engine* {
  static Engine engine;

  return &engine;
}

//==--- [private] ----------------------------------------------------------==//

Engine::Engine() {
  _driver = Driver::create(_platform);
}

} // namespace ripple::glow