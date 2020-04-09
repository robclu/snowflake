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

//==--- [device features] --------------------------------------------------==//

/// Defines the features of the device.
struct DeviceFeatures {
  bool supports_physical_device_properties2      = false;
  bool supports_external                         = false;
  bool supports_dedicated                        = false;
  bool supports_image_format_list                = false;
  bool supports_debug_marker                     = false;
  bool supports_debug_utils                      = false;
  bool supports_mirror_clamp_to_edge             = false;
  bool supports_google_display_timing            = false;
  bool supports_nv_device_diagnostic_checkpoints = false;
  bool supports_external_memory_host             = false;
  bool supports_surface_capabilities2            = false;
  bool supports_full_screen_exclusive            = false;
  bool supports_update_template                  = false;
  bool supports_maintenance_1                    = false;
  bool supports_maintenance_2                    = false;
  bool supports_maintenance_3                    = false;
  bool supports_descriptor_indexing              = false;
  bool supports_conservative_rasterization       = false;
  bool supports_bind_memory2                     = false;
  bool supports_get_memory_requirements2         = false;
  bool supports_draw_indirect_count              = false;
  bool supports_draw_parameters                  = false;
  bool supports_vulkan_11_instance               = true;
  bool supports_vulkan_11_device                 = true;

  // clang-format off

  /// Properties for device subgroup.
  VkPhysicalDeviceSubgroupProperties              subgroup_properties    = {};
  /// Properties for 8 bit storage.
  VkPhysicalDevice8BitStorageFeaturesKHR          storage_8bit_features  = {};
  /// Properties for 16 bit sotrage.
  VkPhysicalDevice16BitStorageFeaturesKHR         storage_16bit_features = {};
  /// Properties for 16 bit float storage.
  VkPhysicalDeviceFloat16Int8FeaturesKHR          float16_int8_features  = {};
  /// Featrues for the physical device.
  VkPhysicalDeviceFeatures                        enabled_features       = {};
  /// External memory properties for the host.
  VkPhysicalDeviceExternalMemoryHostPropertiesEXT host_memory_properties = {};
  /// Features for multi-view.
  VkPhysicalDeviceMultiviewFeaturesKHR            multiview_features     = {};
  /// Features for imageless framebuffers.
  VkPhysicalDeviceImagelessFramebufferFeaturesKHR imageless_features     = {};
  /// Scalar block layout features.
  VkPhysicalDeviceScalarBlockLayoutFeaturesEXT    scalar_block_features  = {};

  /// Timeline semaphore features.
  VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timeline_semaphore_features = {};
  /// Performance query features.
  VkPhysicalDevicePerformanceQueryFeaturesKHR  performance_query_features  = {};
  /// Device host query reset features.
  VkPhysicalDeviceHostQueryResetFeaturesEXT    host_query_reset_features   = {};

  /// Uniform buffer standard layout features.
  VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR ubo_std_features = {};

  /// Subgroup size control fetures.
  VkPhysicalDeviceSubgroupSizeControlFeaturesEXT
    subgroup_size_control_features        = {};
  /// Subgroup size control properties.
  VkPhysicalDeviceSubgroupSizeControlPropertiesEXT
    subgroup_size_control_properties      = {};
  /// Compute shader derivative featuers.
  VkPhysicalDeviceComputeShaderDerivativesFeaturesNV
    compute_shader_derivative_features    = {};
  /// Device shared demote to helper features.
  VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT
    demote_to_helper_invocation_features  = {};
  /// Descriptor indexing properties.
  VkPhysicalDeviceDescriptorIndexingPropertiesEXT
    descriptor_indexing_properties        = {};
  /// Conservative rasterization properties.
  VkPhysicalDeviceConservativeRasterizationPropertiesEXT
    conservative_rasterization_properties = {};
  /// Sample conversion features for Ycbcr.
  VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR
    sampler_ycbcr_conversion_features     = {};
  /// Descriptor indexing features.
  VkPhysicalDeviceDescriptorIndexingFeaturesEXT 
    descriptor_indexing_features          = {};

  // clang-format on
};

/// The Context for vulkan which owns a VkInstance and a VkDevice.
class Context {
  /// Defines the type of device features.
  using dev_features_t = DeviceFeatures;

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

  /// Initializes the instance and the device, with \p ins_extensions for the
  /// instance, and \p dev_extensions for the device. This returns false if the
  /// initialization was not successful.
  auto init_instance_and_device(
    const char** ins_extensions,
    uint32_t     num_ins_extensions,
    const char** dev_extensions,
    uint32_t     num_dev_extesions) -> bool;

 private:
  dev_features_t   _features;                  //!< Device features.
  VkDevice         _device   = VK_NULL_HANDLE; //!< Vulkan device.
  VkInstance       _instance = VK_NULL_HANDLE; //!< Vulkan instance.
  VkPhysicalDevice _phy_dev  = VK_NULL_HANDLE; //!< Physical device.

  /// Device table for vulkan functions.
  VolkDeviceTable _device_table;

  VkPhysicalDeviceProperties       _dev_props     = {}; //!< Dev props.
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

  bool _owned_device        = true;  //!< If the device is owned.
  bool _owned_instance      = true;  //!< If the instance is owned.
  bool _force_no_validation = false; //!< Ensure no vulkan validation.

#ifdef VULKAN_DEBUG
  // clang-format off
  /// Callback for debugging.
  VkDebugReportCallbackEXT _debug_callback  = VK_NULL_HANDLE;
  /// Messenger for debugging.
  VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
    // clang-format on
#endif

  //==--- [methods] --------------------------------------------------------==//

  /// Creates the vulkan device with the \p dev pysical device, the \p surface,
  /// \p dev_extensions, \p dev_layers, and \p features.
  /// \param dev The physical device.
  /// \param surface The surface for the device.
  /// \param dev_extensions The required device extensions.
  /// \param num_extensions The number of extensions.
  /// \param dev_layers     The required device layers.
  /// \param num_layers     The number of layers.
  /// \param features       The required features for the device.
  auto create_device(
    VkPhysicalDevice          dev,
    VkSurfaceKHR              surface,
    const char**              dev_extesions,
    uint32_t                  num_extensions,
    const char**              dev_layers,
    uint32_t                  num_layers,
    VkPhysicalDeviceFeatures* features) -> bool;

  /// Checks if descriptor indexing features are present, and if so, then adds
  /// that they are to the feature struct.
  auto check_descriptor_indexing_features() -> void;

  /// Creates the vulkan instance with \p ins_extensions extensions and \p
  /// num_extensions number of extensions. Returns false if the instance could
  /// not be created.
  /// \param ins_extensions The instance extensions.
  /// \param num_extensions The number of extensions.
  auto
  create_instance(const char** ins_extensions, uint32_t num_extension) -> bool;

  /// Destoys the context and assosciated resources.
  auto destroy() -> void;
};

} // namespace ripple::glow::vk

#endif // RIPPLE_GLOW_VK_CONTEXT_HPP