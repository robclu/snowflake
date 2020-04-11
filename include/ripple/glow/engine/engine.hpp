//==--- ripple/glow/engine/engine.hpp ---------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  engine.hpp
/// \brief This file defines a engine, which is an interface to glow resources.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_ENGINE_ENGINE_HPP
#define RIPPLE_GLOW_ENGINE_ENGINE_HPP

#include "../backend/platform/platform.hpp"
#include "../backend/vk/vulkan_driver.hpp"
#include <ripple/core/util/portability.hpp>

namespace ripple::glow {

/// The engine class provides the interface to all system components in glow. It
/// holds a platform, and a backend driver.
class Engine {
 public:
  /// Defines the type of the platform.
  /// \todo Change this to be a reference counted type.
  using platform_t = backend::platform_type_t;
  /// Defines the type of the driver.
  using driver_t = backend::VulkanDriver;

  //==--- [construction] ---------------------------------------------------==//

  Engine()              = delete;
  Engine(const Engine&) = delete;
  Engine(Engine&&)      = delete;
  auto operator=(const Engine&) = delete;
  auto operator=(Engine&&) = delete;

  //==--- [interface] ------------------------------------------------------==//

  /// Creates the engine, returning a pointer to the newly created engine. If
  /// the engine could not be created, a nullptr is returned.
  /// \param platform The platform to create the engine for.
  static auto create(const platform_t& platform) -> Engine*;

 private:
  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to create the engine with the \p paltform.
  /// \param platform The platform to create the engine with.
  Engine(const platform_t& platform);

  //==--- [members] --------------------------------------------------------==//

  driver_t _driver; //!< The driver for engine.
};

} // namespace ripple::glow

#endif // RIPPLE_GLOW_ENGINE_ENGINE_HPP
