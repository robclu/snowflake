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

namespace snowflake {

template <>
struct ComponentIdTraits<IdTest> {
  // clang-format off
  /** Type is a static component id. */
  static constexpr bool     is_static = true;
  /** The value of the component id. */
  static constexpr uint16_t value     = 0;
  // clang-format on
};

} // namespace snowflake

TEST(component_id_static, can_get_value) {
  EXPECT_EQ(snowflake::component_id_v<IdTest>, 0);
}

#endif // SNOWFLAKE_TESTS_ECS_COMPONENT_ID_HPP