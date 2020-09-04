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

TEST(sparse_set, iterator_construction) {
  using Iterator = typename SparseSet::Iterator;
  SparseSet         set;
  snowflake::Entity e{entity_id};
  set.emplace(e);

  // Check both copy construction and assignment:
  Iterator end{set.begin()};
  Iterator begin{};
  begin = set.end();

  std::swap(begin, end);
  EXPECT_EQ(begin, set.begin());
  EXPECT_EQ(end, set.end());
  EXPECT_NE(begin, end);
}

TEST(sparse_set, iterator_iteration) {
  using Iterator = typename SparseSet::Iterator;
  SparseSet         set;
  snowflake::Entity e{entity_id};
  set.emplace(e);

  // Check both copy construction and assignment:
  Iterator begin{set.begin()};
  Iterator end{set.end()};

  EXPECT_EQ(*begin, e);
  EXPECT_EQ(*(begin.operator->()), e);

  // Postfix:
  EXPECT_EQ(begin++, set.begin());
  EXPECT_EQ(end--, set.end());

  // Iterators are swapped, swap back:
  std::swap(begin, end);
  EXPECT_EQ(++begin, set.end());
  EXPECT_EQ(--end, set.begin());

  --begin;
  ++end;

  EXPECT_EQ(begin + 1, end);
  EXPECT_EQ(end - 1, begin);

  EXPECT_EQ(begin += 1, end);
  EXPECT_EQ(begin -= 1, set.begin());

  EXPECT_EQ(begin + (end - begin), set.end());
  EXPECT_EQ(begin - (begin - end), set.end());
  EXPECT_EQ(end - (end - begin), set.begin());
  EXPECT_EQ(end + (begin - end), set.begin());

  EXPECT_EQ(begin[0], *begin);

  EXPECT_LT(begin, end);
  EXPECT_GT(end, begin);

  EXPECT_LE(begin, set.begin());
  EXPECT_GE(begin, set.begin());
  EXPECT_LE(end, set.end());
  EXPECT_GE(end, set.end());

  const IdType entities = 10;
  IdType       sum      = entity_id;
  for (IdType id = 1; id < IdType{entities}; ++id) {
    set.emplace(snowflake::Entity{id});
    sum += id;
  }

  IdType it_sum = 0;
  for (const auto& e : set) {
    it_sum += e;
  }
  EXPECT_EQ(it_sum, sum);

  // Check that insertion doesn't break the iterator:
  it_sum = 0;
  snowflake::Entity ent{entity_id + 2};

  for (const auto& e : set) {
    set.emplace(ent);
    it_sum += e;
    ent++;
  }
  EXPECT_EQ(it_sum, sum);
  EXPECT_EQ(set.size(), entities * 2);
}

TEST(sparse_set, reverse_iterator_construction) {
  using Iterator = typename SparseSet::ReverseIterator;
  SparseSet         set;
  snowflake::Entity e{entity_id};
  set.emplace(e);

  // Check both copy construction and assignment:
  Iterator end{set.rbegin()};
  Iterator begin{};
  begin = set.rend();

  std::swap(begin, end);
  EXPECT_EQ(begin, set.rbegin());
  EXPECT_EQ(end, set.rend());
  EXPECT_NE(begin, end);
}

TEST(sparse_set, reverse_iterator_iteration) {
  using Iterator = typename SparseSet::ReverseIterator;
  SparseSet         set;
  snowflake::Entity e{entity_id};
  set.emplace(e);

  // Check both copy construction and assignment:
  Iterator begin{set.rbegin()};
  Iterator end{set.rend()};

  EXPECT_EQ(*begin, e);

  // post fix
  EXPECT_EQ(begin++, set.rbegin());
  EXPECT_EQ(end--, set.rend());

  // Iterators are swapped, swap back, then prefix:
  std::swap(begin, end);
  EXPECT_EQ(++begin, set.rend());
  EXPECT_EQ(--end, set.rbegin());

  --begin;
  ++end;

  EXPECT_EQ(begin + 1, end);
  EXPECT_EQ(end - 1, begin);

  EXPECT_EQ(begin += 1, end);
  EXPECT_EQ(begin -= 1, set.rbegin());

  EXPECT_EQ(begin + (end - begin), set.rend());
  EXPECT_EQ(begin - (begin - end), set.rend());
  EXPECT_EQ(end - (end - begin), set.rbegin());
  EXPECT_EQ(end + (begin - end), set.rbegin());

  EXPECT_EQ(begin[0], *begin);

  EXPECT_LT(begin, end);
  EXPECT_GT(end, begin);

  EXPECT_LE(begin, set.rbegin());
  EXPECT_GE(begin, set.rbegin());
  EXPECT_LE(end, set.rend());
  EXPECT_GE(end, set.rend());

  const IdType entities = 10;
  IdType       sum      = entity_id;
  for (IdType id = 1; id < IdType{entities}; ++id) {
    set.emplace(snowflake::Entity{id});
    sum += id;
  }

  IdType it_sum = 0;
  for (Iterator it = set.rbegin(); it != set.rend(); ++it) {
    it_sum += *it;
  }
  EXPECT_EQ(it_sum, sum);
}

#endif // SNOWFLAKE_TESTS_ECS_SPARSE_SET_HPP