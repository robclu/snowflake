//==--- snowflake/tests/ecs/entity.hpp --------------------- -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  sparse_set.hpp
/// \brief This file implements tests for the sparse set container.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_TESTS_ECS_ENTITY_HPP
#define SNOWFLAKE_TESTS_ECS_ENTITY_HPP

#include <snowflake/ecs/entity.hpp>
#include <gtest/gtest.h>

TEST(entity, default_construction) {
  using namespace snowflake;
  Entity e;

  EXPECT_TRUE(e.invalid());
  EXPECT_EQ(e.id(), Entity::null_id);
  EXPECT_FALSE(static_cast<bool>(e));
}

TEST(entity, id_construction) {
  using namespace snowflake;
  Entity e{2};

  EXPECT_FALSE(e.invalid());
  EXPECT_EQ(e.id(), Entity::IdType(2));
  EXPECT_TRUE(static_cast<bool>(e));
}

TEST(entity, reset) {
  using namespace snowflake;
  Entity e{2};

  EXPECT_FALSE(e.invalid());
  EXPECT_EQ(e.id(), Entity::IdType(2));

  e.reset();
  EXPECT_TRUE(e.invalid());
  EXPECT_EQ(e.id(), Entity::null_id);
}

TEST(entity, copyable) {
  using namespace snowflake;
  Entity e{2};

  Entity e1{e};
  Entity e2 = e1;

  EXPECT_FALSE(e1.invalid());
  EXPECT_FALSE(e2.invalid());
  EXPECT_EQ(e1.id(), Entity::IdType(2));
  EXPECT_EQ(e2.id(), Entity::IdType(2));
}

TEST(entity, moveable) {
  using namespace snowflake;
  Entity e{2};

  Entity e1{std::move(e)};

  EXPECT_FALSE(e1.invalid());
  EXPECT_EQ(e1.id(), Entity::IdType(2));

  Entity e2 = std::move(e1);
  EXPECT_FALSE(e2.invalid());
  EXPECT_EQ(e2.id(), Entity::IdType(2));
}

TEST(entity, comparison) {
  using namespace snowflake;
  Entity e1{4}, e2{3}, e3{4};

  EXPECT_TRUE(e1 == e3);
  EXPECT_TRUE(e1 != e2);
  EXPECT_TRUE(e2 < e3);
}

#endif // SNOWFLAKE_TESTS_ECS_ENTITY_HPP