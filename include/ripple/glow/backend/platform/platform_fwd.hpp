//==--- glow/backend/platform/platform_fwd.hpp ------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform_fwd.hpp
/// \brief This file forward declares platforms.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_PLATFORM_FWD_HPP
#define RIPPLE_GLOW_BACKEND_PLATFORM_FWD_HPP

namespace ripple::glow::backend {

//==--- [forward declarations] ---------------------------------------------==//

/// The Platform type defines an interface for platform-specific
/// window-intefration funcitonality.
///
/// This class uses a static interface since the platform is always known at
/// compile time, and thus, even if the cost of virtual functions is small,
/// there is no point in paying for them.
///
/// Anything that is general for all platforms, is implemented here, and
/// additional functionality can be provided in the implementation class.
///
/// \tparam Impl The type of the implementation of the interface.
template <typename Impl>
class Platform;

/// Defines a platform which uses SDL.
class SdlPlatform;
/// Defines a headless platform.
class HeadlessPlatform;

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_PLATFORM_FWD_HPP
