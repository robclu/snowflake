//==--- snowflake/src/backend/vk/vulkan_context.cpp -------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  vulkan_context.cpp
/// \brief This file defines the implemenation for the vulkan context.
//
//==------------------------------------------------------------------------==//

#include <snowflake/backend/vk/vulkan_context.hpp>
#include <wrench/log/logger.hpp>
#include <algorithm>
#include <cstring>
#include <vector>

#ifndef _WIN32
  #include <dlfcn.h>
#elif defined(_WIN32)
  #include <windows.h>
#endif

namespace snowflake::backend {

//==--- [debug functions] --------------------------------------------------==//

#ifdef VULKAN_DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_messenger_cb(
  VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
  VkDebugUtilsMessageTypeFlagsEXT             message_type,
  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
  void*                                       p_user_data) {
  auto* context = static_cast<VulkanContext*>(p_user_data);

  switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
      if (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        wrench::log_error("Vulkan validation: {}", p_callback_data->pMessage);
      } else {
        wrench::log_error("Vulkan other: {}", p_callback_data->pMessage);
      }
      break;
    }

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
      if (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        wrench::log_warn("Vulkan validation: {}", p_callback_data->pMessage);
      } else {
        wrench::log_warn("Vulkan other: {}", p_callback_data->pMessage);
      }
      break;
    }

  #if 0
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
      if (message_ype == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        wrench::log_info("Vulkan validation: {}", p_callback_data->pMessage);
      } else {
        wrench::log_info("Vulkan other: {}", p_callback_data->pMessage);
      }
      break;
    }
  #endif
    default: return VK_FALSE;
  }

  bool log_object_names = false;
  for (uint32_t i = 0; i < p_callback_data->objectCount; i++) {
    auto* name = p_callback_data->pObjects[i].pObjectName;
    if (name) {
      log_object_names = true;
      break;
    }
  }

  if (log_object_names) {
    for (uint32_t i = 0; i < p_callback_data->objectCount; i++) {
      auto* name = p_callback_data->pObjects[i].pObjectName;
      wrench::log_info("Object {0}: {1}", i, name ? name : "N/A");
    }
  }
  return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_cb(
  VkDebugReportFlagsEXT      flags,
  VkDebugReportObjectTypeEXT object_type,
  uint64_t                   object,
  size_t                     location,
  int32_t                    message_code,
  const char*                p_layer_prefix,
  const char*                p_message,
  void*                      p_user_data) {
  auto* context = static_cast<VulkanContext*>(p_user_data);

  // False positives about lack of srcAccessMask/dstAccessMask.
  if (strcmp(p_layer_prefix, "DS") == 0 && message_code == 10) {
    return VK_FALSE;
  }

  // Demote to a warning, it's a false positive almost all the time.
  if (strcmp(p_layer_prefix, "DS") == 0 && message_code == 6) {
    flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT;
  }

  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    wrench::log_error("Vulkan: {0}: {1}", p_layer_prefix, p_message);
  } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    wrench::log_warn("Vulkan: {0}: {1}", p_layer_prefix, p_message);
  } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    wrench::log_warn("Vulkan performance: {0}: {1}", p_layer_prefix, p_message);
  } else {
    wrench::log_info("Vulkan: {0}: {1}", p_layer_prefix, p_message);
  }
  return VK_FALSE;
}
#endif

//==--- [con/destruction] --------------------------------------------------==//

VulkanContext::~VulkanContext() noexcept {
  destroy();
}

//==--- [static public] ----------------------------------------------------==//

auto VulkanContext::get_application_info() noexcept
  -> const VkApplicationInfo& {
  // clang-format off
  static const VkApplicationInfo info = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    nullptr,
    "Snowflake",
    0,
    "Snowflake",
    0,
    VK_API_VERSION_1_2};
  return info;
  // clang-format on
}
using Lock  = wrench::Spinlock;
using Guard = std::lock_guard<Lock>;

static bool loader_is_initialzied;
static Lock loader_init_lock;

