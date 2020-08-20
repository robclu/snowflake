//==--- snowflake/rendering/backend/vk/vulkan_context.hpp -- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_context.hpp
/// \brief Header file for a Vulkan context.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_CONTEXT_HPP
#define SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_CONTEXT_HPP

#include "vulkan_headers.hpp"
#include <tuple>

namespace snowflake::backend {

//==--- [forward declarations] ---------------------------------------------==//

class VulkanDriver;

//==--- [vendor] -----------------------------------------------------------==//

/// Defines the kinds of the vendors.
enum class Vendor : uint16_t {
  AMD      = 0x1002, //!< AMD vendor kind.
  NVIDIA   = 0x10de, //!< Nvidia vendor kind.
  Intel    = 0x8086, //!< Intel vendor kind.
  ARM      = 0x13b5, //!< Arm vendor kind.
  Qualcomm = 0x5143  //!< Qualcomm vendor kind.
};

/// The VulkanContext type holds state for vulkan. Specifically, it created an
/// instance, physical device, finds queues and queue indices, and create a
/// logical device.
/// VkDevice.
class VulkanContext {
 public:
  //==--- [construction] ---------------------------------------------------==//

  // clang-format off

  /// Default constructor for the constext.
  VulkanContext() noexcept = default;

  /// Deleted copy construction, since only one context is allowed.
  VulkanContext(const VulkanContext&) = delete;

  /// Defaulted move constructor.
  VulkanContext(VulkanContext&&) noexcept = default;

  /// Destructor to clean up the resources.
  ~VulkanContext() noexcept;

  /// Destroys the context. This is provided to allow explicit control of
  /// destroying the context. If this is not called, the destructor will clean
  /// up the context.
  auto destroy() noexcept -> void;

  // clang-format on

  //==--- [operator overloads] ---------------------------------------------==//

  /// Deleted copy assignment since only one context is allowed.
  auto operator=(const VulkanContext&) = delete;

  /// Defaulted move assignment.
  auto operator=(VulkanContext&&) noexcept -> VulkanContext& = default;

  //==--- [static] ---------------------------------------------------------==//

  /// Initializes the vulkan loader, using the loader at \p addr, returning true
  /// if it succeeds.
  /// \param addr The address of the loader function.
  static auto init_loader(PFN_vkGetInstanceProcAddr addr) noexcept -> bool;

  /// Gets the application information for vulkan.
  static auto get_application_info() noexcept -> const VkApplicationInfo&;

  //==--- [interface] ------------------------------------------------------==//

  /// Creates the instance and the device, with \p ins_extensions for the
  /// instance, and \p dev_extensions for the device. This returns false if the
  /// initialization was not successful.
  /// \param ins_extensions     The instance extensions.
  /// \param num_ins_extensions The number of instance extensions.
  /// \param dev_extensions     The device extensions.
  /// \param num_dev_extensions The number of device extensions.
  /// \param surface            The surface for creation.
  auto create_instance_and_device(
    const char** ins_extensions,
    uint32_t     num_ins_extensions,
    const char** dev_extensions,
    uint32_t     num_dev_extesions,
    VkSurfaceKHR surface = VK_NULL_HANDLE) noexcept -> bool;

  /// Returns the vulkan instance.
  auto instance() const noexcept -> VkInstance {
    return instance_;
  }

  /// Returns the vulkan physical device.
  auto physical_device() const noexcept -> VkPhysicalDevice {
    return phy_dev_;
  }

  /// Returns the logical device for the context.
  auto device() const noexcept -> VkDevice {
    return device_;
  }

  /// Returns a pointer to the device table for the instance.
  auto device_table() const noexcept -> const VolkDeviceTable* {
    return &device_table_;
  }

  /// Returns true if surface capabilities 2 are supported.
  auto supports_surface_caps_2() const noexcept -> bool {
    return supports_surface_caps_2_;
  }

  //==--- [queue interface] ------------------------------------------------==//

  /// Returns the index of the graphics queue family.
  auto graphics_queue_family_index() const noexcept -> uint32_t {
    return graphics_queue_index_;
  }
  /// Returns the index of the compute queue family.
  auto compute_queue_family_index() const noexcept -> uint32_t {
    return compute_queue_index_;
  }
  /// Returns the index of the transfer queue family.
  auto transfer_queue_family_index() const noexcept -> uint32_t {
    return transfer_queue_index_;
  }

  /// Returns the graphics queue.
  auto graphics_queue() const noexcept -> VkQueue {
    return graphics_queue_;
  }
  /// Returns the compute queue.
  auto compute_queue() const noexcept -> VkQueue {
    return compute_queue_;
  }
  /// Returns the transfer queue.
  auto transfer_queue() const noexcept -> VkQueue {
    return transfer_queue_;
  }

 private:
  //==--- [constants] ------------------------------------------------------==//

  // clang-format off
  /// Priority for teh graphics queue.
  static constexpr float graphics_queue_priority = 0.5f;
  /// Priority for the compute queue.
  static constexpr float compute_queue_priority  = 1.0f;
  /// Priority for the transfer queue.
  static constexpr float transfer_queue_priority = 1.0f;

  //==--- [members] --------------------------------------------------------==//

  VkDevice         device_   = VK_NULL_HANDLE; //!< Vulkan device.
  VkInstance       instance_ = VK_NULL_HANDLE; //!< Vulkan instance.
  VkPhysicalDevice phy_dev_  = VK_NULL_HANDLE; //!< Physical device.

  /// Device table for vulkan functions.
  VolkDeviceTable device_table_;

  VkPhysicalDeviceProperties       dev_props_     = {}; //!< Dev props.
  VkPhysicalDeviceFeatures2KHR     dev_features_  = {}; //!< Dev features.
  VkPhysicalDeviceMemoryProperties dev_mem_props_ = {}; //!< Dev mem props.

