//==--- snowflake/rendering/backend/platform/platform.hpp -- -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform.hpp
/// \brief This file defines an includes the appropriate platform.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_HPP
#define SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_HPP

#if defined(SNOWFLAKE_HEADLESS_PLATFORM)
#else
  #include "sdl_platform.hpp"
#endif

namespace snowflake::backend {

/// Alias for the type of the platform to create. The platform is specific to
/// what we are running on, unless another flag has been defined in the build
/// to specify the type of the platform, such as a headless platform, otherwise
/// we just use the SDL platform since it handles all common platforms.
using PlatformType =
#if defined(SNOWFLAKE_HEADLESS_PLATFORM)
  HeadlessPlatform;
#else
  SdlPlatform;
#endif

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_PLATFORM_PLATFORM_HPP
