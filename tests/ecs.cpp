//==--- snowflake/tests/ecs.cpp ---------------------------- -*- C++ -*- ---==//
//
//                                  Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  ecs.cpp
/// \brief This file implements tests for ecs functionality.
//
//==------------------------------------------------------------------------==//

#include "ecs/component_id.hpp"
#include "ecs/entity.hpp"
#include "ecs/entity_manager.hpp"
#include "ecs/component_storage.hpp"
#include "ecs/reverse_iterator.hpp"
#include "ecs/sparse_set.hpp"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}