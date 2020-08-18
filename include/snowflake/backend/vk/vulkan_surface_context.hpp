//==--- snowflake/backend/vk/vulkan_surface_context.hpp ---- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_surface_context.hpp
/// \brief Header file for surface related vulkan context.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_BACKEND_VULKAN_SURFACE_CONTEXT_HPP
#define SNOWFLAKE_BACKEND_VULKAN_SURFACE_CONTEXT_HPP

#include "vulkan_context.hpp"
#include <atomic>
#include <vector>

namespace snowflake::backend {

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
  VkFormat       format;                      //!< Format of the attachment.
  VkImage        image      = VK_NULL_HANDLE; //!< Image for the attachment.
  VkImageView    image_view = VK_NULL_HANDLE; //!< View of the attachment image.
  VkDeviceMemory memory     = VK_NULL_HANDLE; //!< Memory for the attachment.
};

/// The SwapContext is a set of resources that are swapped in and out at the
/// start of a frame. There should be multiple instances per swapchain.
struct SwapContext {
  VulkanAttachment attachment; //!< The attachments for the context.
};

/// This stores surface related vulkan context, including the vulkan swapchain.
class VulkanSurfaceContext {
  // clang-format off
  /// Defines the type of the vector for the formats.
  using Formats      = std::vector<VkSurfaceFormatKHR>;
  /// Defines the type of the vector for the swap context.
  using SwapContexts = std::vector<SwapContext>;
  /// Defines the type of the vector for the images.
  using Images       = std::vector<VkImage>;
  /// Defines a container of supported present modes.
  using PresentModes = std::vector<VkPresentModeKHR>;
  // clang-format on

 public:
  //==--- [interface] ------------------------------------------------------==//

  /// Destroys the surface context.
  /// \param context The vulkan context to use to destroy the surface context.
  auto destroy(const VulkanContext& context) noexcept -> void;

  /// Gets a reference to the surface.
  auto surface() noexcept -> VkSurfaceKHR& {
    return surface_;
  }

  /// Gets a reference to the swapchain.
  auto swapchain() noexcept -> VkSwapchainKHR& {
    return swapchain_;
  }

  /// Updates the current swap index.
  auto update_swap_index() noexcept -> void {
    current_swap_idx_ = (current_swap_idx_ + 1) % swap_contexts_.size();
  }

  /// Returns the current swap index.
  auto current_swap_index() noexcept -> uint32_t& {
    return current_swap_idx_;
  }

  /// Returns the present mode for the surface context.
  auto present_mode() const noexcept -> PresentMode {
    return present_mode_;
  }

  /// Returns a reference to the image available semaphore.
  auto image_available_semaphore() noexcept -> VkSemaphore& {
    return image_available_;
  }

  /// Returns a reference to the done rendering semaphore.
  auto done_rendering_semaphore() noexcept -> VkSemaphore& {
    return done_rendering_;
  }

  /// Intializes the surface context with \p width and \p height and \p
  /// present_mode. This should only be called when the context is created.
  /// Calling `reinit()` will give better performance in subsequent calls.
  /// \param context      The vulkan context.
  /// \param present_mode The present mode for the surface context.
  /// \param width        The width of the surface.
  /// \param height       The height of the surface.
  auto init(
    const VulkanContext& context,
    PresentMode          present_mode,
    uint32_t             width,
    uint32_t             height) noexcept -> bool;

  /// Presents the current swapchain image to the graphics_queue from the
  /// context. It returns true if the presentation was successfull, or false is
  /// the `done_rendering_semaphore()` is not valid, or if there was an error
  /// with the presentation.
  /// \param context The context whose graphics queue will be presented to.
  /// \param fence   A fence to wait on, while the value of the fence is greater
  ///                than zero.
  auto
  present(const VulkanContext& context, std::atomic_uint32_t& fence) noexcept
    -> bool;

  /// Re-initializes the surface context with \p width and \p height and \p
  /// present_mode.
  ///
  /// This should only be called if `init()` has been called previously.
  ///
  /// \param context      The vulkan context.
  /// \param present_mode The present mode for the surface context.
  /// \param width        The width of the surface.
  /// \param height       The height of the surface.
  auto reinit(
    const VulkanContext& context,
    PresentMode          present_mode,
    uint32_t             width,
    uint32_t             height) noexcept -> bool;

 private:
  VkSurfaceKHR   surface_         = VK_NULL_HANDLE; //!< Context surface.
  VkSwapchainKHR swapchain_       = VK_NULL_HANDLE; //!< Current swapchain.
  VkSemaphore    image_available_ = VK_NULL_HANDLE; //!< If image is available.
  VkSemaphore    done_rendering_  = VK_NULL_HANDLE; //!< For finished rendering.
  VkQueue        present_queue_   = VK_NULL_HANDLE; //!< Presentation queue.