auto VulkanContext::init_loader(PFN_vkGetInstanceProcAddr addr) noexcept
  -> bool {
  Guard guard(loader_init_lock);
  if (loader_is_initialzied) {
    return true;
  }

  if (!addr) {
#if !defined(_WIN32)
    static void* module;
    if (!module) {
      const char* vulkan_path = getenv("SNOWFLAKE_VULKAN_LIBRARY");
      if (vulkan_path) {
        module = dlopen(vulkan_path, RTLD_LOCAL | RTLD_LAZY);
      }
  #if defined(__APPLE__)
      if (!module) {
        module = dlopen("libvulkan.1.dylib", RTLD_LOCAL | RTLD_LAZY);
      }
  #else
      if (!module) {
        module = dlopen("libvulkan.so.1", RTLD_LOCAL | RTLD_LAZY);
      }
      if (!module) {
        module = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_LAZY);
      }
  #endif
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

auto VulkanContext::create_instance_and_device(
  const char** ins_extensions,
  uint32_t     num_ins_extensions,
  const char** dev_extensions,
  uint32_t     num_dev_extensions,
  VkSurfaceKHR surface) noexcept -> bool {
  destroy();

  if (!create_instance(ins_extensions, num_ins_extensions)) {
    destroy();
    wrench::log_error("Failed to create vulkan instance!");
    return false;
  }

  auto created_device = create_device(
    VK_NULL_HANDLE, surface, dev_extensions, num_dev_extensions, nullptr, 0);

  if (!created_device) {
    destroy();
    wrench::log_error("Failed to create vulkan device!");
    return false;
  }

  destroyed_ = false;
  return true;
}

auto VulkanContext::create_instance(
  const char** ins_extensions, uint32_t num_extensions) noexcept -> bool {
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

  //==--- [find general extensions] ----------------------------------------==//

  const auto has_extension = [&](const char* name) -> bool {
    auto itr = std::find_if(
      std::begin(queried_extensions),
      std::end(queried_extensions),
      [name](const VkExtensionProperties& e) -> bool {
        return std::strcmp(e.extensionName, name) == 0;
      });
    return itr != end(queried_extensions);
  };

  // Look for the requested instance extensions:
  for (uint32_t i = 0; i < num_extensions; i++) {
    if (!has_extension(ins_extensions[i])) {
      wrench::log_error("Instance extension not found: {}", ins_extensions[i]);
      return false;
    }
  }

  // Desired extension for the insance, which don't require other extensions be
  // checked first.
  // clang-format off
  const auto desired_extensions = std::vector<const char *>{
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef VULKAN_DEBUG
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
  };
  // clang-format on

  for (auto ext : desired_extensions) {
    if (has_extension(ext)) {
      instance_exts.push_back(ext);
    }
  }

  //==--- [find specific extensions] ---------------------------------------==//

  if (has_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
    instance_exts.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    supports_phy_dev_props_2_ = true;
  }

  if (
    supports_phy_dev_props_2_ &&
    has_extension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) &&
    has_extension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME)) {
    instance_exts.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    instance_exts.push_back(
      VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    supports_external_ = true;
  }

  const auto ins_ext_end = ins_extensions + num_extensions;
  auto itr = std::find_if(ins_extensions, ins_ext_end, [](const char* name) {
    return std::strcmp(name, VK_KHR_SURFACE_EXTENSION_NAME) == 0;
  });
  bool has_surface_extension = (itr != (ins_ext_end));

  if (
    has_surface_extension &&
    has_extension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
    instance_exts.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    supports_surface_caps_2_ = true;
  }

#ifdef VALIDATION_ENABLED
  const auto has_layer = [&](const char* name) -> bool {
    auto layer_itr = std::find_if(
      std::begin(queried_layers),
      std::end(queried_layers),
      [name](const VkLayerProperties& e) -> bool {
        return strcmp(e.layerName, name) == 0;
      });
    return layer_itr != std::end(queried_layers);
  };

  if (has_layer("VK_LAYER_KHRONOS_validation")) {
    instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    wrench::log_info("Enabling VK_LAYER_KHRONOS_validation.");
  } else if (has_layer("VK_LAYER_LUNARG_standard_validation")) {
    instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    wrench::log_info("Enabling VK_LAYER_LUNARG_standard_validation.");
  }
#endif

  //==--- [create] ---------------------------------------------------------==//

  info.enabledExtensionCount   = instance_exts.size();
  info.ppEnabledExtensionNames = instance_exts.empty() ? nullptr
                                                       : instance_exts.data();
  info.enabledLayerCount   = instance_layers.size();
  info.ppEnabledLayerNames = instance_layers.empty() ? nullptr
                                                     : instance_layers.data();

  for (auto* ext_name : instance_exts) {
    wrench::log_info("Enabling instance extension: {0}", ext_name);
  }

  if (vkCreateInstance(&info, nullptr, &instance_) != VK_SUCCESS) {
    return false;
  }

  volkLoadInstance(instance_);

#ifdef VULKAN_DEBUG
  // Check debug utils and setup callback:
  if (has_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    supports_debug_utils_                         = true;
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
      instance_, &debug_info, nullptr, &_debug_messenger);
  } else if (has_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
    VkDebugReportCallbackCreateInfoEXT debug_info = {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT};
    debug_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                       VK_DEBUG_REPORT_WARNING_BIT_EXT |
                       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_info.pfnCallback = vulkan_debug_cb;
    debug_info.pUserData   = this;
    vkCreateDebugReportCallbackEXT(
      instance_, &debug_info, nullptr, &debug_callback_);
  }
#endif

  return true;
}

