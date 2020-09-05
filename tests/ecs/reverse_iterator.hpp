//==--- snowflake/tests/ecs/reverse_iterator.hpp ----------- -*- C++ -*- ---==//
//
//                                Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  reverse_iterator.hpp
/// \brief This file implements tests for a reverse_iterator.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_TESTS_ECS_REVERSE_ITERATOR_HPP
#define SNOWFLAKE_TESTS_ECS_REVERSE_ITERATOR_HPP

#include <snowflake/ecs/reverse_iterator.hpp>
#include <gtest/gtest.h>
#include <vector>

using Type          = int32_t;
using Container     = std::vector<Type>;
using Iterator      = snowflake::ReverseIterator<Container, false, size_t>;
using ConstIterator = snowflake::ReverseIterator<Container, true, size_t>;

TEST(reverse_iterator, nonconst_functionality) {
  constexpr Type value = 22;
  Container      c     = {value};

  // Check both copy construction and assignment:
  Iterator end{c, 0};
  Iterator begin{c, c.size()};

  auto e = end;
  auto b = begin;

  EXPECT_EQ(*begin, c[0]);

  // post fix
  EXPECT_EQ(begin++, b);
  EXPECT_EQ(end--, e);

  // Iterators are swapped, swap back, then prefix:
  std::swap(begin, end);
  EXPECT_EQ(++begin, e);
  EXPECT_EQ(--end, b);

  --begin;
  ++end;

  EXPECT_EQ(begin + 1, end);
  EXPECT_EQ(end - 1, begin);

  EXPECT_EQ(begin += 1, end);
  EXPECT_EQ(begin -= 1, b);

  EXPECT_EQ(begin + (end - begin), e);
  EXPECT_EQ(begin - (begin - end), e);
  EXPECT_EQ(end - (end - begin), b);
  EXPECT_EQ(end + (begin - end), b);

  EXPECT_EQ(begin[0], *begin);

  EXPECT_LT(begin, end);
  EXPECT_GT(end, begin);

  EXPECT_LE(begin, end);
  EXPECT_GE(begin, b);
  EXPECT_LE(end, e);
  EXPECT_GE(end, begin);

  const Type values = 10;
  Type       sum    = value;
  for (Type v = 1; v < Type{values}; ++v) {
    c.emplace_back(v);
    sum += v;
  }

  Type it_sum = 0;
  for (auto it = Iterator{c, c.size()}; it != Iterator{c, 0}; ++it) {
    it_sum += *it;
  }
  EXPECT_EQ(it_sum, sum);

  it_sum = 0;
  for (auto it = Iterator{c, c.size()}; it != Iterator{c, 0}; ++it) {
    it_sum += *it;
  }
  EXPECT_EQ(it_sum, sum);
}

TEST(reverse_iterator, const_functionality) {
  constexpr Type value = 22;
  Container      c     = {value};

  // Check both copy construction and assignment:
  ConstIterator end{c, 0};
  ConstIterator begin{c, c.size()};

  auto e = end;
  auto b = begin;

  EXPECT_EQ(*begin, c[0]);

  // post fix
  EXPECT_EQ(begin++, b);
  EXPECT_EQ(end--, e);

  // Iterators are swapped, swap back, then prefix:
  std::swap(begin, end);
  EXPECT_EQ(++begin, e);
  EXPECT_EQ(--end, b);

  --begin;
  ++end;

  EXPECT_EQ(begin + 1, end);
  EXPECT_EQ(end - 1, begin);

  EXPECT_EQ(begin += 1, end);
  EXPECT_EQ(begin -= 1, b);

  EXPECT_EQ(begin + (end - begin), e);
  EXPECT_EQ(begin - (begin - end), e);
  EXPECT_EQ(end - (end - begin), b);
  EXPECT_EQ(end + (begin - end), b);

  EXPECT_EQ(begin[0], *begin);

  EXPECT_LT(begin, end);
  EXPECT_GT(end, begin);

  EXPECT_LE(begin, end);
  EXPECT_GE(begin, b);
  EXPECT_LE(end, e);
  EXPECT_GE(end, begin);

  const Type values = 10;
  Type       sum    = value;
  for (Type v = 1; v < Type{values}; ++v) {
    c.emplace_back(v);
    sum += v;
  }

  Type it_sum = 0;
  for (auto it = ConstIterator{c, c.size()}; it != ConstIterator{c, 0}; ++it) {
    it_sum += *it;
  }
  EXPECT_EQ(it_sum, sum);

  it_sum = 0;
  for (auto it = ConstIterator{c, c.size()}; it != ConstIterator{c, 0}; ++it) {
    it_sum += *it;
  }
  EXPECT_EQ(it_sum, sum);
}

#endif // SNOWFLAKE_TESTS_ECS_REVERSE_ITERATOR_HPP
