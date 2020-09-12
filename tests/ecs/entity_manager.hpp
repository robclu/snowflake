//==--- snowflake/tests/ecs/entity_manager.hpp ------------- -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  entity_manager.hpp
/// \brief This file implements tests for the entity_manager.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_TESTS_ECS_ENTITY_MANAGER_HPP
#define SNOWFLAKE_TESTS_ECS_ENTITY_MANAGER_HPP

#include <snowflake/ecs/entity_manager.hpp>
#include <gtest/gtest.h>

struct StaticComponent : snowflake::ComponentIdStatic<0> {
  int   a = 0;
  float b = 0.f;
};

struct DynamicComponent : snowflake::ComponentIdDynamic {
  using Base = snowflake::ComponentIdDynamic;

  DynamicComponent() : Base(Base::next()) {}

  int   a = 0;
  float b = 0.f;
};

using Manager = snowflake::EntityManager<snowflake::Entity>;

TEST(entity_manager, creation_and_recycling) {
  Manager manager;
  EXPECT_EQ(manager.entities_created(), size_t{0});
  EXPECT_EQ(manager.entities_active(), size_t{0});
  EXPECT_EQ(manager.entities_free(), size_t{0});

  auto e1 = manager.create();
  auto e2 = manager.create();
  auto e3 = manager.create();

  EXPECT_EQ(manager.entities_created(), size_t{3});
  EXPECT_EQ(manager.entities_active(), size_t{3});
  EXPECT_EQ(manager.entities_free(), size_t{0});

  manager.recycle(e2);

  EXPECT_EQ(manager.entities_created(), size_t{3});
  EXPECT_EQ(manager.entities_active(), size_t{2});
  EXPECT_EQ(manager.entities_free(), size_t{1});
}

#endif // SNOWFLAKE_TESTS_ECS_ENTITY_MANAGER_HPP
