//==--- snowflake/tests/ecs/component_id.hpp --------------- -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  component_id.hpp
/// \brief This file implements tests for component ids.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_TESTS_ECS_COMPONENT_ID_HPP
#define SNOWFLAKE_TESTS_ECS_COMPONENT_ID_HPP

#include <snowflake/ecs/component_id.hpp>
#include <gtest/gtest.h>

struct IdTest : public snowflake::ComponentIdStatic<0> {};
struct AnyType {};
struct OtherType {};

TEST(component_id, can_get_value_static) {
  EXPECT_EQ(snowflake::component_id_v<IdTest>, 0);
}

TEST(component_id, static_is_minimal_size) {
  EXPECT_EQ(sizeof(IdTest), size_t{1});
}

TEST(component_id_static, constexpr_id_trait) {
  const bool a = snowflake::constexpr_component_id_v<IdTest>;
  const bool b = snowflake::constexpr_component_id_v<AnyType>;
  EXPECT_TRUE(a);
  EXPECT_FALSE(b);
}

TEST(component_id_static, can_use_general_id_function) {
  const auto id_a = snowflake::component_id<IdTest>();
  const auto id_b = snowflake::component_id<AnyType>();
  const auto id_c = snowflake::component_id<AnyType>();
  const auto id_d = snowflake::component_id<OtherType>();
  EXPECT_EQ(id_a, 0);
  EXPECT_EQ(id_b, snowflake::ComponentIdDynamic::start_id);
  EXPECT_EQ(id_c, snowflake::ComponentIdDynamic::start_id);
  EXPECT_EQ(id_d, snowflake::ComponentIdDynamic::start_id + 1);
}

#endif // SNOWFLAKE_TESTS_ECS_COMPONENT_ID_HPP