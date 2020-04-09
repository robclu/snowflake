#==--- ripple/glow/external/CMakeLists.txt ----------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

#==--- [volk] ---------------------------------------------------------------==#

# Avoid definition errors with Volk and Vulkan, since Volk defines the Vulkan
# functions for us.
add_compile_definitions(VK_NO_PROTOTYPES=1)

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
  if (IOS)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_IOS_MVK)
  else()
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
  endif()
endif()

add_compile_definitions(GLOW_RESIZABLE_WINDOW=0)

add_subdirectory(volk)

#==--------------------------------------------------------------------------==#