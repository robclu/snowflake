//==--- src/rendering/backend/vk/platform/sdl_platform.cpp - -*- C++ -*- ---==//
//
//                            Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  sdl_paltform.cpp
/// \brief This file defines the implemenation of a platform which uses SDL.
//
//==------------------------------------------------------------------------==//

#include <snowflake/rendering/backend/platform/sdl_platform.hpp>
#include <snowflake/rendering/backend/vk/vulkan_context.hpp>
#include <wrench/log/logger.hpp>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>
#include <cassert>

namespace snowflake::backend {

//==--- [con/destruction] --------------------------------------------------==//

SdlPlatform::SdlPlatform() noexcept {
  initialize();
}

SdlPlatform::SdlPlatform(
  const char* title, uint32_t width, uint32_t height) noexcept
: BasePlatform(width, height) {
  initialize();
  set_title(title);
}

SdlPlatform::~SdlPlatform() noexcept {
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

//==--- [interface] --------------------------------------------------------==//

auto SdlPlatform::create_vulkan_surface(
  VkInstance instance, VkPhysicalDevice device) const noexcept -> VkSurfaceKHR {
  VkSurfaceKHR surface;
  if (SDL_Vulkan_CreateSurface(window_, instance, &surface)) {
    return surface;
  }
  return VK_NULL_HANDLE;
}

auto SdlPlatform::is_alive_impl() const noexcept -> bool {
  return is_alive_;

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
auto SdlPlatform::get_device_extensions() const noexcept -> ExtVector {
  return ExtVector{"VK_KHR_swapchain"};
}

auto SdlPlatform::get_instance_extensions() const noexcept -> ExtVector {
  unsigned num_ins_exts = 0;
  SDL_Vulkan_GetInstanceExtensions(window_, &num_ins_exts, nullptr);
  auto instance_names = ExtVector(num_ins_exts);
  SDL_Vulkan_GetInstanceExtensions(
    window_, &num_ins_exts, instance_names.data());
  return instance_names;
}

auto SdlPlatform::poll_input_impl() noexcept -> void {
  // Todo : Add input polling ...
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: {
        is_alive_ = false;
        break;
      }
      default: break;
    }
  }
}

auto SdlPlatform::resize_impl() noexcept -> void {
  SDL_SetWindowSize(window_, static_cast<int>(width), static_cast<int>(height));
}

auto SdlPlatform::set_title_impl(const char* title) noexcept -> void {
  if (window_ == nullptr) {
    initialize();
  }
  SDL_SetWindowTitle(window_, title);
}

//==--- [initialization] ---------------------------------------------------==//

static bool loader_initialized = false;

auto SdlPlatform::initialize() noexcept -> void {
  if (!loader_initialized && !SdlPlatform::initialize_vulkan_loader()) {
    assert(false && "Failed to load the vulkan loader!");
  }

  if (window_) {
    return;
  }

  assert(SDL_Init(SDL_INIT_EVENTS) == 0);

  const int center_x = SDL_WINDOWPOS_CENTERED;
  const int center_y = SDL_WINDOWPOS_CENTERED;
  uint32_t  flags =
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN;

  // If using a paltform with a resizable window:
  if constexpr (SNOWFLAKE_RESIZABLE_WINDOW) {
    flags |= SDL_WINDOW_RESIZABLE;
  }

  window_ = SDL_CreateWindow(
    "Default Initialized Window",
    center_x,
    center_y,
    static_cast<int>(width),
    static_cast<int>(height),
    flags);

  if (window_ == nullptr) {
    wrench::log_error("Failed to create SDL window.");
  }
}

/// Initializes the vulkan loader, returning true if the loading was
/// successul.
auto SdlPlatform::initialize_vulkan_loader() const noexcept -> bool {
  if (!VulkanContext::init_loader(
        (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr())) {
    wrench::log_error("Sdl platform failed to create Vulkan loader.");
    return false;
  }
  loader_initialized = true;
  return true;
}

} // namespace snowflake::backend