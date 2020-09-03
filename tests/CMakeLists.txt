#==--- snowflake/tests/CMakeLists.txt ---------------------------------------==#
#
#                      Copyright (c) 2020 Rob Clucas
#
#  This file is distributed under the MIT License. See LICENSE for details.
#
#==--------------------------------------------------------------------------==#

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(ecs_tests ${CMAKE_CURRENT_SOURCE_DIR}/ecs.cpp)
target_link_libraries(ecs_tests gtest_main wrench::wrench)
