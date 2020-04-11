//==--- glow/backend/platform/platform.hpp ----------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
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

#ifndef RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_HPP
#define RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_HPP

#if defined(GLOW_HEADLESS_PLATFORM)
#else
  #include "sdl_platform.hpp"
#endif

namespace ripple::glow::backend {

/// Alias for the type of the platform to create. The platform is specific to
/// what we are running on, unless another flag has been defined in the build
/// to specify the type of the platform, such as a headless platform, otherwise
/// we just use the SDL platform since it handles all common platforms.
using platform_type_t =
#if defined(GLOW_HEADLESS_PLATFORM)
  HeadlessPlatform;
#else
  SdlPlatform;
#endif

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_PLATFORM_PLATFORM_HPP