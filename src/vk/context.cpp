//==--- glow/src/vk/platform/context.cpp ------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  context.cpp
/// \brief This file defines the implemenation for the vulkan context.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/vk/context.hpp>
#include <algorithm>
#include <cstring>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#ifndef _WIN32
#  include <dlfcn.h>
#elif defined(_WIN32)
#  include <windows.h>
#endif

namespace ripple::glow::vk {

//==--- [con/destruction] --------------------------------------------------==//

Context::~Context() {}

//==--- [static public] ----------------------------------------------------==//

auto Context::get_application_info() -> const VkApplicationInfo& {
  // clang-format off
  static const VkApplicationInfo info = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    nullptr,
    "Glow",
    0,
    "Glow",
    0,
    VK_API_VERSION_1_2};
  return info;
  // clang-format on
}

static bool       loader_is_initialzied;
static std::mutex loader_init_lock;

auto Context::init_loader(PFN_vkGetInstanceProcAddr addr) -> bool {
  std::lock_guard<std::mutex> guard(loader_init_lock);
  if (loader_is_initialzied) {
    return true;
  }

  if (!addr) {
#if !defined(_WIN32)
    static void* module;
    if (!module) {
      const char* vulkan_path = getenv("GLOW_VULKAN_LIBRARY");
      if (vulkan_path) {
        module = dlopen(vulkan_path, RTLD_LOCAL | RTLD_LAZY);
      }
#  if defined(__APPLE__)
      if (!module) {
        module = dlopen("libvulkan.1.dylib", RTLD_LOCAL | RTLD_LAZY);
      }
#  else
      if (!module) {
        module = dlopen("libvulkan.so.1", RTLD_LOCAL | RTLD_LAZY);
      }
      if (!module) {
        module = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_LAZY);
      }
#  endif
      if (!module) {
        return false;
      }
    }

    addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      dlsym(module, "vkGetInstanceProcAddr"));
    if (!addr) {
      return false;
    }
#else
    static HMODULE module;
    if (!module) {
      module = LoadLibraryA("vulkan-1.dll");
      if (!module) {
        return false;
      }
    }

    // Ugly pointer warning workaround.
    auto ptr = GetProcAddress(module, "vkGetInstanceProcAddr");
    static_assert(sizeof(ptr) == sizeof(addr), "Mismatch pointer type.");
    memcpy(&addr, &ptr, sizeof(ptr));

    if (!addr) {
      return false;
    }
#endif
  }

  volkInitializeCustom(addr);
  loader_is_initialzied = true;
  return true;
}

//==--- [public] -----------------------------------------------------------==//

auto Context::init_instance_and_device(
  const char** ins_extensions,
  uint32_t     num_ins_extensions,
  const char** dev_extensions,
  uint32_t     num_dev_extensions) -> bool {
  destroy();

  if (!create_instance(ins_extensions, num_ins_extensions)) {
    destroy();
    log_error("Failed to create vulkan instance!");
    return false;
  }

  auto dev_features   = VkPhysicalDeviceFeatures();
  auto created_device = create_device(
    VK_NULL_HANDLE,
    VK_NULL_HANDLE,
    dev_extensions,
    num_dev_extensions,
    nullptr,
    0,
    &dev_features);

  if (!created_device) {
    destroy();
    log_error("Failed to create vulkan device!");
    return false;
  }

  return true;
}

