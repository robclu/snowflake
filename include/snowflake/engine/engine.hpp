//==--- snowflake/engine/engine.hpp ------------------------ -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  engine.hpp
/// \brief This file defines a engine, which is an interface to all resources.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ENGINE_ENGINE_HPP
#define SNOWFLAKE_ENGINE_ENGINE_HPP

#include "resource_map.hpp"
#include <snowflake/rendering/backend/platform/platform.hpp>
#include <snowflake/rendering/backend/vk/vulkan_driver.hpp>
#include <snowflake/util/portability.hpp>
#include <wrench/memory/allocator.hpp>
#include <wrench/memory/linear_allocator.hpp>

namespace snowflake {

class Renderer;

/**
 * The engine class provides the interface to all system components, as well as
 * managers for the various components for the system.
 */
class Engine {
 public:
  /**
   * Defines the type of the platform.
   * \todo Change this to be a reference counted type.
   */
  using Platform = backend::PlatformType;

  /**
   * Defines the type of the driver.
   */
  using Driver = backend::VulkanDriver;

  /*==--- [construction] ---------------------------------------------------==*/

  /**
   * Creates the engine, returning a reference to the newly created engine. This
   * either creates a valid engine, or terminates.
   * \return A pointer to the engine, or a nullptr if the engine could not be
   *         created.
   */
  snowflake_nodiscard static auto
  create(size_t arena_size = alloc_arena_size) noexcept -> Engine&;

  /*
   * Cleans up the engine resources.
   *
   * \note This __only__ cleans up resources which are allocated internally by
   *       the engine. Any objects created through the `create()` methods
   *       __are not__ cleaned up. They must be explicity destroyed.
   */
  ~Engine() noexcept;

  /*==--- [deleted] --------------------------------------------------------==*/

  // clang-format off
  /**  Copy constructor -- deleted. */
  Engine(const Engine&)         = delete;
  /** Move constructor -- deleted. */
  Engine(Engine&&)              = delete;
  /** Copy assignment -- deleted. */
  auto operator=(const Engine&) = delete;
  /**  Move assignment -- deleted. */
  auto operator=(Engine&&)      = delete;
  // clang-format on

  /*==--- [interface] ------------------------------------------------------==*/

  /**
   * Provides access to the platform.
   * \return A pointer to the platform.
   */
  snowflake_nodiscard auto platform() noexcept -> Platform& {
    return platform_;
  }

  /**
   * Provides const access to the platform.
   * \return A const reference to the platform.
   */
  snowflake_nodiscard auto platform() const noexcept -> const Platform& {
    return platform_;
  }

  /**
   * Provides access to the driver.
   * \return A reference to the driver.
   */
  snowflake_nodiscard auto driver() noexcept -> Driver& {
    return driver_;
  }

  /**
   * Provides access to the driver.
   * \return A const reference to the driver.
   */
  snowflake_nodiscard auto driver() const noexcept -> Driver& {
    return driver_;
  }

  /**
   * Creates the renderer, returning a pointer to it.
   * \return A pointer to the renderer.
   */
  snowflake_nodiscard auto create_renderer() noexcept -> Renderer*;

  /*==--- [destruction] ----------------------------------------------------==*/

  /**
   * Destroys the renderer pointed to by \p renderer.
   * \return __true__ if the \p renderer is destroyed, __false__ otherwise.
   */
  auto destroy(Renderer* renderer) noexcept -> bool;

 private:
  /**
   * Defines the type of the default allocator.
   */
  using DefaultAllocator = wrench::Allocator<wrench::LinearAllocator>;

  /**
   * Defines the type of the container for renderers.
   */
  using RendererMap = ResourceMap<Renderer, wrench::VoidLock>;

  /** Default allocator arena size. */
  static constexpr size_t alloc_arena_size = 2048;

  /*==--- [members] --------------------------------------------------------==*/

  DefaultAllocator allocator_; //!< Default allocator for objects.
  RendererMap      renderers_; //!< Map of renderers created.
  Platform         platform_;  //!< The platform to run on.
  Driver&          driver_;    //!< The driver for engine.

  /*==--- [construction] ---------------------------------------------------==*/

  /**
   * Constructor to create the engine.
   * \param arena_size The size of the arena for the allocations.
   */
  Engine(size_t arena_size) noexcept;

  /**
   * Constructor to create the engine.
   */
  Engine() noexcept;

  /**
   * Cleans up the \p resource from the \p resource_map.
   * \param  resource     The resource to cleanup.
   * \param  resource_map The map to remove from.
   * \tparam Resource     The type of the resource.
   * \tparam Map          The map to remove from.
   * \return __true__ if the resource is removed, otherwise __false__.
   */
  template <typename Resource, typename Map>
  auto cleanup_resource(Resource* resource, Map& resource_map) noexcept -> bool;
};

} // namespace snowflake

#endif // SNOWFLAKE_ENGINE_ENGINE_HPP
