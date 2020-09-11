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

TEST(component_id_static, can_get_value) {
  EXPECT_EQ(snowflake::component_id_v<IdTest>, 0);
}

TEST(component_id_static, minimal_size) {
  EXPECT_EQ(sizeof(IdTest), size_t{1});
}

TEST(component_id_static, constexpr_id_trait) {
  const bool b = snowflake::constexpr_component_id_v<IdTest>;
  EXPECT_TRUE(b);
}

TEST(component_id_dynamic, can_get_value_with_next) {
  using type = typename snowflake::ComponentIdDynamic::Type;

  auto a = snowflake::ComponentIdDynamic::next();
  auto b = snowflake::ComponentIdDynamic::next();

  EXPECT_EQ(a.value, snowflake::ComponentIdDynamic::start_id);
  EXPECT_EQ(b.value, snowflake::ComponentIdDynamic::start_id + 1);
}

TEST(component_id_dynamic, evaluales_to_bool) {
  using namespace snowflake;

  auto a = ComponentIdDynamic::next();
  auto b = ComponentIdDynamic{};

  EXPECT_TRUE(a);
  EXPECT_FALSE(static_cast<bool>(b));
}

TEST(component_id_dynamic, constexpr_id_trait) {
  const bool b =
    snowflake::constexpr_component_id_v<snowflake::ComponentIdDynamic>;
  EXPECT_FALSE(b);
}

#endif // SNOWFLAKE_TESTS_ECS_COMPONENT_ID_HPP