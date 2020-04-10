//==--- glow/vk/context.hpp -------------------------------- -*- C++ -*- ---==//
//
//                              Ripple - Glow
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  context.hpp
/// \brief Header file for a Vulkan context.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_GLOW_VK_CONTEXT_HPP
#define RIPPLE_GLOW_VK_CONTEXT_HPP

#include "vulkan_headers.hpp"
#include <tuple>

namespace ripple::glow::vk {

//==--- [vendor] -----------------------------------------------------------==//

/// Defines the kinds of the vendors.
enum class Vendor : uint16_t {
  AMD      = 0x1002, //!< AMD vendor kind.
  NVIDIA   = 0x10de, //!< Nvidia vendor kind.
  Intel    = 0x8086, //!< Intel vendor kind.
  ARM      = 0x13b5, //!< Arm vendor kind.
  Qualcomm = 0x5143  //!< Qualcomm vendor kind.
};

/// The Context for vulkan which owns all the structs necessary for the vulkan
/// context. The main purpose of the context is to create the VkInstance and the
/// VkDevice.
class Context {
 public:
  //==--- [construction] ---------------------------------------------------==//

  // clang-format off

  /// Default constructor for the constext.
  Context() = default;

  /// Deleted copy construction, since only one context is allowed.
  Context(const Context&) = delete;

  /// Defaulted move constructor.
  Context(Context&&) noexcept = default;

  /// Destructor to clean up the resources.
  ~Context();

  // clang-format on

  //==--- [operator overloads] ---------------------------------------------==//

  /// Deleted copy assignment since only one context is allowed.
  auto operator=(const Context&) = delete;

  /// Defaulted move assignment.
  auto operator=(Context&&) noexcept -> Context& = default;

  //==--- [static] ---------------------------------------------------------==//

  /// Initializes the vulkan loader, using the loader at \p addr, returning true
  /// if it succeeds.
  /// \param addr The address of the loader function.
  static auto init_loader(PFN_vkGetInstanceProcAddr addr) -> bool;

  /// Gets the application information for vulkan.
  static auto get_application_info() -> const VkApplicationInfo&;

  //==--- [interface] ------------------------------------------------------==//

  /// Creates the instance and the device, with \p ins_extensions for the
  /// instance, and \p dev_extensions for the device. This returns false if the
  /// initialization was not successful.
  auto create_instance_and_device(
    const char** ins_extensions,
    uint32_t     num_ins_extensions,
    const char** dev_extensions,
    uint32_t     num_dev_extesions) -> bool;

 private:
  //==--- [constants] ------------------------------------------------------==//

  // clang-format off
  /// Priority for teh graphics queue.
  static constexpr float graphics_queue_priority_v = 0.5f;
  /// Priority for the compute queue.
  static constexpr float compute_queue_priority_v  = 1.0f;
  /// Priority for the transfer queue.
  static constexpr float transfer_queue_priority_v = 1.0f;

  //==--- [members] --------------------------------------------------------==//

  VkDevice         _device   = VK_NULL_HANDLE; //!< Vulkan device.
  VkInstance       _instance = VK_NULL_HANDLE; //!< Vulkan instance.
  VkPhysicalDevice _phy_dev  = VK_NULL_HANDLE; //!< Physical device.

  /// Device table for vulkan functions.
  VolkDeviceTable _device_table;

  VkPhysicalDeviceProperties       _dev_props     = {}; //!< Dev props.
  VkPhysicalDeviceFeatures2KHR     _dev_features  = {}; //!< Dev features.
  VkPhysicalDeviceMemoryProperties _dev_mem_props = {}; //!< Dev mem props.

  VkQueue _graphics_queue = VK_NULL_HANDLE; //!< Queue for graphics.
  VkQueue _compute_queue  = VK_NULL_HANDLE; //!< Queue for compute.
  VkQueue _transfer_queue = VK_NULL_HANDLE; //!< Queue for transfer.

  // clang-format off
  /// Graphics queue family index.
  uint32_t _graphics_queue_family = VK_QUEUE_FAMILY_IGNORED;
  /// Compute queue family index.
  uint32_t _compute_queue_family  = VK_QUEUE_FAMILY_IGNORED;
  /// Transfer queue family index.
  uint32_t _transfer_queue_family = VK_QUEUE_FAMILY_IGNORED;
  // clang-format on

  uint32_t _graphics_queue_index  = 0; //!< Index of the graphics queue.
  uint32_t _compute_queue_index   = 0; //!< Index of the compute queue.
  uint32_t _transfer_queue_index  = 0; //!< Index of the transfer queue.
  unsigned _universal_queue_index = 1; //!< Index of a universal queue.

  bool _supports_vulkan_11       = false; //!< Supports VK >= 1.1
  bool _supports_surface_caps_2  = false; //!< Supports surf caps 2.
  bool _supports_phy_dev_props_2 = false; //!< Supports dev props 2.
  bool _supports_external        = false; //!< Supports external props.

#ifdef VULKAN_DEBUG
  bool _supports_debug_utils = false; //!< Supports debug utils.
  // clang-format off
  /// Callback for debugging.
  VkDebugReportCallbackEXT _debug_callback  = VK_NULL_HANDLE;
  /// Messenger for debugging.
  VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
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
    uint32_t         num_layers) -> bool;

  /// Creates the vulkan instance with \p ins_extensions extensions and \p
  /// num_extensions number of extensions. Returns false if the instance could
  /// not be created.
  /// \param ins_extensions The instance extensions.
  /// \param num_extensions The number of extensions.
  auto
  create_instance(const char** ins_extensions, uint32_t num_extension) -> bool;

  /// Creates the queue information for device creation, returning a vector of
  /// filled VkDeviceQueueCreateInfo for each queue, and a vector of proritities
  /// for each of the queues. Since the VkDeviceQueueCreateInfo's in the vector
  /// refere to the priorities in the second vector, the second vector needs to
  /// be kept around until the device is created.
  auto create_queue_info() const
    -> std::tuple<std::vector<VkDeviceQueueCreateInfo>, std::vector<float>>;

  /// Returns the flags required for the device.
  auto required_flags() const -> VkQueueFlags {
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
  auto select_physical_device(VkSurfaceKHR surface) -> bool;

  /// Selects all queue families. This first tries to select separate queue
  /// families, and then ensures that all queue families have been selected, and
  /// sets the values of the indices of the queues in the queue families.
  ///
  /// \param queue_props A vector of queue properties.
  auto
  select_queue_families(const std::vector<VkQueueFamilyProperties>& queue_props)
    -> void;

  /// Finds the indices for the queues.

  /// Tries to select as many separate queue families as possible. To ensure
  /// that all
  ///
  /// \param queue_props A vector of queue properties.
  auto try_select_separate_queue_families(
    const std::vector<VkQueueFamilyProperties>& queue_props) -> void;

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
  auto validate_extensions(const char** dev_extensions, uint32_t num_extensions)
    -> bool;

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
  auto validate_layers(const char** dev_layers, uint32_t num_layers) -> bool;

  /// Destroys the context and assosciated resources.
  auto destroy() -> void;
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_CONTEXT_HPP