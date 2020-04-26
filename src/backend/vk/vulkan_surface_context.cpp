//==--- glow/src/backend/vk/vulkan_surface_context.cpp ----- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_surface_context.cpp
/// \brief This file defines the implemenation for the vulkan surface context.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/backend/vk/vulkan_surface_context.hpp>

namespace ripple::glow::backend {

namespace {

static inline auto
format_to_aspect_mask(VkFormat format) -> VkImageAspectFlags {
  // clang-format off
  switch (format) {
    case VK_FORMAT_UNDEFINED          : return 0;
    case VK_FORMAT_S8_UINT            : return VK_IMAGE_ASPECT_STENCIL_BIT;
    case VK_FORMAT_D16_UNORM          :
    case VK_FORMAT_D32_SFLOAT         :
    case VK_FORMAT_X8_D24_UNORM_PACK32: return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_D16_UNORM_S8_UINT  :
    case VK_FORMAT_D24_UNORM_S8_UINT  :
    case VK_FORMAT_D32_SFLOAT_S8_UINT : return VK_IMAGE_ASPECT_STENCIL_BIT 
                                      | VK_IMAGE_ASPECT_DEPTH_BIT;
    default                           : return VK_IMAGE_ASPECT_COLOR_BIT;
  }
  // clang-format on
}

} // namespace

auto VulkanSurfaceContext::init(
  const VulkanContext& context,
  PresentMode          present_mode,
  uint32_t             width,
  uint32_t             height) -> bool {
  if (_surface == VK_NULL_HANDLE) {
    log_error("Can't initialize surface context until surface is set.");
    return false;
  }

  _present_mode = present_mode;

  if (!init_swapchain(context, width, height)) {
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::present(
  const VulkanContext& context, std::atomic_uint32_t& fence) -> bool {
  if (_done_rendering == VK_NULL_HANDLE) {
    return false;
  }

  while (fence.load(std::memory_order_relaxed) > 0) {
    // Wait for any outstanding work on other threads ...
  }

  VkResult         result = VK_SUCCESS;
  VkPresentInfoKHR info   = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores    = &_done_rendering;
  info.swapchainCount     = 1;
  info.pSwapchains        = &_swapchain;
  info.pImageIndices      = &_current_swap_idx;
  info.pResults           = &result;

  auto submit_result =
    context.device_table()->vkQueuePresentKHR(context.graphics_queue(), &info);

#ifdef ANDROID
  // clang-format off
  // Because of the pre-transform, Android might return sub-optimal, which we
  // treat as a success.
  if (submit_result == VK_SUBOPTIMAL_KHR) { submit_result = VK_SUCCESS; }
  if (result        == VK_SUBOPTIMAL_KHR) { result        = VK_SUCCESS; }
  // clang-format on
#endif

  if (submit_result != VK_SUCCESS || result != VK_SUCCESS) {
    log_error("Failed to present to queue.");
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::reinit(
  const VulkanContext& context,
  PresentMode          present_mode,
  uint32_t             width,
  uint32_t             height) -> bool {
  _present_mode = present_mode;
  create_extent(context, width, height);
  set_present_mode();
  set_num_swapchain_images();

  if (!create_swapchain(context)) {
    return false;
  }

  if (!create_images(context)) {
    return false;
  }

  if (!create_image_views(context)) {
    return false;
  }

  return true;
}

//==--- [private] ----------------------------------------------------------==//

auto VulkanSurfaceContext::create_surface_caps(const VulkanContext& context)
  -> bool {
  VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
  surface_info.surface = _surface;

  //==--- [get surface caps]
  auto phy_device = context.physical_device();
  auto result     = VK_SUCCESS;
  if (context.supports_surface_caps_2()) {
    VkSurfaceCapabilities2KHR surface_caps_2 = {
      VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};

    result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(
      phy_device, &surface_info, &surface_caps_2);
    if (result != VK_SUCCESS) {
      return false;
    }

    _surface_caps = surface_caps_2.surfaceCapabilities;
  } else {
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      phy_device, _surface, &_surface_caps);
    if (result != VK_SUCCESS) {
      return false;
    }
  }

  if (!create_surface_formats(context, surface_info)) {
    return false;
  }

  return true;
}

auto VulkanSurfaceContext::create_surface_formats(
  const VulkanContext& context, VkPhysicalDeviceSurfaceInfo2KHR& surface_info)
  -> bool {
  auto format_count = uint32_t{0};
  auto device       = context.physical_device();
  if (context.supports_surface_caps_2()) {
    auto result = vkGetPhysicalDeviceSurfaceFormats2KHR(
      device, &surface_info, &format_count, nullptr);
    if (result != VK_SUCCESS) {
      return false;
    }

    std::vector<VkSurfaceFormat2KHR> formats2(format_count);
    for (auto& f : formats2) {
      f = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
    }

    result = vkGetPhysicalDeviceSurfaceFormats2KHR(
      device, &surface_info, &format_count, formats2.data());
    if (result != VK_SUCCESS) {
      return false;
    }

    _formats.reserve(format_count);
    for (auto& f : formats2) {
      _formats.push_back(f.surfaceFormat);
    }
  } else {
    auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      device, _surface, &format_count, nullptr);
    if (result != VK_SUCCESS) {
      return false;
    }
    _formats.resize(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      device, _surface, &format_count, _formats.data());

    if (result != VK_SUCCESS) {
      return false;
    }
  }
  return true;
}

auto VulkanSurfaceContext::create_surface_format(const VulkanContext& context)
  -> bool {
  const auto format_count = _formats.size();
  if (format_count == 0) {
    log_error("Surface has no formats, can't create swapchain.");
    return false;
  }

  // Only 1 undefined format, just choose it to be the default:
  if (format_count == 1 && _formats[0].format == VK_FORMAT_UNDEFINED) {
    _surface_format        = _formats[0];
    _surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    return true;
  }

  // Have multiple, look for something that we want:
  for (const auto& format : _formats) {
    // Look for SRGB if that's what we want:
    if (_srgb_enabled) {
      if (
        format.format == VK_FORMAT_R8G8B8A8_SRGB ||
        format.format == VK_FORMAT_B8G8R8A8_SRGB ||
        format.format == VK_FORMAT_A8B8G8R8_SRGB_PACK32) {
        _surface_format = format;
        return true;
      }
      continue;
    }
    // Otherwise just look for the default unorm:
    if (
      format.format == VK_FORMAT_R8G8B8A8_UNORM ||
      format.format == VK_FORMAT_B8G8R8A8_UNORM ||
      format.format == VK_FORMAT_A8B8G8R8_UNORM_PACK32) {
      _surface_format = format;
      return true;
    }
  }

  // Haven't found one that we want, choose the first:
  _surface_format = _formats[0];
  return true;
}

auto VulkanSurfaceContext::set_surface_transform(const VulkanContext& context)
  -> void {
  // clang-format off
  constexpr auto transform_names = std::array<const char*, 9>{
    "IDENTITY_BIT_KHR",
    "ROTATE_90_BIT_KHR",
    "ROTATE_180_BIT_KHR",
    "ROTATE_270_BIT_KHR",
    "HORIZONTAL_MIRROR_BIT_KHR",
    "HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR",
    "HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR",
    "HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR",
    "INHERIT_BIT_KHR",
  };
  // clang-format on

  log_info(
    "Current surface transform is 0x{0:x}",
    static_cast<unsigned>(_surface_caps.currentTransform));

  for (unsigned i = 0; i <= transform_names.size(); ++i) {
    if (_surface_caps.supportedTransforms & (1u << i)) {
      log_info("Supported transform 0x{0:x}: {1}", 1u << i, transform_names[i]);
    }
  }

  VkSurfaceTransformFlagBitsKHR pre_transform;
  const auto has_id_bit = (_surface_caps.supportedTransforms &
                           VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0;
  if (!_prerotate_enabled && has_id_bit) {
    pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    pre_transform = _surface_caps.currentTransform;
  }

  if (pre_transform != _surface_caps.currentTransform) {
    log_warn(
      "Surface transform (0x{0:x}) does not match current transform (0x{1:x}) "
      " Might get performance penalty",
      static_cast<unsigned>(pre_transform),
      static_cast<unsigned>(_surface_caps.currentTransform));
  }

  _surface_transform = pre_transform;
}

auto VulkanSurfaceContext::create_extent(
  const VulkanContext& context, uint32_t width, uint32_t height) -> void {
  set_surface_transform(context);

  log_info(
    "Swapchain current extent: {0} x {1}",
    static_cast<int>(_surface_caps.currentExtent.width),
    static_cast<int>(_surface_caps.currentExtent.height));

  // TODO: Try to match the swapchain size up with what we expect.

  // If we are using pre-rotate of 90 or 270 degrees, we need to flip width and
  // height.
  const auto mask = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR |
                    VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR |
                    VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR |
                    VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR;
  if (_surface_transform & mask) {
    std::swap(width, height);
  }

  // Clamp the target width, height to boundaries.
  const auto& ext_max    = _surface_caps.maxImageExtent;
  const auto& ext_min    = _surface_caps.minImageExtent;
  const auto  w_min      = std::min(width, ext_max.width);
  const auto  h_min      = std::min(height, ext_max.height);
  _swapchain_size.width  = std::max(w_min, ext_min.width);
  _swapchain_size.height = std::max(h_min, ext_min.height);
}

auto VulkanSurfaceContext::create_present_modes(const VulkanContext& context)
  -> bool {
  auto num_present_modes = uint32_t{0};
  auto phy_dev           = context.physical_device();
  auto result            = vkGetPhysicalDeviceSurfacePresentModesKHR(
    phy_dev, _surface, &num_present_modes, nullptr);
  if (result != VK_SUCCESS) {
    log_error("Failed to get number of present modes for surface context.");
    return false;
  }

  _present_modes.resize(num_present_modes);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    phy_dev, _surface, &num_present_modes, _present_modes.data());
  if (result != VK_SUCCESS) {
    log_error("Failed to create present modes for surface context.");
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::set_present_mode() -> void {
  _swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  if (_present_mode == PresentMode::sync_to_vblank) {
    return;
  }

  // Try the others:
  const bool allow_mlbox = _present_mode != PresentMode::force_tear;
  const bool allow_immed = _present_mode != PresentMode::no_tear;
  for (auto& present_mode : _present_modes) {
    if (
      (allow_immed && present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) ||
      (allow_mlbox && present_mode == VK_PRESENT_MODE_MAILBOX_KHR)) {
      _swapchain_present_mode = present_mode;
      return;
    }
  }
}

auto VulkanSurfaceContext::set_num_swapchain_images() -> void {
  // Already found the number of images, or set somewhere else.
  if (_num_images > 0) {
    return;
  }

  // The general advice is to require one more than the minimum swap chain
  // length, since the absolute minimum could easily require waiting for a
  // driver or presentation layer to release the previous frame's buffer. The
  // only situation in which we'd ask for the minimum length is when using a
  // MAILBOX presentation strategy for low-latency situations where tearing is
  // acceptable.
  const uint32_t max_count = _surface_caps.maxImageCount;
  const uint32_t min_count = _surface_caps.minImageCount;

  _num_images = min_count + (_present_mode != PresentMode::no_tear ? 1 : 0);

  // According to section 30.5 of VK 1.1, maxImageCount of zero means "that
  // there is no limit on the number of images, though there may be limits
  // related to the total amount of memory used by presentable images."
  if (max_count != 0 && _num_images > max_count) {
    log_error("Swap chain does not support {} images.", _num_images);
    _num_images = min_count;
  }

  log_info("Using {} swapchain images.", _num_images);
}

auto VulkanSurfaceContext::get_composite_mode() const
  -> VkCompositeAlphaFlagBitsKHR {
  VkCompositeAlphaFlagBitsKHR composite_mode =
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  const auto& _supported_comp_alpha = _surface_caps.supportedCompositeAlpha;

  if (_supported_comp_alpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
    composite_mode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
  } else if (
    _supported_comp_alpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
    composite_mode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  } else if (_supported_comp_alpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  } else if (_supported_comp_alpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    composite_mode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  }
  return composite_mode;
}

auto VulkanSurfaceContext::create_swapchain(const VulkanContext& context)
  -> bool {
  VkSwapchainKHR old_swapchain = _swapchain;

  // clang-format off
  VkSwapchainCreateInfoKHR info = {
    .sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface            = _surface,
    .minImageCount      = _num_images,
    .imageFormat        = _surface_format.format,
    .imageColorSpace    = _surface_format.colorSpace,
    .imageExtent.width  = _swapchain_size.width,
    .imageExtent.height = _swapchain_size.height,
    .imageArrayLayers   = 1,
    .imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform       = _surface_transform,
    .compositeAlpha     = get_composite_mode(),
    .presentMode        = _swapchain_present_mode,
    .clipped            = VK_TRUE,
    .oldSwapchain       = old_swapchain};
  // clang-format on

  auto result = context.device_table()->vkCreateSwapchainKHR(
    context.device(), &info, nullptr, &_swapchain);

  if (old_swapchain != VK_NULL_HANDLE) {
    context.device_table()->vkDestroySwapchainKHR(
      context.device(), old_swapchain, nullptr);
  }

  if (result != VK_SUCCESS) {
    log_error(
      "Failed to create swapchain: (code : {})", static_cast<int>(result));
    _swapchain = VK_NULL_HANDLE;
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::create_images(const VulkanContext& context) -> bool {
  uint32_t image_count = 0;
  auto     result      = context.device_table()->vkGetSwapchainImagesKHR(
    context.device(), _swapchain, &image_count, nullptr);
  if (result != VK_SUCCESS) {
    log_error("Failed to get image count for swapchain.");
    return false;
  }

  _swap_contexts.resize(image_count);
  std::vector<VkImage> images(image_count);
  result = context.device_table()->vkGetSwapchainImagesKHR(
    context.device(), _swapchain, &image_count, images.data());
  if (result != VK_SUCCESS) {
    log_error("Failed to create swapchain image data.");
    return false;
  }

  // clang-format off
  for (size_t i = 0; i < images.size(); ++i) {
    _swap_contexts[i].attachment = {
      .format     = _surface_format.format,
      .image      = images[i],
      .image_view = {},
      .memory     = {}};
  }
  // clang-format on

  return true;
}

auto VulkanSurfaceContext::create_image_views(const VulkanContext& context)
  -> bool {
  VkImageViewCreateInfo info = {};
  info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

  info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  info.format                          = _surface_format.format;
  info.components.r                    = VK_COMPONENT_SWIZZLE_R;
  info.components.g                    = VK_COMPONENT_SWIZZLE_G;
  info.components.b                    = VK_COMPONENT_SWIZZLE_B;
  info.components.a                    = VK_COMPONENT_SWIZZLE_A;
  info.subresourceRange.baseMipLevel   = 0;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.levelCount     = 1;
  info.subresourceRange.layerCount     = 1;
  info.subresourceRange.aspectMask =
    format_to_aspect_mask(_surface_format.format);

  for (auto& swap_context : _swap_contexts) {
    info.image  = swap_context.attachment.image;
    auto result = context.device_table()->vkCreateImageView(
      context.device(), &info, nullptr, &swap_context.attachment.image_view);

    if (result != VK_SUCCESS) {
      log_error("Failed to create image view for attachement!");
    }
  }

  return true;
}

auto VulkanSurfaceContext::create_semaphores(const VulkanContext& context)
  -> bool {
  VkSemaphoreCreateInfo create_info = {};
  create_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  auto result = context.device_table()->vkCreateSemaphore(
    context.device(), &create_info, nullptr, &_image_available);
  if (result != VK_SUCCESS) {
    log_error("Failed to create image available semaphore.");
    return false;
  }

  result = context.device_table()->vkCreateSemaphore(
    context.device(), &create_info, nullptr, &_done_rendering);
  if (result != VK_SUCCESS) {
    log_error("Failed to create done rendering semaphore.");
    return false;
  }

  return true;
}

auto VulkanSurfaceContext::init_swapchain(
  const VulkanContext& context, uint32_t width, uint32_t height) -> bool {
  if (!create_surface_caps(context)) {
    return false;
  }

  if (!create_surface_format(context)) {
    return false;
  }

  create_extent(context, width, height);

  if (!create_present_modes(context)) {
    return false;
  }
  set_present_mode();

  set_num_swapchain_images();

  if (!create_swapchain(context)) {
    return false;
  }

  if (!create_images(context)) {
    return false;
  }

  if (!create_image_views(context)) {
    return false;
  }

  if (!create_semaphores(context)) {
    return false;
  }

  log_info(
    "Created swapchain:\n\t- Extent: {0}x{1}\n\t- Format: {2}\n\t- Images: {3}",
    _swapchain_size.width,
    _swapchain_size.height,
    _surface_format.format,
    _formats.size());

  return true;
}

} // namespace ripple::glow::backend