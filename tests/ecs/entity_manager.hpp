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

struct StaticComponent : public snowflake::ComponentIdStatic<0> {
  int   a = 0;
  float b = 0.0f;
};

struct DynamicComponent {
  int   a = 0;
  float b = 0.0f;
};

using EntityManager = snowflake::EntityManager<snowflake::Entity>;

TEST(entity_manager, creation_and_recycling) {
  EntityManager manager;
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

TEST(entity_manager, dynamic_components) {
  EntityManager em;

  auto e1 = em.create();
  auto e2 = em.create();
  auto e3 = em.create();

  em.emplace<DynamicComponent>(e1, 4, 3.0f);
  em.emplace<DynamicComponent>(e2, 5, 6.0f);

  auto& c1 = em.get<DynamicComponent>(e1);
  auto& c2 = em.get<DynamicComponent>(e2);

  EXPECT_EQ(c1.a, 4);
  EXPECT_EQ(c1.b, 3.0f);
  EXPECT_EQ(c2.a, 5);
  EXPECT_EQ(c2.b, 6.0f);

  EXPECT_EQ(em.size<StaticComponent>(), size_t{2});
}

TEST(entity_manager, static_components) {
  EntityManager em;

  auto e1 = em.create();
  auto e2 = em.create();
  auto e3 = em.create();

  em.emplace<StaticComponent>(e1, 4, 3.0f);
  em.emplace<StaticComponent>(e2, 5, 6.0f);

  auto& c1 = em.get<StaticComponent>(e1);
  auto& c2 = em.get<StaticComponent>(e2);

  EXPECT_EQ(c1.a, 4);
  EXPECT_EQ(c1.b, 3.0f);
  EXPECT_EQ(c2.a, 5);
  EXPECT_EQ(c2.b, 6.0f);

  EXPECT_EQ(em.size<StaticComponent>(), size_t{2});
}

#endif // SNOWFLAKE_TESTS_ECS_ENTITY_MANAGER_HPP