//==--- [private] ----------------------------------------------------------==//

auto VulkanContext::select_queue_families(
  const std::vector<VkQueueFamilyProperties>& queue_props) noexcept -> void {
  try_select_separate_queue_families(queue_props);

  const auto gfx_q_count = queue_props[graphics_queue_family_].queueCount - 1;
  // Couldnt find separate graphics and compute:
  if (compute_queue_family_ == VK_QUEUE_FAMILY_IGNORED) {
    compute_queue_family_ = graphics_queue_family_;
    compute_queue_index_  = std::min(gfx_q_count, universal_queue_index_);
    universal_queue_index_++;
  }

  // Couldn't find separate graphics, compute, transfer, or separate graphics,
  // transfer:
  const auto& compute_queue_props = queue_props[compute_queue_family_];
  if (transfer_queue_family_ == VK_QUEUE_FAMILY_IGNORED) {
    transfer_queue_family_ = graphics_queue_family_;
    transfer_queue_index_  = std::min(gfx_q_count, universal_queue_index_);
    universal_queue_index_++;
  } else if (transfer_queue_family_ == compute_queue_family_) {
    transfer_queue_index_ =
      std::min(queue_props[compute_queue_family_].queueCount - 1, 1u);
  }
}

auto VulkanContext::try_select_separate_queue_families(
  const std::vector<VkQueueFamilyProperties>& queue_props) noexcept -> void {
  const auto   num_queues         = queue_props.size();
  VkQueueFlags required           = VK_QUEUE_COMPUTE_BIT;
  const auto   has_required_flags = [&](const auto& props) -> bool {
    return (props.queueFlags & required) == required;
  };

  // Try and find compute queue with different index to graphics queue:
  for (unsigned i = 0; i < num_queues; i++) {
    if (i != graphics_queue_family_ && has_required_flags(queue_props[i])) {
      compute_queue_family_ = i;
      break;
    }
  }

  // Try and find transfer queue different to both graphics and compute:
  required = VK_QUEUE_TRANSFER_BIT;
  for (unsigned i = 0; i < num_queues; i++) {
    if (
      i != graphics_queue_family_ && i != compute_queue_family_ &&
      has_required_flags(queue_props[i])) {
      transfer_queue_family_ = i;
      return;
    }
  }

  // Failed to find separate transfer queue from both graphics and compute, see
  // if we can find one separate from just graphics:
  for (unsigned i = 0; i < num_queues; i++) {
    if (i != graphics_queue_family_ && has_required_flags(queue_props[i])) {
      transfer_queue_family_ = i;
      return;
    }
  }
}

auto VulkanContext::select_physical_device(VkSurfaceKHR surface) noexcept
  -> bool {
  const auto success = [](const auto result) -> bool {
    return result == VK_SUCCESS;
  };

  uint32_t dev_count = 0;
  if (!success(vkEnumeratePhysicalDevices(instance_, &dev_count, nullptr))) {
    return false;
  }

  if (dev_count == 0) {
    wrench::log_error("No physical devices found");
    return false;
  }

  auto devices = std::vector<VkPhysicalDevice>(dev_count);
  if (!success(
        vkEnumeratePhysicalDevices(instance_, &dev_count, devices.data()))) {
    return false;
  }

  //==--- [find a suitable queue] ------------------------------------------==//

  // Returns true if the flags in the queue match the required flags for the
  // context.
  auto has_required_flags = [&](const VkQueueFamilyProperties& props) -> bool {
    return (props.queueFlags & required_flags()) == required_flags();
  };

  for (auto dev : devices) {
    phy_dev_ = dev;
    vkGetPhysicalDeviceProperties(phy_dev_, &dev_props_);

    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(phy_dev_, &queue_count, nullptr);
    if (queue_count == 0) {
      continue;
    }
    auto queue_props = std::vector<VkQueueFamilyProperties>(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
      phy_dev_, &queue_count, queue_props.data());

    for (unsigned i = 0; i < queue_count; ++i) {
      VkBool32 surf_supported = (surface == VK_NULL_HANDLE);
      if (surface != VK_NULL_HANDLE) {
        vkGetPhysicalDeviceSurfaceSupportKHR(
          phy_dev_, i, surface, &surf_supported);
      }

      if (surf_supported && has_required_flags(queue_props[i])) {
        graphics_queue_family_ = i;

        select_queue_families(queue_props);

        // Set the memory properties for the device.
        vkGetPhysicalDeviceMemoryProperties(phy_dev_, &dev_mem_props_);

        if (
          dev_props_.apiVersion >= VK_VERSION_1_1 &&
          volkGetInstanceVersion() >= VK_VERSION_1_1) {
          supports_vulkan_11_ = true;
        }

        return true;
      }
    }
  }
  return false;
}

