#==--- ripple/glow//CMakeLists.txt ------------------------------------------==#
#
#                      Copyright (c) 2020 Ripple
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(glow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${glow_SOURCE_DIR}/cmake")

if (WIN32)

else()
  set(CXX_DEFAULT_FLAGS "-std=c++17 -pthread -Wno-c++1z-extensions")
  set(CXX_PERF_FLAGS    "-O3 -march=native -ffast-math")
  set(CMAKE_CXX_FLAGS_RELEASE "${CXX_DEFAULT_FLAGS} ${CXX_PERF_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CXX_DEFAULT_FLAGS}")
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(apps)