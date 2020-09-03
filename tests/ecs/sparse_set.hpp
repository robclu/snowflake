//==--- snowflake/tests/ecs/sparese_set.hpp ---------------- -*- C++ -*- ---==//
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

#ifndef SNOWFLAKE_TESTS_ECS_SPARSE_SET_HPP
#define SNOWFLAKE_TESTS_ECS_SPARSE_SET_HPP

#include <snowflake/ecs/entity.hpp>
#include <snowflake/ecs/sparse_set.hpp>
#include <gtest/gtest.h>

using SparseSet = snowflake::SparseSet<snowflake::Entity>;
using IdType    = typename snowflake::Entity::IdType;

constexpr inline size_t set_size  = 100;
constexpr inline auto   entity_id = IdType{22};

TEST(sparse_set, basic_functionality) {
  SparseSet set;

  set.reserve(set_size);

  EXPECT_EQ(set.capacity(), set_size);
  EXPECT_EQ(set.size(), size_t{0});
  EXPECT_EQ(set.extent(), size_t{0});
  EXPECT_TRUE(set.empty());
  EXPECT_FALSE(set.exists(snowflake::Entity{entity_id}));
  EXPECT_FALSE(set.exists(snowflake::Entity::null_entity()));

  snowflake::Entity entity{entity_id};
  set.emplace(entity);

  EXPECT_EQ(set.index(entity), size_t{0});
  EXPECT_EQ(set.size(), size_t{1});
  EXPECT_EQ(set.extent(), size_t{1} * SparseSet::page_size);
  EXPECT_FALSE(set.empty());
  EXPECT_TRUE(set.exists(entity));
  EXPECT_FALSE(set.exists(snowflake::Entity::null_entity()));

  set.erase(entity);

  EXPECT_EQ(set.size(), size_t{0});
  EXPECT_EQ(set.extent(), size_t{1} * SparseSet::page_size);
  EXPECT_TRUE(set.empty());
  EXPECT_FALSE(set.exists(entity));
  EXPECT_FALSE(set.exists(snowflake::Entity::null_entity()));
}

TEST(sparse_set, move_constructible) {
  const bool a = std::is_nothrow_move_constructible_v<SparseSet>;
  const bool b = std::is_nothrow_move_assignable_v<SparseSet>;

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);

  snowflake::Entity entity{entity_id};
  SparseSet         init;
  init.emplace(entity);

  EXPECT_EQ(init.index(entity), size_t{0});
  EXPECT_EQ(init.size(), size_t{1});
  EXPECT_EQ(init.extent(), size_t{1} * SparseSet::page_size);
  EXPECT_FALSE(init.empty());
  EXPECT_TRUE(init.exists(entity));

  SparseSet set{std::move(init)};

  init = std::move(set);
  set  = std::move(init);

  EXPECT_TRUE(init.empty());
  EXPECT_FALSE(set.empty());
  EXPECT_EQ(set.index(entity), size_t{0});
}

TEST(sparse_set, page_functionality) {
  SparseSet      set;
  constexpr auto page_size = SparseSet::page_size;

  EXPECT_EQ(set.extent(), size_t{0});
  EXPECT_EQ(set.size(), size_t{0});

  snowflake::Entity e1{page_size - 1};
  set.emplace(e1);
  EXPECT_EQ(set.extent(), page_size);
  EXPECT_EQ(set.size(), size_t{1});
  EXPECT_EQ(set.index(e1), size_t{0});
  EXPECT_TRUE(set.exists(e1));

  snowflake::Entity e2(page_size);
  set.emplace(e2);
  EXPECT_EQ(set.extent(), page_size * 2);
  EXPECT_EQ(set.size(), size_t{2});
  EXPECT_EQ(set.index(e2), size_t{1});
  EXPECT_TRUE(set.exists(e2));

  set.erase(e1);
  EXPECT_EQ(set.extent(), page_size * 2);
  EXPECT_EQ(set.size(), size_t{1});
  EXPECT_EQ(set.index(e2), size_t{0});
  EXPECT_TRUE(set.exists(e2));
}

TEST(sparse_set, swap) {
  SparseSet         set;
  snowflake::Entity e1{3}, e2{4};

  set.emplace(e1);
  set.emplace(e2);
}

#endif // SNOWFLAKE_TESTS_ECS_SPARSE_SET_HPP