//==--- ripple/glow/src/apple_cocoa_platform.mm ------------ -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  apple_cocoa_platform.mm
/// \brief This file defines the implemenation of a platform for apple which
///        uses cocoa.
//
//==------------------------------------------------------------------------==//

#include <ripple/glow/vk/platform/apple_cocoa_platform.hpp>
#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>
#include <SDL_syswm.h>
#include <cassert>

namespace ripple::glow::vk {

auto AppleCocoaPlatform::get_view_ptr() const {
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  assert(SDL_GetWindowWMInfo(this->window, &wmi));
  NSWindow* win  = wmi.info.cocoa.window;
  NSView*   view = [win contentView];
  return view;
}

auto AppleCocoaPlatform::initialize(const std::string& title) -> void {
  sdl_platform_t::init_window(title, this->_width, this->_height);

  auto view = get_view_ptr();

  // Set up the metal layer to render to:
  [view setWantsLayer:YES];
  CAMetalLayer* metalLayer = [CAMetalLayer layer];
  metalLayer.bounds = view.bounds;

  // It's important to set the drawableSize to the actual backing pixels. When
  // rendering full-screen, we can skip the macOS compositor if the size matches
  // the display size.
  metalLayer.drawableSize = [view convertSizeToBacking:view.bounds.size];

  // This is set to NO by default, but is also important to ensure we can bypass
  // the compositor  in full-screen mode
  // See "Direct to Display" 
  // http://metalkit.org/2017/06/30/introducing-metal-2.html.
  metalLayer.opaque = YES;

  [view setLayer:metalLayer];
}

auto AppleCocoaPlatform::create_vulkan_surface(
  VkInstance instance, VkPhysicalDevice device
) -> VkSurfaceKHR {
  VkSurfaceKHR surface;
  return surface;

/*
  // Get the CAMetalLayer-backed view.
  auto nsview = get_view_ptr();

  // Create the VkSurface.
  assert(vkCreateMacOSSurfaceMVK);
  VkSurfaceKHR                surface    = nullptr;
  VkMacOSSurfaceCreateInfoMVK createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
  createInfo.pView = (__bridge void*)nsview;
  VkResult result  = vkCreateMacOSSurfaceMVK(
    (VkInstance)instance, &createInfo, VKALLOC, &surface);
  assert(result == VK_SUCCESS);

  // The size that we return to VulkanDriver is consistent with what the macOS
  // client sees for the view size, but it's not necessarily consistent with
  // the surface caps currentExtent. We've observed that if the window was
  // initially created on a high DPI display, then dragged to a low DPI
  // display, the VkSurfaceKHR physical caps still have a high resolution,
  // despite the fact that we've recreated it.
  NSSize sz = [nsview convertSizeToBacking: nsview.frame.size];
  *width    = sz.width;
  *height   = sz.height;
  return surface;
*/
}


} // namespace ripple::glow