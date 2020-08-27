//==--- snowflake/ecs/sparse_set.hpp ----------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  entity.hpp
/// \brief This file defines an entity, which is essentially just an integer.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ECS_SPARSE_SET_HPP
#define SNOWFLAKE_ECS_SPARSE_SET_HPP

#include "entity.hpp"

namespace snowflake {

/**
 * Defines the size of the sparse pages.
 */
static constexpr size_t sparse_page_size =
#if defined(SNOWFLAKE_SPARSE_PAGE_SIZE)
  SNOWFLAKE_SPARSE_PAGE_SIZE;
#else
  2 << 14;
#endif

/**
 * Implementation of a sparse set, which stores two vectors -- one which is
 * sparse and another which is dense. The sparse array does cause memory
 * bloat, but the mapping between sparse and dense allows the dense array to
 * __always__ remain packed, so it can iteration over it is very
 * cache-friendly.
 *
 * The use case is for iteration over the dense array, and the indirection
 * through the sparse array is only required when inserting and deleting from
 * the set, which should not be on the hot path when this type is used.
 */
template <typename Entitiy, typename Allocator>
class SparseSet {
  /** Defines the size of the pages in the sparse array. */
  static constexpr size_t page_size = sparse_page_size;

  /** Defines the type of a page. */
  using Page = Entity*;

 public:
  /**
   * Default constructor.
   */
  SparseSet() noexcept = default;

  /**
   * Constructor to set the allocator for the set.
   */
  SparseSet(Allocator* allocator) noexcept : allocator_{allocator_} {}

  /**
   * Destructor which cleans up the sparse pages.
   */
  ~SparseSet() noexcept {
    for (auto& page : sparse_) {
      allocator_ != nullptr ? allocator_.recycle(page) : std::free(page);
    }
  }

  std::vector<Page>   sparse_;              //!< Sparse array.
  std::vector<Entity> dense_;               //!< Dense array,
  Allocator*          allocator_ = nullptr; //!< Pointer to allocator.
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_SPARSE_SET_HPP
