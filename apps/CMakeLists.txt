#==--- snowflake/apps/CMakeLists.txt ----------------------------------------==#
#
#                                 Snowflake
#
#                      Copyright (c) 2020 Rob Clucas
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
  add_compile_definitions(SNOWFLAKE_RESIZABLE_WINDOW=0)
else()
  add_compile_definitions(SNOWFLAKE_RESIZABLE_WINDOW=1)
endif()

#===== [sources] ==============================================================#

set(SNOWFLAKE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SNOWFLAKE_SOURCE_DIR  ${PROJECT_SOURCE_DIR}/src)
set(SOURCES snowflake.cpp)

if (SNOWFLAKE_HEADLESS_PLATFORM)
else()
  list(
    APPEND SOURCES 
    ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/platform/sdl_platform.cpp
  )
endif(SNOWFLAKE_HEADLESS_PLATFORM)

list(
  APPEND SOURCES
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/frame_data.cpp
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/vulkan_command_buffer.cpp
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/vulkan_command_pool.cpp
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/vulkan_context.cpp
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/vulkan_driver.cpp
  ${SNOWFLAKE_SOURCE_DIR}/rendering/backend/vk/vulkan_surface_context.cpp
  ${SNOWFLAKE_SOURCE_DIR}/window.cpp
  ${SNOWFLAKE_SOURCE_DIR}/engine.cpp
)

#===== [applications] =========================================================#

add_executable(snowflake ${SOURCES})
include_directories(
  ${SNOWFLAKE_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIR}
  ${EXTRA_INCLUDE_DIRS}
)
target_link_libraries(
  snowflake
  PRIVATE 
  wrench::wrench
  volk
  Vulkan::Vulkan
  ${SDL2_LIBRARY}
  ${EXTRA_LIBRARIES}
  fmt::fmt
)