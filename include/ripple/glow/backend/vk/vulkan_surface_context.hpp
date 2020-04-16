//==--- glow/backend/vk/vulkan_surface_context.hpp --------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_surface_context.hpp
/// \brief Header file for surface related vulkan context.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_BACKEND_VULKAN_SURFACE_CONTEXT_HPP
#define RIPPLE_GLOW_BACKEND_VULKAN_SURFACE_CONTEXT_HPP

#include "vulkan_context.hpp"

namespace ripple::glow::backend {

/// Froward declaration of the vulkan driver.
class VulkanDriver;

/// The type of present mode for the driver.
enum class PresentMode : uint8_t {
  sync_to_vblank = 0, //!< Always sync to vert blanking (FIFO).
  maybe_tear     = 1, //!< Possible tearing (MAILBOX or IMMEDIATE).
  force_tear     = 2, //!< Likely tearning (IMMEDIATE).
  no_tear        = 3  //!< No tearing (MAILBOX).
};

struct VulkanAttachment {
  VkFormat       format;     //!< Format of the attachment.
  VkImage        image;      //!< Image for the attachment.
  VkImageView    image_view; //!< View of the attachment image.
  VkDeviceMemory memory;     //!< Memory for the attachment.
};

/// The SwapContext is a set of resources that are swapped in and out at the
/// start of a frame. There should be multiple instances per swapchain.
struct SwapContext {
  VulkanAttachment attachment; //!< The attachments for the context.
};

/// This stores surface related vulkan context, including the vulkan swapchain.
class VulkanSurfaceContext {
  /// Defines the type of the vector for the formats.
  using formats_t = std::vector<VkSurfaceFormatKHR>;
  /// Defines the type of the vector for the swap context.
  using swap_contexts_t = std::vector<SwapContext>;

 public:
  /// Gets a reference to the surface.
  auto surface() -> VkSurfaceKHR& {
    return _surface;
  }

  /// Gets a reference to the swapchain.
  auto swapchain() -> VkSwapchainKHR& {
    return _swapchain;
  }

  /// Updates the current swap index.
  auto update_swap_index() -> void {
    _current_swap_idx = (_current_swap_idx + 1) % _swap_contexts.size();
  }

  /// Returns the current swap index.
  auto current_swap_index() -> uint32_t& {
    return _current_swap_idx;
  }

  /// Returns a reference to the image available semaphore.
  auto image_available_semaphore() -> VkSemaphore& {
    return _image_available;
  }

  /// Returns a reference to the done rendering semaphore.
  auto done_rendering_semaphore() -> VkSemaphore& {
    return _done_rendering;
  }

  /// Intializes the surface context with \p width and \p height.
  /// \param context The vulkan context.
  /// \param width   The width of the surface.
  /// \param height  The height of the surface.
  auto
  init(const VulkanContext& context, uint32_t width, uint32_t height) -> bool;

 private:
  /// The surface for the
  VkSurfaceKHR   _surface         = VK_NULL_HANDLE; //!< Context surface.
  VkSwapchainKHR _swapchain       = VK_NULL_HANDLE; //!< Current swapchain.
  VkSemaphore    _image_available = VK_NULL_HANDLE; //!< If image is available.
  VkSemaphore    _done_rendering  = VK_NULL_HANDLE; //!< For finished rendering.

  VkSurfaceCapabilitiesKHR      _surface_caps;      //!< Surface capabilities.
  VkSurfaceFormatKHR            _surface_format;    //!< Format being used.
  VkSurfaceTransformFlagBitsKHR _surface_transform; //!< Surface transform.
  VkExtent2D                    _swapchain_size;    //!< Size of the surface.
  formats_t                     _formats;           //!< All available format.
  swap_contexts_t               _swap_contexts;     //!< Swap contexts.

  /// The present mode for the surface.
  PresentMode _present_mode = PresentMode::sync_to_vblank;
  /// The present mode for the swapchain.
  VkPresentModeKHR _swapchain_present_mode;

  uint32_t _current_swap_idx  = 0;     //!< Current swap context index.
  uint32_t _num_images        = 0;     //!< Number of swapchain images.
  bool     _srgb_enabled      = false; //!< SRGB backfuffer enable.
  bool     _prerotate_enabled = false; //!< Prerotate enabled.

  //==--- [methods] --------------------------------------------------------==//

  /// Creates the extent for the surface context.
  /// \param context The vulkan context.
  /// \param width   The width of the extent.
  /// \param height  The height of the extent.
  auto
  create_extent(const VulkanContext& context, uint32_t width, uint32_t height)
    -> void;

  /// Creates the images for the swapchain.
  auto create_images(const VulkanContext& context) -> bool;

  /// Creates the image views for the images.
  auto create_image_views(const VulkanContext& context) -> bool;

  /// Creates the present modes.
  /// \param context The vulkan context.
  auto create_present_modes(const VulkanContext& context) -> bool;

  /// Creates the semaphores for the surface images.
  auto create_semaphores(const VulkanContext& context) -> bool;

  /// Gets the surface capabilities for the surface.
  /// \param context The vulkan context.
  auto create_surface_caps(const VulkanContext& context) -> bool;

  /// Gets the surface format.
  /// \param context       The vulkan context.
  /// \param surface_info The surface info.
  auto create_surface_formats(
    const VulkanContext& context, VkPhysicalDeviceSurfaceInfo2KHR& surface_info)
    -> bool;

  /// Gets the best surface format option.
  /// \param context The vulkan context.
  auto create_surface_format(const VulkanContext& context) -> bool;

  /// Creates the swapchain for the surface context.
  /// \param context The vulkan context.
  auto create_swapchain(const VulkanContext& context) -> bool;

  /// Intializes the vulkan swapchain, with \p width and \p height.
  /// \param context The vulkan context.
  /// \param width   The width of the surface.
  /// \param height  The height of the surface.
  auto
  init_swapchain(const VulkanContext& context, uint32_t width, uint32_t height)
    -> bool;

  /// Returns the composite mode for the swapchain.
  auto get_composite_mode() const -> VkCompositeAlphaFlagBitsKHR;

  /// Initializes the swapchain

  /// Sets the transform properties for the surface.
  /// \param context The vulkan context.
  auto set_surface_transform(const VulkanContext& context) -> void;

  /// Sets the number of images for the swapchain.
  auto set_num_swapchain_images() -> void;

  /// Sets the composite modes for the swapchain.
  auto set_composite_mode() -> void;
};

} // namespace ripple::glow::backend

#endif // RIPPLE_GLOW_BACKEND_VULKAN_SURFACE_CONTEXT_HPP