auto Context::create_instance(
  const char** ins_extensions, uint32_t num_extensions) -> bool {
  _features.supports_vulkan_11_instance =
    volkGetInstanceVersion() >= VK_API_VERSION_1_1;

  auto                 app_info = get_application_info();
  VkInstanceCreateInfo info     = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  info.pApplicationInfo         = &get_application_info();

  auto instance_exts   = std::vector<const char*>();
  auto instance_layers = std::vector<const char*>();
  for (uint32_t i = 0; i < num_extensions; i++) {
    instance_exts.push_back(ins_extensions[i]);
  }

  uint32_t num_exts = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &num_exts, nullptr);
  auto queried_extensions = std::vector<VkExtensionProperties>(num_exts);
  if (num_exts) {
    vkEnumerateInstanceExtensionProperties(
      nullptr, &num_exts, queried_extensions.data());
  }

  uint32_t num_layers = 0;
  vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
  auto queried_layers = std::vector<VkLayerProperties>(num_layers);
  if (num_layers) {
    vkEnumerateInstanceLayerProperties(&num_layers, queried_layers.data());
  }

  const auto has_extension = [&](const char* name) -> bool {
    auto itr = std::find_if(
      std::begin(queried_extensions),
      std::end(queried_extensions),
      [name](const VkExtensionProperties& e) -> bool {
        return std::strcmp(e.extensionName, name) == 0;
      });
    return itr != end(queried_extensions);
  };

  for (uint32_t i = 0; i < num_extensions; i++) {
    if (!has_extension(ins_extensions[i])) {
      return false;
    }
  }

  if (has_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
    _features.supports_physical_device_properties2 = true;
    instance_exts.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }

  if (
    _features.supports_physical_device_properties2 &&
    has_extension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) &&
    has_extension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME)) {
    instance_exts.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    instance_exts.push_back(
      VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    _features.supports_external = true;
  }

  if (has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    instance_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    _features.supports_debug_utils = true;
  }

  const auto ins_ext_end = ins_extensions + num_extensions;
  auto itr = std::find_if(ins_extensions, ins_ext_end, [](const char* name) {
    return std::strcmp(name, VK_KHR_SURFACE_EXTENSION_NAME) == 0;
  });
  bool has_surface_extension = itr != (ins_ext_end);

  if (
    has_surface_extension &&
    has_extension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
    instance_exts.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    _features.supports_surface_capabilities2 = true;
  }

#ifdef VULKAN_DEBUG
  const auto has_layer = [&](const char* name) -> bool {
    auto layer_itr = std::find_if(
      std::begin(queried_layers),
      std::end(queried_layers),
      [name](const VkLayerProperties& e) -> bool {
        return strcmp(e.layerName, name) == 0;
      });
    return layer_itr != std::end(queried_layers);
  };

  if (
    !_features.supports_debug_utils &&
    has_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
    instance_exts.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }

  if (getenv("GLOW_VULKAN_NO_VALIDATION")) {
    _force_no_validation = true;
  }

  if (!_force_no_validation && has_layer("VK_LAYER_KHRONOS_validation")) {
    instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    log_info("Enabling VK_LAYER_KHRONOS_validation.");
  } else if (
    !_force_no_validation && has_layer("VK_LAYER_LUNARG_standard_validation")) {
    instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    log_info("Enabling VK_LAYER_LUNARG_standard_validation.");
  }
#endif

  info.enabledExtensionCount   = instance_exts.size();
  info.ppEnabledExtensionNames = instance_exts.empty() ? nullptr
                                                       : instance_exts.data();
  info.enabledLayerCount   = instance_layers.size();
  info.ppEnabledLayerNames = instance_layers.empty() ? nullptr
                                                     : instance_layers.data();

  for (auto* ext_name : instance_exts) {
    log_info("Enabling instance extension: %s.", ext_name);
  }

  if (vkCreateInstance(&info, nullptr, &_instance) != VK_SUCCESS) {
    return false;
  }

  volkLoadInstance(_instance);

#ifdef VULKAN_DEBUG
  if (_features.supports_debug_utils) {
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debug_info.pfnUserCallback = vulkan_messenger_cb;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    debug_info.pUserData = this;

    vkCreateDebugUtilsMessengerEXT(
      _instance, &debug_info, nullptr, &_debug_messenger);
  } else if (has_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
    VkDebugReportCallbackCreateInfoEXT debug_info = {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                       VK_DEBUG_REPORT_WARNING_BIT_EXT |
                       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_info.pfnCallback = vulkan_debug_cb;
    debug_info.pUserData   = this;
    vkCreateDebugReportCallbackEXT(
      _instance, &debug_info, nullptr, &_debug_callback);
  }
#endif

  return true;
} // namespace ripple::glow::vk

auto Context::create_device(
  VkPhysicalDevice          dev,
  VkSurfaceKHR              surface,
  const char**              dev_req_extensions,
  uint32_t                  num_req_extensions,
  const char**              dev_req_layers,
  uint32_t                  num_req_layers,
  VkPhysicalDeviceFeatures* req_features) -> bool {
  _phy_dev = dev;
  if (_phy_dev == VK_NULL_HANDLE) {
    uint32_t dev_count = 0;
    if (
      vkEnumeratePhysicalDevices(_instance, &dev_count, nullptr) !=
      VK_SUCCESS) {
      return false;
    }

    if (dev_count == 0) {
      return false;
    }

    auto devices = std::vector<VkPhysicalDevice>(dev_count);
    if (
      vkEnumeratePhysicalDevices(_instance, &dev_count, devices.data()) !=
      VK_SUCCESS) {
      return false;
    }

    for (auto& d : devices) {
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(d, &props);
      log_info("Found Vulkan Device (GPU): %s", props.deviceName);
      log_info(
        "API: %u.%u.%u",
        VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion));
      log_info(
        "Driver: %u.%u.%u",
        VK_VERSION_MAJOR(props.driverVersion),
        VK_VERSION_MINOR(props.driverVersion),
        VK_VERSION_PATCH(props.driverVersion));
    }

    // Try and find the device that we actually want:
    const char* dev_index = getenv("GLOW_VULKAN_DEVICE_INDEX");
    if (dev_index) {
      unsigned index = strtoul(dev_index, nullptr, 0);
      if (index < dev_count) {
        _phy_dev = devices[index];
      }
    }
    // Fallback to the first device that we find:
    if (_phy_dev == VK_NULL_HANDLE) {
      _phy_dev = devices.front();
    }
  }

  uint32_t num_exts = 0;
  vkEnumerateDeviceExtensionProperties(_phy_dev, nullptr, &num_exts, nullptr);
  auto queried_extensions = std::vector<VkExtensionProperties>(num_exts);
  if (num_exts) {
    vkEnumerateDeviceExtensionProperties(
      _phy_dev, nullptr, &num_exts, queried_extensions.data());
  }

  uint32_t num_layers = 0;
  vkEnumerateDeviceLayerProperties(_phy_dev, &num_layers, nullptr);
  auto queried_layers = std::vector<VkLayerProperties>(num_layers);
  if (num_layers) {
    vkEnumerateDeviceLayerProperties(
      _phy_dev, &num_layers, queried_layers.data());
  }

  const auto has_extension = [&](const char* name) -> bool {
    auto itr = std::find_if(
      std::begin(queried_extensions),
      std::end(queried_extensions),
      [name](const VkExtensionProperties& e) -> bool {
        return strcmp(e.extensionName, name) == 0;
      });
    return itr != std::end(queried_extensions);
  };

  const auto has_layer = [&](const char* name) -> bool {
    auto itr = std::find_if(
      std::begin(queried_layers),
      std::end(queried_layers),
      [name](const VkLayerProperties& e) -> bool {
        return strcmp(e.layerName, name) == 0;
      });
    return itr != std::end(queried_layers);
  };

  for (uint32_t i = 0; i < num_req_extensions; i++) {
    if (!has_extension(dev_req_extensions[i])) {
      return false;
    }
  }

  for (uint32_t i = 0; i < num_req_layers; i++) {
    if (!has_layer(dev_req_layers[i])) {
      return false;
    }
  }

  vkGetPhysicalDeviceProperties(_phy_dev, &_dev_props);
  vkGetPhysicalDeviceMemoryProperties(_phy_dev, &_dev_mem_props);

  log_info("Selected Vulkan Device: %s", _dev_props.deviceName);

  if (_dev_props.apiVersion >= VK_API_VERSION_1_2) {
    _features.supports_vulkan_12_device = _features.supports_vulkan_12_instance;
    _features.supports_vulkan_11_device = _features.supports_vulkan_11_instance;
    log_info("Device supports Vulkan 1.2.");
  } else if (_dev_props.apiVersion >= VK_API_VERSION_1_1) {
    _features.supports_vulkan_11_device = _features.supports_vulkan_11_instance;
    log_info("Device supports Vulkan 1.1.");
  } else if (_dev_props.apiVersion >= VK_API_VERSION_1_0) {
    _features.supports_vulkan_11_device = false;
    log_info("Device supports Vulkan 1.0.");
  }

  uint32_t num_queues;
  vkGetPhysicalDeviceQueueFamilyProperties(_phy_dev, &num_queues, nullptr);
  auto queue_props = std::vector<VkQueueFamilyProperties>(num_queues);
  vkGetPhysicalDeviceQueueFamilyProperties(
    _phy_dev, &num_queues, queue_props.data());

  for (unsigned i = 0; i < num_queues; i++) {
    VkBool32 surf_supported = surface == VK_NULL_HANDLE;
    if (surface != VK_NULL_HANDLE) {
      vkGetPhysicalDeviceSurfaceSupportKHR(
        _phy_dev, i, surface, &surf_supported);
    }

    static const VkQueueFlags required =
      VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
    if (
      surf_supported && ((queue_props[i].queueFlags & required) == required)) {
      _graphics_queue_family = i;
      break;
    }
  }

  for (unsigned i = 0; i < num_queues; i++) {
    static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
    if (
      i != _graphics_queue_family &&
      (queue_props[i].queueFlags & required) == required) {
      _compute_queue_family = i;
      break;
    }
  }

  for (unsigned i = 0; i < num_queues; i++) {
    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
    if (
      i != _graphics_queue_family && i != _compute_queue_family &&
      (queue_props[i].queueFlags & required) == required) {
      _transfer_queue_family = i;
      break;
    }
  }

  if (_transfer_queue_family == VK_QUEUE_FAMILY_IGNORED) {
    for (unsigned i = 0; i < num_queues; i++) {
      static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
      if (
        i != _graphics_queue_family &&
        (queue_props[i].queueFlags & required) == required) {
        _transfer_queue_family = i;
        break;
      }
    }
  }

  if (_graphics_queue_family == VK_QUEUE_FAMILY_IGNORED) {
    return false;
  }

  unsigned universal_queue_index = 1;
  uint32_t graphics_queue_index  = 0;
  uint32_t compute_queue_index   = 0;
  uint32_t transfer_queue_index  = 0;

  if (_compute_queue_family == VK_QUEUE_FAMILY_IGNORED) {
    _compute_queue_family = _graphics_queue_family;
    compute_queue_index   = std::min(
      queue_props[_graphics_queue_family].queueCount - 1,
      universal_queue_index);
    universal_queue_index++;
  }

  if (_transfer_queue_family == VK_QUEUE_FAMILY_IGNORED) {
    _transfer_queue_family = _graphics_queue_family;
    transfer_queue_index   = std::min(
      queue_props[_graphics_queue_family].queueCount - 1,
      universal_queue_index);
    universal_queue_index++;
  } else if (_transfer_queue_family == _compute_queue_family) {
    transfer_queue_index =
      std::min(queue_props[_compute_queue_family].queueCount - 1, 1u);
  }

  static const float graphics_queue_prio = 0.5f;
  static const float compute_queue_prio  = 1.0f;
  static const float transfer_queue_prio = 1.0f;
  float              prio[3]             = {
    graphics_queue_prio, compute_queue_prio, transfer_queue_prio};

  unsigned                queue_family_count = 0;
  VkDeviceQueueCreateInfo queue_info[3]      = {};
  VkDeviceCreateInfo      device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pQueueCreateInfos       = queue_info;

  queue_info[queue_family_count].sType =
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[queue_family_count].queueFamilyIndex = _graphics_queue_family;
  queue_info[queue_family_count].queueCount       = std::min(
    universal_queue_index, queue_props[_graphics_queue_family].queueCount);
  queue_info[queue_family_count].pQueuePriorities = prio;
  queue_family_count++;

  if (_compute_queue_family != _graphics_queue_family) {
    queue_info[queue_family_count].sType =
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = _compute_queue_family;
    queue_info[queue_family_count].queueCount       = std::min(
      _transfer_queue_family == _compute_queue_family ? 2u : 1u,
      queue_props[_compute_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio + 1;
    queue_family_count++;
  }

  if (
    _transfer_queue_family != _graphics_queue_family &&
    _transfer_queue_family != _compute_queue_family) {
    queue_info[queue_family_count].sType =
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = _transfer_queue_family;
    queue_info[queue_family_count].queueCount       = 1;
    queue_info[queue_family_count].pQueuePriorities = prio + 2;
    queue_family_count++;
  }

  device_info.queueCreateInfoCount = queue_family_count;

  auto enabled_extensions = std::vector<const char*>();
  auto enabled_layers     = std::vector<const char*>();

  for (uint32_t i = 0; i < num_req_extensions; i++) {
    enabled_extensions.push_back(dev_req_extensions[i]);
  }
  for (uint32_t i = 0; i < num_req_layers; i++) {
    enabled_layers.push_back(dev_req_layers[i]);
  }

  if (has_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)) {
    _features.supports_get_memory_requirements2 = true;
    enabled_extensions.push_back(
      VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  }

  if (
    _features.supports_get_memory_requirements2 &&
    has_extension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)) {
    _features.supports_dedicated = true;
    enabled_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME)) {
    _features.supports_image_format_list = true;
    enabled_extensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)) {
    _features.supports_get_memory_requirements2 = true;
    enabled_extensions.push_back(
      VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  }

  if (
    _features.supports_get_memory_requirements2 &&
    has_extension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)) {
    _features.supports_dedicated = true;
    enabled_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME)) {
    _features.supports_image_format_list = true;
    enabled_extensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
  }

