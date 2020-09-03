#==--- ripple/glow//CMakeLists.txt ------------------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(snowflake VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

#===== [platform] =============================================================#

#===== [compiler flags] =======================================================#

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_compile_definitions(VALIDATION_ENABLED=1)
  add_compile_definitions(VULKAN_DEBUG=1)
else()
  add_compile_definitions(NDEBUG=1)
endif()

if (WIN32)

else()
  set(CXX_DEFAULT_FLAGS "-pthread -Wno-c++1z-extensions")
  set(CXX_PERF_FLAGS    "-O3 -march=native -ffast-math")
  set(CMAKE_CXX_FLAGS_RELEASE "${CXX_DEFAULT_FLAGS} ${CXX_PERF_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CXX_DEFAULT_FLAGS} -g")
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
  set(
    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}" 
    CACHE STRING "CXX FLAGS" FORCE
  )
else()
  set(
    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}"
    CACHE STRING "CXX FLAGS" FORCE
  )
endif()

#===== [subdirs] ==============================================================#

add_subdirectory(external)
add_subdirectory(apps)

option(SNOWFLAKE_BUILD_TESTS "build tests" OFF)
if(${SNOWFLAKE_BUILD_TESTS} OR 
  (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
  configure_file(
    CMakeLists-googletest.txt.in googletest-download/CMakeLists.txt
  )
  execute_process(
    COMMAND           ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE   result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
  )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(
    COMMAND           ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE   result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()
  
  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                   ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                   EXCLUDE_FROM_ALL)

  include_directories("${gtest_SOURCE_DIR}/include")

  add_subdirectory(tests)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
