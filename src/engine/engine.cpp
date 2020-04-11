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

#include <ripple/glow/engine/engine.hpp>

namespace ripple::glow {

//==--- [public static] ----------------------------------------------------==//

auto Engine::create(const Engine::platform_t& platform) -> Engine* {
  static Engine engine(platform);
  return &engine;
}

//==--- [private] ----------------------------------------------------------==//

Engine::Engine(const Engine::platform_t& platform) : _driver(platform) {}

} // namespace ripple::glow