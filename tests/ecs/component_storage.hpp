//==--- snowflake/tests/ecs/component_storage.hpp ---------- -*- C++ -*- ---==//
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

#ifndef SNOWFLAKE_TESTS_ECS_COMPONENT_STORAGE_HPP
#define SNOWFLAKE_TESTS_ECS_COMPONENT_STORAGE_HPP

#include <snowflake/ecs/entity.hpp>
#include <snowflake/ecs/component_storage.hpp>
#include <gtest/gtest.h>

struct Agg {
  int   a;
  float b;
};

struct NonAgg {
  NonAgg(int a_, float b_) : a{a_}, b{b_} {}

  int   a;
  float b;
};

using AggStorage    = snowflake::ComponentStorage<snowflake::Entity, Agg>;
using NonAggStorage = snowflake::ComponentStorage<snowflake::Entity, NonAgg>;
using IdType        = typename snowflake::Entity::IdType;

constexpr inline size_t num_comps = 100;
constexpr inline auto   comp_id   = IdType{22};

TEST(component_storage, basic_functionality_aggregate) {
  AggStorage aggs;

  aggs.reserve(num_comps);

  EXPECT_EQ(aggs.capacity(), num_comps);
  EXPECT_EQ(aggs.size(), size_t{0});
  EXPECT_EQ(aggs.extent(), size_t{0});
  EXPECT_TRUE(aggs.empty());
  EXPECT_FALSE(aggs.exists(snowflake::Entity{comp_id}));
  EXPECT_FALSE(aggs.exists(snowflake::Entity::null_entity()));

  snowflake::Entity entity{comp_id};
  aggs.emplace(entity, int{10}, float{1.0});

  EXPECT_EQ(aggs.index(entity), size_t{0});
  EXPECT_EQ(aggs.size(), size_t{1});
  EXPECT_EQ(aggs.extent(), size_t{1} * AggStorage::page_size);
  EXPECT_FALSE(aggs.empty());
  EXPECT_TRUE(aggs.exists(entity));
  EXPECT_FALSE(aggs.exists(snowflake::Entity::null_entity()));

  Agg& c = aggs.get(entity);
  EXPECT_EQ(c.a, int{10});
  EXPECT_EQ(c.b, float{1.0});

  aggs.erase(entity);

  EXPECT_EQ(aggs.size(), size_t{0});
  EXPECT_EQ(aggs.extent(), size_t{1} * AggStorage::page_size);
  EXPECT_TRUE(aggs.empty());
  EXPECT_FALSE(aggs.exists(entity));
  EXPECT_FALSE(aggs.exists(snowflake::Entity::null_entity()));
}

TEST(component_storage, basic_functionality_non_aggregate) {
  NonAggStorage non_aggs;

  non_aggs.reserve(num_comps);

  EXPECT_EQ(non_aggs.capacity(), num_comps);
  EXPECT_EQ(non_aggs.size(), size_t{0});
  EXPECT_EQ(non_aggs.extent(), size_t{0});
  EXPECT_TRUE(non_aggs.empty());
  EXPECT_FALSE(non_aggs.exists(snowflake::Entity{comp_id}));
  EXPECT_FALSE(non_aggs.exists(snowflake::Entity::null_entity()));

  snowflake::Entity entity{comp_id};
  non_aggs.emplace(entity, int{10}, float{1.0});

  EXPECT_EQ(non_aggs.index(entity), size_t{0});
  EXPECT_EQ(non_aggs.size(), size_t{1});
  EXPECT_EQ(non_aggs.extent(), size_t{1} * NonAggStorage::page_size);
  EXPECT_FALSE(non_aggs.empty());
  EXPECT_TRUE(non_aggs.exists(entity));
  EXPECT_FALSE(non_aggs.exists(snowflake::Entity::null_entity()));

  NonAgg& c = non_aggs.get(entity);
  EXPECT_EQ(c.a, int{10});
  EXPECT_EQ(c.b, float{1.0});

  non_aggs.erase(entity);

  EXPECT_EQ(non_aggs.size(), size_t{0});
  EXPECT_EQ(non_aggs.extent(), size_t{1} * NonAggStorage::page_size);
  EXPECT_TRUE(non_aggs.empty());
  EXPECT_FALSE(non_aggs.exists(entity));
  EXPECT_FALSE(non_aggs.exists(snowflake::Entity::null_entity()));
}

TEST(component_storage, move_constructible_aggregate) {
  const bool a = std::is_nothrow_move_constructible_v<AggStorage>;
  const bool b = std::is_nothrow_move_assignable_v<AggStorage>;

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);

  snowflake::Entity entity{comp_id};
  AggStorage        init;
  init.emplace(entity, int{10}, float{1.0});

  EXPECT_EQ(init.index(entity), size_t{0});
  EXPECT_EQ(init.size(), size_t{1});
  EXPECT_EQ(init.extent(), size_t{1} * AggStorage::page_size);
  EXPECT_FALSE(init.empty());
  EXPECT_TRUE(init.exists(entity));

  AggStorage aggs{std::move(init)};

  init = std::move(aggs);
  aggs = std::move(init);

  EXPECT_TRUE(init.empty());
  EXPECT_FALSE(aggs.empty());
  EXPECT_EQ(aggs.index(entity), size_t{0});

  Agg& c = aggs.get(entity);
  EXPECT_EQ(c.a, int{10});
  EXPECT_EQ(c.b, float{1.0});
}