  VkSurfaceCapabilitiesKHR      surface_caps_;      //!< Surface capabilities.
  VkSurfaceFormatKHR            surface_format_;    //!< Format being used.
  VkSurfaceTransformFlagBitsKHR surface_transform_; //!< Surface transform.
  VkExtent2D                    swapchain_size_;    //!< Size of the surface.
  Formats                       formats_;           //!< All available format.
  SwapContexts                  swap_contexts_;     //!< Swap contexts.

  // clang-format off
  /// All supported present modes, so we dont have to query them each time the
  /// swapchain is recreated.
  PresentModes     present_modes_;
  /// The present mode for the surface.
  PresentMode      present_mode_ = PresentMode::sync_to_vblank;
  /// The present mode for the swapchain.
  VkPresentModeKHR swapchain_present_mode_;
  // clang-format on

  uint32_t current_swap_idx_  = 0;     //!< Current swap context index.
  uint32_t num_images_        = 0;     //!< Number of swapchain images.
  bool     srgb_enabled_      = false; //!< SRGB backfuffer enable.
  bool     prerotate_enabled_ = false; //!< Prerotate enabled.

  //==--- [methods] --------------------------------------------------------==//

  /// Creates the extent for the surface context.
  /// \param context The vulkan context.
  /// \param width   The width of the extent.
  /// \param height  The height of the extent.
  auto create_extent(
    const VulkanContext& context, uint32_t width, uint32_t height) noexcept
    -> void;

  /// Creates the images for the swapchain.
  auto create_images(const VulkanContext& context) noexcept -> bool;

  /// Creates the image views for the images.
  auto create_image_views(const VulkanContext& context) noexcept -> bool;

  /// Creates the present modes.
  /// \param context The vulkan context.
  auto create_present_modes(const VulkanContext& context) noexcept -> bool;

  /// Creates the semaphores for the surface images.
  auto create_semaphores(const VulkanContext& context) noexcept -> bool;

  /// Gets the surface capabilities for the surface.
  /// \param context The vulkan context.
  auto create_surface_caps(const VulkanContext& context) noexcept -> bool;

  /// Gets the surface format.
  /// \param context       The vulkan context.
  /// \param surface_info The surface info.
  auto create_surface_formats(
    const VulkanContext&             context,
    VkPhysicalDeviceSurfaceInfo2KHR& surface_info) noexcept -> bool;

  /// Gets the best surface format option.
  /// \param context The vulkan context.
  auto create_surface_format(const VulkanContext& context) noexcept -> bool;

  /// Creates the swapchain for the surface context.
  /// \param context The vulkan context.
  auto create_swapchain(const VulkanContext& context) noexcept -> bool;

  /// Intializes the vulkan swapchain, with \p width and \p height.
  /// \param context The vulkan context.
  /// \param width   The width of the surface.
  /// \param height  The height of the surface.
  auto init_swapchain(
    const VulkanContext& context, uint32_t width, uint32_t height) noexcept
    -> bool;

  /// Returns the composite mode for the swapchain.
  auto get_composite_mode() const noexcept -> VkCompositeAlphaFlagBitsKHR;

  /// Sets the present mode for the swapchain.
  auto set_present_mode() noexcept -> void;

  /// Sets the presentation queue.
  /// \param context The vulkan context.
  auto set_present_queue(const VulkanContext& context) noexcept -> void;

  /// Sets the transform properties for the surface.
  /// \param context The vulkan context.
  auto set_surface_transform(const VulkanContext& context) noexcept -> void;

  /// Sets the number of images for the swapchain.
  auto set_num_swapchain_images() noexcept -> void;

  /// Sets the composite modes for the swapchain.
  auto set_composite_mode() noexcept -> void;

  //==--- [destruction] ----------------------------------------------------==//

  /// Destroys the semaphores using the \p context.
  /// \param context The vulkan context used to initialize the semaphores.
  auto destroy_semaphores(const VulkanContext& context) noexcept -> void;

  /// Destroys the surface using the \p context.
  /// \param context The vulkan context used to initialize the surface.
  auto destroy_surface(const VulkanContext& context) noexcept -> void;

  /// Destroys the swapchain using the \p context.
  /// \param context The vulkan context used to initialize the swapchain.
  auto destroy_swapchain(const VulkanContext& context) noexcept -> void;

  /// Destroys the swap contexts using the \p context.
  /// \param context The vulkan context used to initialize the swap contexts.
  auto destroy_swap_contexts(const VulkanContext& context) noexcept -> void;
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_BACKEND_VULKAN_SURFACE_CONTEXT_HPP
