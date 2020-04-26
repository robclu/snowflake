#==--- ripple/glow//apps/CMakeLists.txt -------------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

#===== [packages] =============================================================#

set(CMAKE_PREFIX_PATH /usr/local)

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

if (GLOW_HEADLESS_PLATFORM)
else()
  list(
    APPEND SOURCES 
    ${GLOW_SOURCE_DIR}/backend/platform/sdl_platform.cpp
  )
endif(GLOW_HEADLESS_PLATFORM)

list(
  APPEND SOURCES
  ${GLOW_SOURCE_DIR}/backend/vk/frame_data.cpp
  ${GLOW_SOURCE_DIR}/backend/vk/vulkan_command_buffer.cpp
  ${GLOW_SOURCE_DIR}/backend/vk/vulkan_command_pool.cpp
  ${GLOW_SOURCE_DIR}/backend/vk/vulkan_context.cpp
  ${GLOW_SOURCE_DIR}/backend/vk/vulkan_driver.cpp
  ${GLOW_SOURCE_DIR}/backend/vk/vulkan_surface_context.cpp
  ${GLOW_SOURCE_DIR}/engine/engine.cpp
  ${GLOW_SOURCE_DIR}/window/window.cpp
)

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
  PRIVATE 
  volk
  Vulkan::Vulkan
  ${SDL2_LIBRARY}
  ${EXTRA_LIBRARIES}
  fmt::fmt
)