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

#include <snowflake/backend/platform/platform.hpp>
#include <snowflake/backend/vk/vulkan_driver.hpp>

namespace snowflake {

/// The engine class provides the interface to all system components. It
/// holds a platform, and a backend driver, and a renderer.
class Engine {
 public:
  // clang-format off
  /// Defines the type of the platform.
  /// \todo Change this to be a reference counted type.
  using Platform = backend::PlatformType;
  /// Defines the type of the driver.
  using Driver   = backend::VulkanDriver;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Creates the engine, returning a pointer to the newly created engine. If
  /// the engine could not be created, a nullptr is returned.
  static auto create() noexcept -> Engine*;

  /// Destructor -- defaulted.
  ~Engine() noexcept;

  //==--- [deleted] --------------------------------------------------------==//

  // clang-format off
  /// Copy constructor -- deleted.
  Engine(const Engine&)         = delete;
  /// Move constructor -- deleted.
  Engine(Engine&&)              = delete;
  /// Copy assignment -- deleted.
  auto operator=(const Engine&) = delete;
  /// Move assignment -- deleted.
  auto operator=(Engine&&)      = delete;
  // clang-format on

  //==--- [interface] ------------------------------------------------------==//

  /// Returns a pointer to the platform.
  auto platform() noexcept -> Platform* {
    return &platform_;
  }

  /// Returns a const pointer to the platform.
  auto platform() const noexcept -> const Platform* {
    return &platform_;
  }

  /// Returns a pointer to the driver.
  auto driver() noexcept -> Driver* {
    return driver_;
  }

 private:
  //==--- [construction] ---------------------------------------------------==//

  /// Constructor to create the engine.
  Engine() noexcept;

  //==--- [members] --------------------------------------------------------==//

  Platform platform_;         //!< The platform to run on.
  Driver*  driver_ = nullptr; //!< The driver for engine.
};

} // namespace snowflake

#endif // SNOWFLAKE_ENGINE_ENGINE_HPP
