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
#include <wrench/memory/allocator.hpp>
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
template <
  typename Entity,
  typename Allocator = wrench::ObjectPoolAllocator<Entity>>
class SparseSet {
  static_assert(
    std::is_convertible_v<Entity, size_t>,
    "Entity must be convertible to size type for use in sparse set!");

  /** Defines the type of a page. */
  using Page = Entity*;
  /** Defines a nullpage. */
  static constexpr Page nullpage = nullptr;

 public:
  /**
   * Defines the size type used for the sparse set.
   */
  using SizeType = size_t;

  /** Defines the size of the pages in the sparse array. */
  static constexpr size_t page_size = sparse_page_size;

  /*==--- [construction] ---------------------------------------------------==*/

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

  /*==--- [deleted] --------------------------------------------------------==*/

  /** Copy constructor -- deleted. */
  SparseSet(const SparseSet&) = delete;

  /** Move assignment operator -- deleted. */
  auto operator=(const SparseSet&) = delete;

  /*==--- [interface] ------------------------------------------------------==*/

  /**
   * Returns the capacity of the sparse set.
   *
   * The capacity is the number of entities which can be stored in the sparse
   * set, whithout additional allocation, and is the capacity of the internal
   * dense array.
   *
   * \return The capcacity of the internal dense array.
   */
  snowflake_nodiscard auto capacity() const noexcept -> SizeType {
    return dense_.capacity();
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
   * Reserves enough space to emplace \p size entities in the dense array
   * without allocation.
   */
  auto reserve(SizeType size) noexcept -> void {
    dense_.reserve(size);
  }

  /**
   * Determines if the sparse set is empty.
   * \return __true__ if the sparse set is empty.
   */
  snowflake_nodiscard auto empty() const noexcept -> bool {
    return _dense.empty();
  }

  /**
   * Gets the size of the sparse set.
   *
   * The number of elements in the sparse set is the total number of elements in
   * the internal densely packed array.
   *
   * \return The number of elements in the sparse set.
   */
  snowflake_nodiscard auto size() const noexcept -> SizeType {
    return dense_.size();
  }

  /**
   * Gets the index of the \p entity.
   *
   * \note This will assert in debug builds if the entity does not exist, and in
   *       release builds it will cause undefined behaviour.
   *
   * \param entity The entity to get the index of.
   * \return The index of the \p entity.
   */
  snowflake_nodicard auto
  index(const Entity& entity) const noexcept -> SizeType {
    assert(exists(entity) && "Can't get the index of an invalid entity!");
    return static_cast<SizeType>(sparse_entity(entity));
  }

  /**
   * Determines if the \p entity exists.
   *
   * \param entity The entity to determine if exists.
   * \return __true__ if the entity exists, false otherwise.
   */
  snowflake_nodiscard auto exists(const Entity& entity) const noexcept -> bool {
    const auto page = page_index(entity);
    return page < sparse_.size() && sparse_[page] &&
           !sparse_entity(entity).invalid();
  }

  /**
   * Emplaces an entity into the sparse set.
   *
   * \note If the internal sparse vector is not preallocated, then this will
   *       allocate a page of entities if the required page for the entity is
   *       not already allocated.
   *
   * \note This will assert in debug builds if the \p entity is already in the
   *       set.
   *
   * \param entity The entity to emplace into the set.
   */
  auto emplace(const Entity& entity) noexcept -> void {
    assert(!exists(entity) && "Entity already in sparse set!");
    sparse_entity(entity) = Entity{dense_.size()};
    dense_.emplace_back(entity);
  }

  /**
   * Removes the \p entity from the sparse set.
   *
   * \note If the entity does not exist then this will assert in debug builds,
   *       while in release builds it will cause undefined behaviour.
   *
   * \param entity The entity to remove.
   */
  auto erase(const Entity& entity) noexcept -> void {
    assert(exists(entity) && "Erasing an entity not in the sparse set!");
    const auto& curr_sparse = sparse_entity(entity);

    // Swap the one to remove with the back one in dense:
    dense_[curr_sparse]          = dense_.back();
    sparse_entity(dense_.back()) = curr_sparse;
    curr_sparse.reset();
    dense_.pop_back();
  }

  /**
   * Swaps two entities in the sparse set.
   *
   * \note This touches both the internal sparse and dense arrays.
   *
   * \note If either of the entities are not in the sparse set, then this will
   *       assert in debug, or be undefined in release.
   *
   * \param a An entity to swap with.
   * \param b An entity to swap with.
   */
  auto swap(const Entity& a, const Entity& b) noexcept -> void {
    assert(exists(a) && "Can't swap entity which doesn't exist!");
    assert(exists(b) && "Can't swap entity which doesn't exist!");

    auto& sparse_a = sparse_entity(a);
    auto& sparse_b = sparse_entity(b);
    std::swap(dense_[sparse_a], dense_[sparse_b]);
    std::swap(sparse_a, sparse_b);
  }

 private:
  std::vector<Page>   sparse_    = {};      //!< Sparse array.
  std::vector<Entity> dense_     = {};      //!< Dense array,
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
   * Gets the offset for the entity in the page it's assosciated to.
   * \param   entity The entity to get the index for.
   * \return  The offset of the entity in its page.
   */
  snowflake_nodiscard auto
  offset(const Entity& entity) const noexcept -> SizeType {
    return static_cast<SizeType>(entity) & (page_size - 1);
  }

  /**
   * Gets a reference to the page at \p index.
   *
   * \note This will call the allocator to allocate the page if the page at the
   *       given index had not been allocated. If the allocator is null, this
   *       will call the global allocator.
   *
   * \param index The index of the page to get.
   * \return A pointer to the page at the \p index.
   */
  snowflake_nodiscard auto fetch_page(SizeType index) noexcept -> Page& {
    constexpr size_t page_byte_size = sizeof(Entity) * page_size;
    while (sparse_.size() < index) {
      sparse_.emplace_back(nullpage);
    }
    if (sparse_[index] == nullpage) {
      sparse_[index] = static_cast<Page>(
        allocator_ ? allocator_.alloc(page_byte_size) : malloc(page_byte_size));

      for (auto *e = sparse_[index], *end = e + page_size; e != end; ++e) {
        e->reset();
      }
    }
    return sparse_[index];
  }

  /**
   * Returns a reference to an entity in the sparse vector for the given \p
   * entity.
   *
   * \note This will allocate a page for the entity if a page for the entity
   *       isn't already allocated.
   *
   * \param entity The entity to get from the sparse vector.
   */
  snowflake_nodiscard auto
  sparse_entity(const Entity& entity) noexcept -> Entity& {
    return fetch_page(entity)[offset(entity)];
  }
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_SPARSE_SET_HPP
