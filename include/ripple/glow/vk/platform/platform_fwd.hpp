//==--- glow/vk/platform/platform_fwd.hpp ------------------ -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  platform_fwd.hpp
/// \brief This file forward declares platforms.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_PLATFORM_FWD_HPP
#define RIPPLE_GLOW_VK_PLATFORM_FWD_HPP

namespace ripple::glow::vk {

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

/// Defines a platform for osx.
class AppleCocoaPlatform;
/// Defines a platform for windows.
class WindowsPlatform;
/// Defines a platform for android.
class AndroidPlatform;
/// Defines a platform for linux.
class LinuxPlatform;
/// Defines a headless platform.
class HeadlessPlatform;

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_PLATFORM_FWD_HPP