auto VulkanContext::validate_extensions(
  const char** dev_req_extensions, uint32_t num_req_extensions) noexcept
  -> bool {
  uint32_t ext_count = 0;
  vkEnumerateDeviceExtensionProperties(phy_dev_, nullptr, &ext_count, nullptr);
  auto queried_exts = std::vector<VkExtensionProperties>(ext_count);
  if (ext_count) {
    vkEnumerateDeviceExtensionProperties(
      phy_dev_, nullptr, &ext_count, queried_exts.data());
  }

  const auto has_extension = [&](const char* name) -> bool {
    const auto itr = std::find_if(
      std::begin(queried_exts),
      std::end(queried_exts),
      [name](const VkExtensionProperties& e) -> bool {
        return std::strcmp(e.extensionName, name) == 0;
      });
    return itr != std::end(queried_exts);
  };

  for (uint32_t i = 0; i < num_req_extensions; i++) {
    if (!has_extension(dev_req_extensions[i])) {
      wrench::log_error(
        "Device extension not found: {}", dev_req_extensions[i]);
      return false;
    }
  }
  return true;
}

auto VulkanContext::validate_layers(
  const char** dev_req_layers, uint32_t num_req_layers) noexcept -> bool {
  uint32_t layer_count = 0;
  vkEnumerateDeviceLayerProperties(phy_dev_, &layer_count, nullptr);
  auto queried_layers = std::vector<VkLayerProperties>(layer_count);
  if (layer_count) {
    vkEnumerateDeviceLayerProperties(
      phy_dev_, &layer_count, queried_layers.data());
  }

  const auto has_layer = [&](const char* name) -> bool {
    auto itr = std::find_if(
      std::begin(queried_layers),
      std::end(queried_layers),
      [name](const VkLayerProperties& e) -> bool {
        return strcmp(e.layerName, name) == 0;
      });
    return itr != std::end(queried_layers);
  };

  for (uint32_t i = 0; i < num_req_layers; i++) {
    if (!has_layer(dev_req_layers[i])) {
      wrench::log_error("Device layer not found: {}", dev_req_layers[i]);
      return false;
    }
  }
  return true;
}

auto VulkanContext::create_queue_info() const noexcept
  -> std::tuple<std::vector<VkDeviceQueueCreateInfo>, std::vector<float>> {
  uint32_t queue_count;
  vkGetPhysicalDeviceQueueFamilyProperties(phy_dev_, &queue_count, nullptr);
  auto queue_props = std::vector<VkQueueFamilyProperties>(queue_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    phy_dev_, &queue_count, queue_props.data());

  // clang-format off
  auto queue_info = std::vector<VkDeviceQueueCreateInfo>();
  auto prio       = std::vector<float>{
    VulkanContext::graphics_queue_priority,
    VulkanContext::compute_queue_priority,
    VulkanContext::transfer_queue_priority};
  // clang-format on

  // Definitely have a queue for graphics:
  auto& q_info            = queue_info.emplace_back();
  q_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  q_info.queueFamilyIndex = graphics_queue_family_;
  q_info.queueCount       = std::min(
    universal_queue_index_, queue_props[graphics_queue_family_].queueCount);
  q_info.pQueuePriorities = &prio[0];

  // If there is a separate compute queue, add it:
  if (compute_queue_family_ != graphics_queue_family_) {
    auto& q_info_c            = queue_info.emplace_back();
    q_info_c.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_info_c.queueFamilyIndex = compute_queue_family_;
    q_info_c.queueCount       = std::min(
      transfer_queue_family_ == compute_queue_family_ ? 2u : 1u,
      queue_props[compute_queue_family_].queueCount);
    q_info_c.pQueuePriorities = &prio[1];
  }

  // If there is also a separate transfer queue, add that:
  if (
    transfer_queue_family_ != graphics_queue_family_ &&
    transfer_queue_family_ != compute_queue_family_) {
    auto& q_info_t            = queue_info.emplace_back();
    q_info_t.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_info_t.queueFamilyIndex = transfer_queue_family_;
    q_info_t.queueCount       = 1;
    q_info_t.pQueuePriorities = &prio[2];
  }
  return std::make_tuple(std::move(queue_info), std::move(prio));
}

