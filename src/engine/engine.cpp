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

#include <snowflake/engine/engine.hpp>
#include <snowflake/rendering/renderer.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake {

/*==--- [construction] -----------------------------------------------------==*/

auto Engine::create(size_t arena_size) noexcept -> Engine& {
  static Engine engine(arena_size);
  return engine;
}

Engine::Engine(size_t arena_size) noexcept
: allocator_{arena_size}, platform_{}, driver_{Driver::create(platform_)} {}

Engine::Engine() noexcept
: allocator_{alloc_arena_size},
  platform_{},
  driver_{Driver::create(platform_)} {}

Engine::~Engine() noexcept {
  driver_.destroy();

  // Check that no resources have been leaked:
  if constexpr (wrench::Log::would_log<wrench::LogLevel::debug>()) {
    for (auto* renderer : renderers_) {
      wrench::log_debug(
        "Engine destroyed before renderer {} created by it is destroyed!",
        fmt::ptr(renderer));
    }
  }
}

/*==--- [creation] ---------------------------------------------------------==*/

auto Engine::create_renderer() noexcept -> Renderer* {
  Renderer* renderer = allocator_.create<Renderer>(*this);
  if (renderer != nullptr) {
    renderers_.insert(renderer);
    renderer->init();
  }
  return renderer;
}

/*==--- [destruction] ------------------------------------------------------==*/

template <typename Resource, typename Map>
auto Engine::cleanup_resource(Resource* resource, Map& map) noexcept -> bool {
  if (resource == nullptr) {
    return true;
  }
  bool erased = map.erase(resource);
  // If in the map, we need to:
  //  - Call the destruction method for the resource
  //  - Recycle the allocation of the resource from the allocator
  if (erased) {
    resource->destroy();
    allocator_.recycle(resource);
  }
  return erased;
}

auto Engine::destroy(Renderer* renderer) noexcept -> bool {
  return cleanup_resource(renderer, renderers_);
}

} // namespace snowflake