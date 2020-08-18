//==--- snowflake/src/backend/vk/vulkan_surface_context.cpp --*- C++ -*- ---==//
//
//                              Snowflake
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

#include <snowflake/backend/vk/vulkan_surface_context.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake::backend {

namespace {

static inline auto
format_to_aspect_mask(VkFormat format) noexcept -> VkImageAspectFlags {
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

//==--- [interface] --------------------------------------------------------==//

auto VulkanSurfaceContext::destroy(const VulkanContext& context) noexcept
  -> void {
  destroy_swap_contexts(context);
  destroy_swapchain(context);
  destroy_semaphores(context);
  destroy_surface(context);
}

auto VulkanSurfaceContext::init(
  const VulkanContext& context,
  PresentMode          present_mode,
  uint32_t             width,
  uint32_t             height) noexcept -> bool {
  if (surface_ == VK_NULL_HANDLE) {
    wrench::log_error("Can't initialize surface context until surface is set.");
    return false;
  }

  present_mode_ = present_mode;

  if (!init_swapchain(context, width, height)) {
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::present(
  const VulkanContext& context, std::atomic_uint32_t& fence) noexcept -> bool {
  if (done_rendering_ == VK_NULL_HANDLE) {
    return false;
  }

  // Submit that we are done rendering:
  // TODO: Move this intro the submission of the last command buffers.
  //       Also, the wait semaphore should be somehting else when this is moved.
  // clang-format off
  VkPipelineStageFlags wait_dest_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo         submit_info{
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount   = 1u,
    .pWaitSemaphores      = &image_available_,
    .pWaitDstStageMask    = &wait_dest_stage_mask,
    .commandBufferCount   = 0,
    .pCommandBuffers      = nullptr,
    .signalSemaphoreCount = 1u,
    .pSignalSemaphores    = &done_rendering_,
  };
  // clang-format on

  auto result =
    vkQueueSubmit(context.graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to submit semaphore signal for done rendering.");
  }

  while (fence.load(std::memory_order_relaxed) > 0) {
    // Wait for any outstanding work on other threads ...
  }

  // Present:
  result                  = VK_SUCCESS;
  VkPresentInfoKHR info   = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores    = &done_rendering_;
  info.swapchainCount     = 1;
  info.pSwapchains        = &swapchain_;
  info.pImageIndices      = &current_swap_idx_;
  info.pResults           = &result;

  auto submit_result =
    context.device_table()->vkQueuePresentKHR(present_queue_, &info);

#ifdef ANDROID
  // clang-format off
  // Because of the pre-transform, Android might return sub-optimal, which we
  // treat as a success.
  if (submit_result == VK_SUBOPTIMAL_KHR) { submit_result = VK_SUCCESS; }
  if (result        == VK_SUBOPTIMAL_KHR) { result        = VK_SUCCESS; }
  // clang-format on
#endif

  if (submit_result != VK_SUCCESS || result != VK_SUCCESS) {
    wrench::log_error("Failed to present to queue.");
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::reinit(
  const VulkanContext& context,
  PresentMode          present_mode,
  uint32_t             width,
  uint32_t             height) noexcept -> bool {
  present_mode_ = present_mode;
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

auto VulkanSurfaceContext::create_surface_caps(
  const VulkanContext& context) noexcept -> bool {
  VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
  surface_info.surface = surface_;

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

    surface_caps_ = surface_caps_2.surfaceCapabilities;
  } else {
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      phy_device, surface_, &surface_caps_);
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
  const VulkanContext&             context,
  VkPhysicalDeviceSurfaceInfo2KHR& surface_info) noexcept -> bool {
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

    formats_.reserve(format_count);
    for (auto& f : formats2) {
      formats_.push_back(f.surfaceFormat);
    }
  } else {
    auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      device, surface_, &format_count, nullptr);
    if (result != VK_SUCCESS) {
      return false;
    }
    formats_.resize(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      device, surface_, &format_count, formats_.data());

    if (result != VK_SUCCESS) {
      return false;
    }
  }
  return true;
}

auto VulkanSurfaceContext::create_surface_format(
  const VulkanContext& context) noexcept -> bool {
  const auto format_count = formats_.size();
  if (format_count == 0) {
    wrench::log_error("Surface has no formats, can't create swapchain.");
    return false;
  }

  // Only 1 undefined format, just choose it to be the default:
  if (format_count == 1 && formats_.front().format == VK_FORMAT_UNDEFINED) {
    surface_format_        = formats_.front();
    surface_format_.format = VK_FORMAT_B8G8R8A8_UNORM;
    return true;
  }

  // Have multiple, look for something that we want:
  for (const auto& format : formats_) {
    // Look for SRGB if that's what we want:
    if (srgb_enabled_) {
      if (
        format.format == VK_FORMAT_R8G8B8A8_SRGB ||
        format.format == VK_FORMAT_B8G8R8A8_SRGB ||
        format.format == VK_FORMAT_A8B8G8R8_SRGB_PACK32) {
        surface_format_ = format;
        return true;
      }
      continue;
    }
    // Otherwise just look for the default unorm:
    if (
      format.format == VK_FORMAT_R8G8B8A8_UNORM ||
      format.format == VK_FORMAT_B8G8R8A8_UNORM ||
      format.format == VK_FORMAT_A8B8G8R8_UNORM_PACK32) {
      surface_format_ = format;
      return true;
    }
  }

  // Haven't found one that we want, choose the first:
  surface_format_ = formats_.front();
  return true;
}

auto VulkanSurfaceContext::set_surface_transform(
  const VulkanContext& context) noexcept -> void {
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

  if constexpr (wrench::Log::would_log<wrench::LogLevel::info>()) {
    wrench::log_info(
      "Current surface transform is 0x{0:x}",
      static_cast<unsigned>(surface_caps_.currentTransform));

    for (unsigned i = 0; i <= transform_names.size(); ++i) {
      if (surface_caps_.supportedTransforms & (1u << i)) {
        wrench::log_info(
          "Supported transform 0x{0:x}: {1}", 1u << i, transform_names[i]);
      }
    }
  }

  VkSurfaceTransformFlagBitsKHR pre_transform;
  const auto has_id_bit = (surface_caps_.supportedTransforms &
                           VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0;
  if (!prerotate_enabled_ && has_id_bit) {
    pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    pre_transform = surface_caps_.currentTransform;
  }

  if (pre_transform != surface_caps_.currentTransform) {
    wrench::log_warn(
      "Surface transform (0x{0:x}) does not match current transform (0x{1:x}) "
      " Might get performance penalty",
      static_cast<unsigned>(pre_transform),
      static_cast<unsigned>(surface_caps_.currentTransform));
  }

  surface_transform_ = pre_transform;
}

auto VulkanSurfaceContext::create_extent(
  const VulkanContext& context, uint32_t width, uint32_t height) noexcept
  -> void {
  set_surface_transform(context);

  wrench::log_info(
    "Swapchain current extent: {0} x {1}",
    static_cast<int>(surface_caps_.currentExtent.width),
    static_cast<int>(surface_caps_.currentExtent.height));

  // TODO: Try to match the swapchain size up with what we expect.

  // If we are using pre-rotate of 90 or 270 degrees,
  // we need to flip width and height.
  const auto mask = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR |
                    VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR |
                    VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR |
                    VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR;
  if (surface_transform_ & mask) {
    std::swap(width, height);
  }

  // Clamp the target width, height to boundaries.
  const auto& ext_max    = surface_caps_.maxImageExtent;
  const auto& ext_min    = surface_caps_.minImageExtent;
  const auto  w_min      = std::min(width, ext_max.width);
  const auto  h_min      = std::min(height, ext_max.height);
  swapchain_size_.width  = std::max(w_min, ext_min.width);
  swapchain_size_.height = std::max(h_min, ext_min.height);
}

auto VulkanSurfaceContext::create_present_modes(
  const VulkanContext& context) noexcept -> bool {
  auto num_present_modes = uint32_t{0};
  auto phy_dev           = context.physical_device();
  auto result            = vkGetPhysicalDeviceSurfacePresentModesKHR(
    phy_dev, surface_, &num_present_modes, nullptr);
  if (result != VK_SUCCESS) {
    wrench::log_error(
      "Failed to get number of present modes for surface context.");
    return false;
  }

  present_modes_.resize(num_present_modes);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    phy_dev, surface_, &num_present_modes, present_modes_.data());
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to create present modes for surface context.");
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::set_present_mode() noexcept -> void {
  swapchain_present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
  if (present_mode_ == PresentMode::sync_to_vblank) {
    return;
  }

  // Try the others:
  const bool allow_mlbox = present_mode_ != PresentMode::force_tear;
  const bool allow_immed = present_mode_ != PresentMode::no_tear;
  for (auto& present_mode : present_modes_) {
    if (
      (allow_immed && present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) ||
      (allow_mlbox && present_mode == VK_PRESENT_MODE_MAILBOX_KHR)) {
      swapchain_present_mode_ = present_mode;
      return;
    }
  }
}

auto VulkanSurfaceContext::set_present_queue(
  const VulkanContext& context) noexcept -> void {
  uint32_t queue_fam_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    context.physical_device(), &queue_fam_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_fam_props(queue_fam_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    context.physical_device(), &queue_fam_count, queue_fam_props.data());
  uint32_t present_queue_fam_index = 0xffff;

  // Check if graphics queue supports presentation, because that's ideal.
  // It's also the case for most platforms.
  VkBool32 supported = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR(
    context.physical_device(),
    context.graphics_queue_family_index(),
    surface_,
    &supported);

  if (supported) {
    present_queue_ = context.graphics_queue();
    return;
  }

  // If the graphics queue can't present, we need a separate one.
  // Otherwise fall back to separate graphics and presentation queues.
  for (uint32_t i = 0; i < queue_fam_count; ++i) {
    vkGetPhysicalDeviceSurfaceSupportKHR(
      context.physical_device(), i, surface_, &supported);
    if (supported) {
      context.device_table()->vkGetDeviceQueue(
        context.device(), i, 0, &present_queue_);
      return;
    }
  }

  wrench::log_error("Failed to find a presentation queue!");
}

auto VulkanSurfaceContext::set_num_swapchain_images() noexcept -> void {
  // Already found the number of images, or set somewhere else.
  if (num_images_ > 0) {
    return;
  }

  // The general advice is to require one more than the minimum swap chain
  // length, since the absolute minimum could easily require waiting for a
  // driver or presentation layer to release the previous frame's buffer. The
  // only situation in which we'd ask for the minimum length is when using a
  // MAILBOX presentation strategy for low-latency situations where tearing is
  // acceptable.
  const uint32_t max_count = surface_caps_.maxImageCount;
  const uint32_t min_count = surface_caps_.minImageCount;

  num_images_ = min_count + (present_mode_ != PresentMode::no_tear ? 1 : 0);

  // According to section 30.5 of VK 1.1, maxImageCount of zero means "that
  // there is no limit on the number of images, though there may be limits
  // related to the total amount of memory used by presentable images."
  if (max_count != 0 && num_images_ > max_count) {
    wrench::log_error("Swap chain does not support {} images.", num_images_);
    num_images_ = min_count;
  }

  wrench::log_info("Using {} swapchain images.", num_images_);
}

auto VulkanSurfaceContext::get_composite_mode() const noexcept
  -> VkCompositeAlphaFlagBitsKHR {
  VkCompositeAlphaFlagBitsKHR composite_mode =
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  const auto alphas = {VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                       VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                       VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                       VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

  for (auto alpha : alphas) {
    if (surface_caps_.supportedCompositeAlpha & alpha) {
      composite_mode = alpha;
    }
  }
  return composite_mode;
}

auto VulkanSurfaceContext::create_swapchain(
  const VulkanContext& context) noexcept -> bool {
  VkSwapchainKHR old_swapchain = swapchain_;

  // clang-format off
  VkSwapchainCreateInfoKHR info = {
    .sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface            = surface_,
    .minImageCount      = num_images_,
    .imageFormat        = surface_format_.format,
    .imageColorSpace    = surface_format_.colorSpace,
    .imageExtent.width  = swapchain_size_.width,
    .imageExtent.height = swapchain_size_.height,
    .imageArrayLayers   = 1,
    .imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform       = surface_transform_,
    .compositeAlpha     = get_composite_mode(),
    .presentMode        = swapchain_present_mode_,
    .clipped            = VK_TRUE,
    .oldSwapchain       = old_swapchain};
  // clang-format on

  auto result = context.device_table()->vkCreateSwapchainKHR(
    context.device(), &info, nullptr, &swapchain_);

  if (old_swapchain != VK_NULL_HANDLE) {
    context.device_table()->vkDestroySwapchainKHR(
      context.device(), old_swapchain, nullptr);
  }

  if (result != VK_SUCCESS) {
    wrench::log_error(
      "Failed to create swapchain: (code : {})", static_cast<int>(result));
    swapchain_ = VK_NULL_HANDLE;
    return false;
  }
  return true;
}

auto VulkanSurfaceContext::create_images(const VulkanContext& context) noexcept
  -> bool {
  uint32_t image_count = 0;
  auto     result      = context.device_table()->vkGetSwapchainImagesKHR(
    context.device(), swapchain_, &image_count, nullptr);
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to get image count for swapchain.");
    return false;
  }

  swap_contexts_.resize(image_count);
  std::vector<VkImage> images;
  images.resize(image_count);
  result = context.device_table()->vkGetSwapchainImagesKHR(
    context.device(), swapchain_, &image_count, images.data());
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to create swapchain image data.");
    return false;
  }

  // clang-format off
  for (size_t i = 0; i < images.size(); ++i) {
    swap_contexts_[i].attachment = {
      .format     = surface_format_.format,
      .image      = images[i],
      .image_view = {},
      .memory     = {}};
  }
  // clang-format on

  return true;
}

auto VulkanSurfaceContext::create_image_views(
  const VulkanContext& context) noexcept -> bool {
  VkImageViewCreateInfo info = {};
  info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

  info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  info.format                          = surface_format_.format;
  info.components.r                    = VK_COMPONENT_SWIZZLE_R;
  info.components.g                    = VK_COMPONENT_SWIZZLE_G;
  info.components.b                    = VK_COMPONENT_SWIZZLE_B;
  info.components.a                    = VK_COMPONENT_SWIZZLE_A;
  info.subresourceRange.baseMipLevel   = 0;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.levelCount     = 1;
  info.subresourceRange.layerCount     = 1;
  info.subresourceRange.aspectMask =
    format_to_aspect_mask(surface_format_.format);

  for (auto& swap_context : swap_contexts_) {
    info.image  = swap_context.attachment.image;
    auto result = context.device_table()->vkCreateImageView(
      context.device(), &info, nullptr, &swap_context.attachment.image_view);

    if (result != VK_SUCCESS) {
      wrench::log_error("Failed to create image view for attachement!");
    }
  }

  return true;
}

auto VulkanSurfaceContext::create_semaphores(
  const VulkanContext& context) noexcept -> bool {
  VkSemaphoreCreateInfo create_info = {};
  create_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  auto result = context.device_table()->vkCreateSemaphore(
    context.device(), &create_info, nullptr, &image_available_);
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to create image available semaphore.");
    return false;
  }

  result = context.device_table()->vkCreateSemaphore(
    context.device(), &create_info, nullptr, &done_rendering_);
  if (result != VK_SUCCESS) {
    wrench::log_error("Failed to create done rendering semaphore.");
    return false;
  }

  return true;
}

auto VulkanSurfaceContext::init_swapchain(
  const VulkanContext& context, uint32_t width, uint32_t height) noexcept
  -> bool {
  set_present_queue(context);

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

  wrench::log_info(
    "Created swapchain:\n\t- Extent: {0}x{1}\n\t- Format: {2}\n\t- Images: {3}",
    swapchain_size_.width,
    swapchain_size_.height,
    surface_format_.format,
    formats_.size());

  return true;
}

//==--- [destruction] ------------------------------------------------------==//

auto VulkanSurfaceContext::destroy_semaphores(
  const VulkanContext& context) noexcept -> void {
  if (image_available_ != VK_NULL_HANDLE) {
    context.device_table()->vkDestroySemaphore(
      context.device(), image_available_, nullptr);
  }
  if (done_rendering_ != VK_NULL_HANDLE) {
    context.device_table()->vkDestroySemaphore(
      context.device(), done_rendering_, nullptr);
  }
}

auto VulkanSurfaceContext::destroy_surface(
  const VulkanContext& context) noexcept -> void {
  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(context.instance(), surface_, nullptr);
  }
}

auto VulkanSurfaceContext::destroy_swapchain(
  const VulkanContext& context) noexcept -> void {
  if (swapchain_ != VK_NULL_HANDLE) {
    context.device_table()->vkDestroySwapchainKHR(
      context.device(), swapchain_, nullptr);
  }
}

auto VulkanSurfaceContext::destroy_swap_contexts(
  const VulkanContext& context) noexcept -> void {
  auto device = context.device();
  for (auto& swap_context : swap_contexts_) {
    auto& attachment = swap_context.attachment;
    if (attachment.image_view != VK_NULL_HANDLE) {
      context.device_table()->vkDestroyImageView(
        device, attachment.image_view, nullptr);
      attachment.image_view = VK_NULL_HANDLE;
    }
  }
}

} // namespace snowflake::backend