auto VulkanContext::create_device(
  VkPhysicalDevice dev,
  VkSurfaceKHR     surface,
  const char**     dev_req_extensions,
  uint32_t         num_req_extensions,
  const char**     dev_req_layers,
  uint32_t         num_req_layers) noexcept -> bool {
  phy_dev_ = dev;

  //==--- [find a device] --------------------------------------------------==//

  if (phy_dev_ == VK_NULL_HANDLE) {
    if (!select_physical_device(surface)) {
      wrench::log_error("Failed to find suitable physical device.");
      return false;
    }
  }

  wrench::log_info(
    "Selected physical device: {0} (vendor: {1:x}, device: {2:x}, "
    " api: {3:x}, driver: {4:x})",
    dev_props_.deviceName,
    dev_props_.vendorID,
    dev_props_.deviceID,
    dev_props_.apiVersion,
    dev_props_.driverVersion);

  //==--- [validate] -------------------------------------------------------==//

  if (
    !validate_extensions(dev_req_extensions, num_req_extensions) ||
    !validate_layers(dev_req_layers, num_req_layers)) {
    return false;
  }

  //==--- [create device info] ---------------------------------------------==//

  auto [queue_info, prio]          = create_queue_info();
  VkDeviceCreateInfo device_info   = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pQueueCreateInfos    = queue_info.data();
  device_info.queueCreateInfoCount = queue_info.size();

  //==--- [get phy dev features] -------------------------------------------==//

  dev_features_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
  if (supports_vulkan_11_) {
    vkGetPhysicalDeviceFeatures2(phy_dev_, &dev_features_);
  } else if (supports_phy_dev_props_2_) {
    vkGetPhysicalDeviceFeatures2KHR(phy_dev_, &dev_features_);
  } else {
    vkGetPhysicalDeviceFeatures(phy_dev_, &dev_features_.features);
  }

  if (supports_phy_dev_props_2_) {
    device_info.pNext = &dev_features_;
  } else {
    device_info.pEnabledFeatures = &dev_features_.features;
  }

  //==--- [create extensions & layers] -------------------------------------==//

  // clang-format off
  auto enabled_layers     = std::vector<const char*>();
  auto enabled_extensions =
    std::vector<const char*>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  for (uint32_t i = 0; i < num_req_extensions; i++) {
    enabled_extensions.push_back(dev_req_extensions[i]);
  }
  for (uint32_t i = 0; i < num_req_layers; i++) {
    enabled_layers.push_back(dev_req_layers[i]);
  }

  device_info.enabledExtensionCount   = enabled_extensions.size();
  device_info.ppEnabledExtensionNames = enabled_extensions.empty() 
                                      ? nullptr : enabled_extensions.data();

  device_info.enabledLayerCount   = enabled_layers.size();
  device_info.ppEnabledLayerNames = enabled_layers.empty() 
                                  ? nullptr : enabled_layers.data();
  // clang-format on

  for (auto* enabled_extension : enabled_extensions) {
    wrench::log_info("Enabling device extension: {}", enabled_extension);
  }
  for (auto* enabled_layer : enabled_layers) {
    wrench::log_info("Enabling device layer: {}", enabled_layer);
  }

  //==--- [create logical device] ------------------------------------------==//

  if (vkCreateDevice(phy_dev_, &device_info, nullptr, &device_) != VK_SUCCESS) {
    return false;
  }

  volkLoadDeviceTable(&device_table_, device_);

  device_table_.vkGetDeviceQueue(
    device_, graphics_queue_family_, graphics_queue_index_, &graphics_queue_);
  device_table_.vkGetDeviceQueue(
    device_, compute_queue_family_, compute_queue_index_, &compute_queue_);
  device_table_.vkGetDeviceQueue(
    device_, transfer_queue_family_, transfer_queue_index_, &transfer_queue_);

  return true;
}

auto VulkanContext::destroy() noexcept -> void {
  if (destroyed_) {
    return;
  }

  if (device_ != VK_NULL_HANDLE) {
    device_table_.vkDeviceWaitIdle(device_);
  }

#ifdef VULKAN_DEBUG
  if (debug_callback_) {
    vkDestroyDebugReportCallbackEXT(instance_, debug_callback_, nullptr);
  }
  if (debug_messenger_) {
    vkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }
  debug_callback_  = VK_NULL_HANDLE;
  debug_messenger_ = VK_NULL_HANDLE;
#endif

  if (device_ != VK_NULL_HANDLE) {
    device_table_.vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  destroyed_ = true;
}

} // namespace snowflake::backend