TEST(component_storage, move_constructible_non_aggregate) {
  const bool a = std::is_nothrow_move_constructible_v<NonAggStorage>;
  const bool b = std::is_nothrow_move_assignable_v<NonAggStorage>;

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);

  snowflake::Entity entity{comp_id};
  NonAggStorage     init;
  init.emplace(entity, int{10}, float{1.0});

  EXPECT_EQ(init.index(entity), size_t{0});
  EXPECT_EQ(init.size(), size_t{1});
  EXPECT_EQ(init.extent(), size_t{1} * NonAggStorage::page_size);
  EXPECT_FALSE(init.empty());
  EXPECT_TRUE(init.exists(entity));

  NonAggStorage non_aggs{std::move(init)};

  init     = std::move(non_aggs);
  non_aggs = std::move(init);

  EXPECT_TRUE(init.empty());
  EXPECT_FALSE(non_aggs.empty());
  EXPECT_EQ(non_aggs.index(entity), size_t{0});

  NonAgg& c = non_aggs.get(entity);
  EXPECT_EQ(c.a, int{10});
  EXPECT_EQ(c.b, float{1.0});
}

TEST(component_storage, swap_aggregate) {
  AggStorage        aggs;
  snowflake::Entity e1{2}, e2{4};

  aggs.emplace(e1, int{10}, float{1});
  aggs.emplace(e2, int{11}, float{2});

  auto i1 = aggs.index(e1);
  auto i2 = aggs.index(e2);

  aggs.swap(e1, e2);

  EXPECT_EQ(i1, aggs.index(e2));
  EXPECT_EQ(i2, aggs.index(e1));
}

TEST(component_storage, swap_non_aggregate) {
  NonAggStorage     non_aggs;
  snowflake::Entity e1{2}, e2{4};

  non_aggs.emplace(e1, int{10}, float{1});
  non_aggs.emplace(e2, int{11}, float{2});

  auto i1 = non_aggs.index(e1);
  auto i2 = non_aggs.index(e2);

  non_aggs.swap(e1, e2);

  EXPECT_EQ(i1, non_aggs.index(e2));
  EXPECT_EQ(i2, non_aggs.index(e1));
}

TEST(component_storage, find_agregagte) {
  AggStorage        aggs;
  snowflake::Entity ent(comp_id);

  EXPECT_EQ(aggs.find(ent), aggs.end());

  Agg a{9, 0.5f};
  aggs.emplace(ent, a);
  EXPECT_NE(aggs.find(ent), aggs.end());

  Agg               a1{10, 1.0f}, a2{11, 2.0f}, a3{12, 3.0f};
  snowflake::Entity e1{1}, e2{2}, e3{3};
  aggs.emplace(e1, a1);
  aggs.emplace(e2, a2);
  aggs.emplace(e3, a3);

  auto it = aggs.find(e3);
  EXPECT_EQ(it->a, a3.a);
  EXPECT_EQ(it->b, a3.b);
  ++it;
  EXPECT_EQ(it->a, a2.a);
  EXPECT_EQ(it->b, a2.b);
  ++it;
  EXPECT_EQ(it->a, a1.a);
  EXPECT_EQ(it->b, a1.b);
  ++it;
  EXPECT_EQ(it->a, a.a);
  EXPECT_EQ(it->b, a.b);
}

TEST(component_storage, find_non_agregagte) {
  NonAggStorage     non_aggs;
  snowflake::Entity ent(comp_id);

  EXPECT_EQ(non_aggs.find(ent), non_aggs.end());

  NonAgg a{9, 0.5f};
  non_aggs.emplace(ent, a);
  EXPECT_NE(non_aggs.find(ent), non_aggs.end());

  NonAgg            a1{10, 1.0f}, a2{11, 2.0f}, a3{12, 3.0f};
  snowflake::Entity e1{1}, e2{2}, e3{3};
  non_aggs.emplace(e1, a1);
  non_aggs.emplace(e2, a2);
  non_aggs.emplace(e3, a3);

  auto it = non_aggs.find(e3);
  EXPECT_EQ(it->a, a3.a);
  EXPECT_EQ(it->b, a3.b);
  ++it;
  EXPECT_EQ(it->a, a2.a);
  EXPECT_EQ(it->b, a2.b);
  ++it;
  EXPECT_EQ(it->a, a1.a);
  EXPECT_EQ(it->b, a1.b);
  ++it;
  EXPECT_EQ(it->a, a.a);
  EXPECT_EQ(it->b, a.b);
}

#endif // SNOWFLAKE_TESTS_ECS_COMPONENT_STORAGE_HPP
