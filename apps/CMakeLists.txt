#==--- ripple/glow//apps/CMakeLists.txt -------------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

#===== [packages] =============================================================#

#==--- [fmt] ----------------------------------------------------------------==#
find_package(fmt REQUIRED)
if (NOT fmt_FOUND)
  message(ERROR "Fmt not found!")
endif()
#==--- [vulkan] -------------------------------------------------------------==#
find_package(Vulkan REQUIRED)
if (NOT VULKAN_FOUND)
  message(ERROR "Vulkan not found!")
endif()
#==--- [sdl 2] --------------------------------------------------------------==#
find_package(SDL2 REQUIRED)
if (NOT SDL2_FOUND)
  message(ERROR "SDL2 not found!")
endif()

if (APPLE)
  #==--- [app kit] ----------------------------------------------------------==#
  find_library(APPKIT_LIB AppKit)
  find_path(APPKIT_INCLUDE_DIR AppKit/AppKit.h)
  #==--- [cocoa] ------------------------------------------------------------==#
  find_library(COCOA_LIB Cocoa)
  find_path(COCOA_INCLUDE_DIR Cocoa/Cocoa.h)
  #==--- [quartz] -----------------------------------------------------------==#
  find_library(QUARTZ_CORE_LIB QuartzCore)
  find_path(QUARTZ_CORE_INCLUDE_DIR QuartzCore/QuartzCore.h)
  #==------------------------------------------------------------------------==#

  mark_as_advanced(COCOA_LIB QUARTZ_CORE_LIB APPKIT_LIB)
  mark_as_advanced(
    APPKIT_INCLUDE_DIR
    COCOA_INCLUDE_DIR 
    QUARTZ_CORE_INCLUDE_DIR
  )
  set(EXTRA_LIBRARIES ${APPKIT_LIB} ${COCOA_LIBS} ${QUARTZ_CORE_LIB})
  set(
    EXTRA_INCLUDE_DIRS 
    ${APPKIT_INCLUDE_DIR}
    ${COCOA_INCLUDE_DIR} 
    ${QUARTZ_CORE_INCLUDE_DIR}
  )
endif()

#===== [definitions] ==========================================================#

if (IOS OR ANDROID)
  add_compile_definitions(GLOW_RESIZABLE_WINDOW=0)
else()
  add_compile_definitions(GLOW_RESIZABLE_WINDOW=1)
endif()

#===== [sources] ==============================================================#

set(GLOW_INCLUDE_DIR ${glow_SOURCE_DIR}/include)
set(GLOW_SOURCE_DIR  ${glow_SOURCE_DIR}/src)
set(SOURCES glow.cpp)

if (APPLE)
  list(
    APPEND SOURCES 
    ${GLOW_SOURCE_DIR}/vk/apple_cocoa_platform.mm
  )
 endif(APPLE)

#===== [applications] =========================================================#

add_executable(glow ${SOURCES})
include_directories(
  ${GLOW_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIR}
  ${EXTRA_INCLUDE_DIRS}
)
target_link_libraries(
  glow 
  ${Vulkan_LIBRARIES}
  ${SDL2_LIBRARY}
  ${EXTRA_LIBRARIES}
  fmt::fmt
)