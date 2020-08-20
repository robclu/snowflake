//==--- snowflake/src/engine/engine.cpp -------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  engine.cpp
/// \brief This file defines the implemenation for the engine.
//
//==------------------------------------------------------------------------==//

#include <snowflake/engine.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake {

//==--- [public static] ----------------------------------------------------==//

auto Engine::create() noexcept -> Engine* {
  static Engine engine;
  return &engine;
}

//==--- [private] ----------------------------------------------------------==//

Engine::Engine() noexcept {
  driver_ = Driver::create(platform_);
}

Engine::~Engine() noexcept {
  driver_->destroy();
}

} // namespace snowflake