#ifdef _WIN32
  if (
    _features.supports_surface_capabilities2 &&
    has_extension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME)) {
    _features.supports_full_screen_exclusive = true;
    enabled_extensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
  }
#endif

#ifdef VULKAN_DEBUG
  if (has_extension(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME)) {
    _features.supports_nv_device_diagnostic_checkpoints = true;
    enabled_extensions.push_back(
      VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
  }
#endif

#ifdef _WIN32
  _features.supports_external = false;
#else
  if (
    _features.supports_external && _features.supports_dedicated &&
    has_extension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME) &&
    has_extension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) &&
    has_extension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME) &&
    has_extension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME)) {
    _features.supports_external = true;
    enabled_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    enabled_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    enabled_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    enabled_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
  } else {
    _features.supports_external = false;
  }
#endif

  if (has_extension(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME)) {
    _features.supports_update_template = true;
    enabled_extensions.push_back(
      VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_MAINTENANCE1_EXTENSION_NAME)) {
    _features.supports_maintenance_1 = true;
    enabled_extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME)) {
    _features.supports_maintenance_2 = true;
    enabled_extensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME)) {
    _features.supports_maintenance_3 = true;
    enabled_extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME)) {
    _features.supports_bind_memory2 = true;
    enabled_extensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME)) {
    _features.supports_draw_indirect_count = true;
    enabled_extensions.push_back(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME)) {
    _features.supports_draw_parameters = true;
    enabled_extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
  }

  if (has_extension(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME)) {
    enabled_extensions.push_back(
      VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
  }

  if (has_extension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME)) {
    _features.supports_conservative_rasterization = true;
    enabled_extensions.push_back(
      VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
  }

  VkPhysicalDeviceFeatures2KHR features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
  _features.storage_8bit_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};
  _features.storage_16bit_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR};
  _features.float16_int8_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR};
  _features.multiview_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR};
  _features.imageless_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR};
  _features.subgroup_size_control_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT};
  _features.compute_shader_derivative_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV};
  _features.host_query_reset_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT};
  _features.demote_to_helper_invocation_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT};
  _features.scalar_block_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT};
  _features.ubo_std_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES_KHR};
  _features.timeline_semaphore_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR};
  _features.descriptor_indexing_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
  _features.performance_query_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR};
  _features.sampler_ycbcr_conversion_features = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES_KHR};
  void** ppNext = &features.pNext;

  bool has_pdf2 = _features.supports_physical_device_properties2 ||
                  (_features.supports_vulkan_11_instance &&
                   _features.supports_vulkan_11_device);

  if (has_pdf2) {
    if (has_extension(VK_KHR_8BIT_STORAGE_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
      *ppNext = &_features.storage_8bit_features;
      ppNext  = &_features.storage_8bit_features.pNext;
    }

    if (has_extension(VK_KHR_16BIT_STORAGE_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
      *ppNext = &_features.storage_16bit_features;
      ppNext  = &_features.storage_16bit_features.pNext;
    }

    if (has_extension(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
      *ppNext = &_features.float16_int8_features;
      ppNext  = &_features.float16_int8_features.pNext;
    }

    if (has_extension(VK_KHR_MULTIVIEW_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
      *ppNext = &_features.multiview_features;
      ppNext  = &_features.multiview_features.pNext;
    }

    if (has_extension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
      *ppNext = &_features.subgroup_size_control_features;
      ppNext  = &_features.subgroup_size_control_features.pNext;
    }

    if (has_extension(VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME)) {
      enabled_extensions.push_back(
        VK_NV_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
      *ppNext = &_features.compute_shader_derivative_features;
      ppNext  = &_features.compute_shader_derivative_features.pNext;
    }

    if (has_extension(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
      *ppNext = &_features.host_query_reset_features;
      ppNext  = &_features.host_query_reset_features.pNext;
    }

    if (has_extension(
          VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME)) {
      enabled_extensions.push_back(
        VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);
      *ppNext = &_features.demote_to_helper_invocation_features;
      ppNext  = &_features.demote_to_helper_invocation_features.pNext;
    }

    if (has_extension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
      *ppNext = &_features.scalar_block_features;
      ppNext  = &_features.scalar_block_features.pNext;
    }

    if (has_extension(VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME)) {
      enabled_extensions.push_back(
        VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME);
      *ppNext = &_features.ubo_std_features;
      ppNext  = &_features.ubo_std_features.pNext;
    }

#ifdef VULKAN_DEBUG
    bool use_timeline_semaphore = _force_no_validation;
#else
    constexpr bool use_timeline_semaphore = true;
#endif
    if (
      use_timeline_semaphore &&
      has_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
      *ppNext = &_features.timeline_semaphore_features;
      ppNext  = &_features.timeline_semaphore_features.pNext;
    }

    if (
      _features.supports_maintenance_3 &&
      has_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
      *ppNext = &_features.descriptor_indexing_features;
      ppNext  = &_features.descriptor_indexing_features.pNext;
    }

    if (has_extension(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME)) {
      enabled_extensions.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
      *ppNext = &_features.performance_query_features;
      ppNext  = &_features.performance_query_features.pNext;
    }

    if (
      _features.supports_bind_memory2 &&
      _features.supports_get_memory_requirements2 &&
      has_extension(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME)) {
      enabled_extensions.push_back(
        VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
      *ppNext = &_features.sampler_ycbcr_conversion_features;
      ppNext  = &_features.sampler_ycbcr_conversion_features.pNext;
    }

#if 0
		if (has_extension(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME)) {
			enabled_extensions.push_back(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME);
			*ppNext = &_features.imageless_features;
			ppNext  = &_features.imageless_features.pNext;
		}
#endif
  }

  if (
    _features.supports_vulkan_11_device &&
    _features.supports_vulkan_11_instance) {
    vkGetPhysicalDeviceFeatures2(_phy_dev, &features);
  } else if (_features.supports_physical_device_properties2) {
    vkGetPhysicalDeviceFeatures2KHR(_phy_dev, &features);
  } else {
    vkGetPhysicalDeviceFeatures(_phy_dev, &features.features);
  }

  // Enable some potentially important device features:
  {
    VkPhysicalDeviceFeatures enabled_features = *req_features;
    if (features.features.textureCompressionETC2)
      enabled_features.textureCompressionETC2 = VK_TRUE;
    if (features.features.textureCompressionBC)
      enabled_features.textureCompressionBC = VK_TRUE;
    if (features.features.textureCompressionASTC_LDR)
      enabled_features.textureCompressionASTC_LDR = VK_TRUE;
    if (features.features.fullDrawIndexUint32)
      enabled_features.fullDrawIndexUint32 = VK_TRUE;
    if (features.features.imageCubeArray)
      enabled_features.imageCubeArray = VK_TRUE;
    if (features.features.fillModeNonSolid)
      enabled_features.fillModeNonSolid = VK_TRUE;
    if (features.features.independentBlend)
      enabled_features.independentBlend = VK_TRUE;
    if (features.features.sampleRateShading)
      enabled_features.sampleRateShading = VK_TRUE;
    if (features.features.fragmentStoresAndAtomics)
      enabled_features.fragmentStoresAndAtomics = VK_TRUE;
    if (features.features.shaderStorageImageExtendedFormats)
      enabled_features.shaderStorageImageExtendedFormats = VK_TRUE;
    if (features.features.shaderStorageImageMultisample)
      enabled_features.shaderStorageImageMultisample = VK_TRUE;
    if (features.features.largePoints)
      enabled_features.largePoints = VK_TRUE;
    if (features.features.shaderInt16)
      enabled_features.shaderInt16 = VK_TRUE;
    if (features.features.shaderInt64)
      enabled_features.shaderInt64 = VK_TRUE;

    if (features.features.shaderSampledImageArrayDynamicIndexing)
      enabled_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    if (features.features.shaderUniformBufferArrayDynamicIndexing)
      enabled_features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
    if (features.features.shaderStorageBufferArrayDynamicIndexing)
      enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
    if (features.features.shaderStorageImageArrayDynamicIndexing)
      enabled_features.shaderStorageImageArrayDynamicIndexing = VK_TRUE;

    features.features          = enabled_features;
    _features.enabled_features = enabled_features;
  }

  if (_features.supports_physical_device_properties2) {
    device_info.pNext = &features;
  } else {
    device_info.pEnabledFeatures = &features.features;
  }

#ifdef VULKAN_DEBUG
  if (!_force_no_validation && has_layer("VK_LAYER_KHRONOS_validation")) {
    enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
  } else if (
    !_force_no_validation && has_layer("VK_LAYER_LUNARG_standard_validation")) {
    enabled_layers.push_back("VK_LAYER_LUNARG_standard_validation");
  }
#endif

  if (
    _features.supports_external &&
    has_extension(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME)) {
    _features.supports_external_memory_host = true;
    enabled_extensions.push_back(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
  }

  // Only need GetPhysicalDeviceProperties2 for Vulkan 1.1-only code, so don't
  // bother getting KHR variant.
  _features.subgroup_properties = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
  _features.host_memory_properties = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT};
  _features.subgroup_size_control_properties = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT};
  _features.descriptor_indexing_properties = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT};
  // clang-format off
  _features.conservative_rasterization_properties = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT
  };
  // clang-format on

  VkPhysicalDeviceProperties2 props = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  ppNext  = &props.pNext;
  *ppNext = &_features.subgroup_properties;
  ppNext  = &_features.subgroup_properties.pNext;

  if (_features.supports_external_memory_host) {
    *ppNext = &_features.host_memory_properties;
    ppNext  = &_features.host_memory_properties.pNext;
  }

  if (has_extension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) {
    *ppNext = &_features.subgroup_size_control_properties;
    ppNext  = &_features.subgroup_size_control_properties.pNext;
  }

  if (
    _features.supports_maintenance_3 &&
    has_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)) {
    *ppNext = &_features.descriptor_indexing_properties;
    ppNext  = &_features.descriptor_indexing_properties.pNext;
  }

  if (_features.supports_conservative_rasterization) {
    *ppNext = &_features.conservative_rasterization_properties;
    ppNext  = &_features.conservative_rasterization_properties.pNext;
  }

  if (
    _features.supports_vulkan_11_instance &&
    _features.supports_vulkan_11_device) {
    vkGetPhysicalDeviceProperties2(_phy_dev, &props);
  }

  device_info.enabledExtensionCount = enabled_extensions.size();
  device_info.enabledLayerCount     = enabled_layers.size();
  device_info.ppEnabledExtensionNames =
    enabled_extensions.empty() ? nullptr : enabled_extensions.data();
  device_info.ppEnabledLayerNames =
    enabled_layers.empty() ? nullptr : enabled_layers.data();

  for (auto* enabled_extension : enabled_extensions) {
    log_info("Enabling device extension: %s.", enabled_extension);
  }

  if (vkCreateDevice(_phy_dev, &device_info, nullptr, &_device) != VK_SUCCESS) {
    return false;
  }

  volkLoadDeviceTable(&_device_table, _device);
  _device_table.vkGetDeviceQueue(
    _device, _graphics_queue_family, graphics_queue_index, &_graphics_queue);
  _device_table.vkGetDeviceQueue(
    _device, _compute_queue_family, compute_queue_index, &_compute_queue);
  _device_table.vkGetDeviceQueue(
    _device, _transfer_queue_family, transfer_queue_index, &_transfer_queue);

  check_descriptor_indexing_features();
  return true;
}

void Context::check_descriptor_indexing_features() {
  auto&      f    = _features.descriptor_indexing_features;
  const auto cond = f.descriptorBindingSampledImageUpdateAfterBind &&
                    f.descriptorBindingPartiallyBound &&
                    f.runtimeDescriptorArray &&
                    f.shaderSampledImageArrayNonUniformIndexing;

  if (cond) {
    _features.supports_descriptor_indexing = true;
  }
}

auto Context::destroy() -> void {
  if (_device != VK_NULL_HANDLE) {
    _device_table.vkDeviceWaitIdle(_device);
  }

#ifdef VULKAN_DEBUG
  if (debug_callback) {
    vkDestroyDebugReportCallbackEXT(_instance, _debug_callback, nullptr);
  }
  if (debug_messenger) {
    vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
  }
  _debug_callback  = VK_NULL_HANDLE;
  _debug_messenger = VK_NULL_HANDLE;
#endif

  if (_owned_device && _device != VK_NULL_HANDLE) {
    _device_table.vkDestroyDevice(_device, nullptr);
  }
  if (_owned_instance && _instance != VK_NULL_HANDLE) {
    vkDestroyInstance(_instance, nullptr);
  }
}

} // namespace ripple::glow::vk