  VkQueue graphics_queue_ = VK_NULL_HANDLE; //!< Queue for graphics.
  VkQueue compute_queue_  = VK_NULL_HANDLE; //!< Queue for compute.
  VkQueue transfer_queue_ = VK_NULL_HANDLE; //!< Queue for transfer.

  // clang-format off
  /// Graphics queue family index.
  uint32_t graphics_queue_family_ = VK_QUEUE_FAMILY_IGNORED;
  /// Compute queue family index.
  uint32_t compute_queue_family_  = VK_QUEUE_FAMILY_IGNORED;
  /// Transfer queue family index.
  uint32_t transfer_queue_family_ = VK_QUEUE_FAMILY_IGNORED;
  // clang-format on

  uint32_t graphics_queue_index_  = 0; //!< Index of the graphics queue.
  uint32_t compute_queue_index_   = 0; //!< Index of the compute queue.
  uint32_t transfer_queue_index_  = 0; //!< Index of the transfer queue.
  unsigned universal_queue_index_ = 1; //!< Index of a universal queue.

  bool supports_vulkan_11_       = false; //!< Supports VK >= 1.1
  bool supports_surface_caps_2_  = false; //!< Supports surf caps 2.
  bool supports_phy_dev_props_2_ = false; //!< Supports dev props 2.
  bool supports_external_        = false; //!< Supports external props.
  bool destroyed_                = false; //!< If the context is destroyed.

#ifdef VULKAN_DEBUG
  bool supports_debug_utils_ = false; //!< Supports debug utils.
  // clang-format off
  /// Callback for debugging.
  VkDebugReportCallbackEXT debug_callback_  = VK_NULL_HANDLE;
  /// Messenger for debugging.
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    // clang-format on
#endif

  //==--- [methods] --------------------------------------------------------==//

  /// Creates the vulkan device with the \p dev pysical device, the \p surface,
  /// \p dev_extensions, and \p dev_layers.
  /// \param dev            The physical device.
  /// \param surface        The surface for the device.
  /// \param dev_extensions The required device extensions.
  /// \param num_extensions The number of extensions.
  /// \param dev_layers     The required device layers.
  /// \param num_layers     The number of layers.
  auto create_device(
    VkPhysicalDevice dev,
    VkSurfaceKHR     surface,
    const char**     dev_extesions,
    uint32_t         num_extensions,
    const char**     dev_layers,
    uint32_t         num_layers) noexcept -> bool;

  /// Creates the vulkan instance with \p ins_extensions extensions and \p
  /// num_extensions number of extensions. Returns false if the instance could
  /// not be created.
  /// \param ins_extensions The instance extensions.
  /// \param num_extensions The number of extensions.
  auto
  create_instance(const char** ins_extensions, uint32_t num_extension) noexcept
    -> bool;

  /// Creates the queue information for device creation, returning a vector of
  /// filled VkDeviceQueueCreateInfo for each queue, and a vector of proritities
  /// for each of the queues. Since the VkDeviceQueueCreateInfo's in the vector
  /// refere to the priorities in the second vector, the second vector needs to
  /// be kept around until the device is created.
  auto create_queue_info() const noexcept
    -> std::tuple<std::vector<VkDeviceQueueCreateInfo>, std::vector<float>>;

  /// Returns the flags required for the device.
  auto required_flags() const noexcept -> VkQueueFlags {
    return VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
  }

  /// Selects a physical device which supports the \p surface, and which has the
  /// queue flags returned by `required_flags()`.
  ///
  /// \post The following will be set when this return true:
  ///         - _phy_dev is set to the selected device
  ///         - _dev_props is set to the _phy_dev device properties
  ///         - _dev_mem_props is set to the _phy_dev device mem properties
  ///         - _graphics_queue_family is set to the index of the gfx queue
  ///
  /// \param surface The surface to provide support for.
  auto select_physical_device(VkSurfaceKHR surface) noexcept -> bool;

  /// Selects all queue families. This first tries to select separate queue
  /// families, and then ensures that all queue families have been selected, and
  /// sets the values of the indices of the queues in the queue families.
  ///
  /// \param queue_props A vector of queue properties.
  auto select_queue_families(
    const std::vector<VkQueueFamilyProperties>& queue_props) noexcept -> void;

  /// Finds the indices for the queues.

  /// Tries to select as many separate queue families as possible. To ensure
  /// that all
  ///
  /// \param queue_props A vector of queue properties.
  auto try_select_separate_queue_families(
    const std::vector<VkQueueFamilyProperties>& queue_props) noexcept -> void;

  /// Checks that all the queue families have valid indices, and sets the

  /// Validates the \p dev_extensions against those available for the context
  /// physical device.
  ///
  /// \pre `select_physical_device()` should have been called prviously so that
  ///      a valid physical device can be validated against.
  ///
  /// Returns true if the validation succeeds.
  ///
  /// \param dev_extensions The required device extensions.
  /// \param num_extensions The number of extensions.
  auto validate_extensions(
    const char** dev_extensions, uint32_t num_extensions) noexcept -> bool;

  /// Validates the \p dev_layers against those available for the context
  /// physical device.
  ///
  /// \pre `select_physical_device()` should have been called prviously so that
  ///      a valid physical device can be validated against.
  ///
  /// Returns true if the validation succeeds.
  ///
  /// \param dev_layers The required device layers.
  /// \param num_layers The number of layers.
  auto validate_layers(const char** dev_layers, uint32_t num_layers) noexcept
    -> bool;
};

} // namespace snowflake::backend

#endif // SNOWFLAKE_RENDERING_BACKEND_VK_VULKAN_CONTEXT_HPP
