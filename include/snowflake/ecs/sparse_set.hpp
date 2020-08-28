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
#include <vector>

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
template <typename Entity, typename Allocator>
class SparseSet {
  /** Defines the size of the pages in the sparse array. */
  static constexpr size_t page_size = sparse_page_size;

  /** Defines the type of a page. */
  using Page = Entity*;

 public:
  /**
   * Defines the size type used for the sparse set.
   */
  using SizeType = size_t;

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

  /** Move constructor -- defaulted */
  SparseSet(SparseSet&&) noexcept = default;

  /** Move assignment operator -- defaulted. */
  auto operator=(SparseSet&&) noexcept -> SparseSet& = default;

  /*==--- [deletet] --------------------------------------------------------==*/

  /** Copy constructor -- deleted. */
  SparseSet(const SparseSet&) = delete;

  /** Move assignment operator -- deleted. */
  auto operator=(const SparseSet&) = delete;

  /*==--- [interface] ------------------------------------------------------==*/

  /**
   * Returns the capacity of the sparse set.
   *
   * The capacity is the number of pages which have been allocated in the
   * internal sparse array.
   *
   * \return The number of allocated pages in the internal sparse array.
   */
  snowflake_nodiscard auto capacity() const noexcept -> SizeType {
    return sparse_.size();
  }

  /**
   * Returns the extent of the sparse array, which the maximum value which can
   * be stored in the internal sparse array.
   *
   * \return The largest element which can be stored in the sparse array.
   */
  snowflake_nodiscard auto extent() const noexcept -> SizeType {
    return sparse_.size() * page_size;
  }

  /**
   * Gets the size of the sparse set.
   *
   * The number of elements in the sparse set is the total number of elements in
   * the internal densely packed array.
   *
   * \return The number of elements in the sparse set.
   */
  snowflake_nodiscard auto size() const noexcept -> Size {
    return dense_.size();
  }

  /**
   * Determines if the \p entity exists.
   *
   * \param entity The entity to determine if exists.
   * \return __true__ if the entity exists, false otherwise.
   */
  snowflake_nodiscard auto exists(const Entity& entity) const noexcept -> bool {
    const auto page = page_index(entity);
    return page < sparse_.size() && sparse[page] &&
           sparse[page][entity_index() entity] != Entity::invalid;
  }

 private:
  std::vector<Page>   sparse_;              //!< Sparse array.
  std::vector<Entity> dense_;               //!< Dense array,
  Allocator*          allocator_ = nullptr; //!< Pointer to allocator.

  /**
   * Gets the page index for the entity.
   * \param   entity The entity to get the page index for.
   * \return  The index of the page for the entity.
   */
  snowflake_nodiscard auto
  page_index(const Entity& entity) const noexcept -> SizeType {
    return static_cast<SizeType>(entity) / page_size;
  }

  /**
   * Gets the index for the entity in the page it's assosciated to.
   * \param   entity The entity to get the index for.
   * \return  The index of the entity in its page.
   */
  snowflake_nodiscard auto
  entity_index(const Entity& entity) const noexcept -> SizeType {
    return static_cast<SizeType>(entity) & (page_size - 1);
  }
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_SPARSE_SET_HPP
