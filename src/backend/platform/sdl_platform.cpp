//==--- glow/src/vk/platform/sdl_platform.cpp -------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  sdl_paltform.cpp
/// \brief This file defines the implemenation of a platform which uses SDL.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/backend/platform/sdl_platform.hpp>
#include <ripple/glow/backend/vk/vulkan_context.hpp>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>
#include <cassert>

namespace ripple::glow::backend {

//==--- [con/destruction] --------------------------------------------------==//

SdlPlatform::SdlPlatform() {
  initialize();
}

SdlPlatform::SdlPlatform(
  const std::string& title, uint32_t width, uint32_t height)
: base_platform_t(width, height) {
  initialize();
  set_title(title);
}

SdlPlatform::~SdlPlatform() {
  SDL_DestroyWindow(_window);
  SDL_Quit();
}

//==--- [interface] --------------------------------------------------------==//

auto SdlPlatform::create_vulkan_surface(
  VkInstance instance, VkPhysicalDevice device) const -> VkSurfaceKHR {
  VkSurfaceKHR surface;
  if (SDL_Vulkan_CreateSurface(_window, instance, &surface)) {
    return surface;
  } else {
    return VK_NULL_HANDLE;
  }
}

auto SdlPlatform::is_alive_impl() const -> bool {
  return _is_alive;

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: {
        return false;
      }
    }
  }
  return true;
}
auto SdlPlatform::get_device_extensions() const -> ext_vector_t {
  return ext_vector_t{"VK_KHR_swapchain"};
}

auto SdlPlatform::get_instance_extensions() const -> ext_vector_t {
  unsigned num_ins_exts = 0;
  SDL_Vulkan_GetInstanceExtensions(_window, &num_ins_exts, nullptr);
  auto instance_names = ext_vector_t(num_ins_exts);
  SDL_Vulkan_GetInstanceExtensions(
    _window, &num_ins_exts, instance_names.data());
  return instance_names;
}

auto SdlPlatform::poll_input_impl() -> void {
  // Todo : Add input polling ...
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: {
        _is_alive = false;
        break;
      }
      default: break;
    }
  }
}

auto SdlPlatform::resize_impl() -> void {
  SDL_SetWindowSize(
    _window, static_cast<int>(_width), static_cast<int>(_height));
}

auto SdlPlatform::set_title_impl(const std::string& title) -> void {
  if (_window == nullptr) {
    initialize();
  }
  SDL_SetWindowTitle(_window, title.c_str());
}

//==--- [initialization] ---------------------------------------------------==//

static bool loader_initialized = false;

auto SdlPlatform::initialize() -> void {
  if (!loader_initialized && !SdlPlatform::initialize_vulkan_loader()) {
    assert(false && "Failed to load the vulkan loader!");
  }

  if (_window) {
    return;
  }

  assert(SDL_Init(SDL_INIT_EVENTS) == 0);

  const int center_x = SDL_WINDOWPOS_CENTERED;
  const int center_y = SDL_WINDOWPOS_CENTERED;
  uint32_t  flags =
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN;

  // If using a paltform with a resizable window:
  if constexpr (GLOW_RESIZABLE_WINDOW) {
    flags |= SDL_WINDOW_RESIZABLE;
  }

  _window = SDL_CreateWindow(
    "Default Initialized Window",
    center_x,
    center_y,
    static_cast<int>(this->_width),
    static_cast<int>(this->_height),
    flags);

  if (_window == nullptr) {
    log_error("Failed to create SDL window.");
  }
}

/// Initializes the vulkan loader, returning true if the loading was
/// successul.
auto SdlPlatform::initialize_vulkan_loader() const -> bool {
  if (!VulkanContext::init_loader(
        (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr())) {
    log_error("Sdl platform failed to create Vulkan loader.");
    return false;
  }
  loader_initialized = true;
  return true;
}

} // namespace ripple::glow::backend