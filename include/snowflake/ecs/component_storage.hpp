//==--- snowflake/ecs/component_storage.hpp ---------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  component_storage.hpp
/// \brief This file defines storage for component.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ECS_COMPONENT_STORAGE_HPP
#define SNOWFLAKE_ECS_COMPONENT_STORAGE_HPP

#include "sparse_set.hpp"

namespace snowflake {

/**
 * Implemenatation of a storage class for components. This is essentially just
 * a wrapper around a SparseSet which stores the entities and components such
 * that they are ordered the same way, and both contiguously.
 *
 * \note The ordering of the components and the entities are the same, so random
 *       access into the containers is valid.
 *
 * \note The order of insertion into the container is not preserved.
 *
 * \see SparseSet
 *
 * \tparam Entity The type of the entity.
 * \tparam Component The type of the component.
 * \tparam EntityAllocator The type of the entity allocator.
 */
template <
  typename Entity,
  typename Component,
  typename EntityAllocator = wrench::ObjectPoolAllocator<Entity>>
class ComponentStorage : public SparseSet<Entity, EntityAllocator> {
  // clang-format off
  /** Storage type for the entities */
  using Entities   = SparseSet<Entity, EntityAllocator>;
  /** Defines the type for the components. */
  using Components = std::vector<Component>;
  // clang-format on

 public:
  // clang-format off
  /** The size type. */
  using SizeType             = size_t;
  /** The iterator type used for the components. */
  using Iterator             = ReverseIterator<Components, false>;
  /** The const iterator type for the components. */
  using ConstIterator        = ReverseIterator<Components, true>;
  /** The reverse iterator type for the components. */
  using ReverseIterator      = Component*;
  /** The const reverse iterator type for the components. */
  using ConstReverseIterator = const Component*;
  // clang-format on

  /** The page size for the storage. */
  static constexpr size_t page_size = Entities::page_size;

  /**
   * Default constructor for storage -- this does not use an allocator for
   * entities.
   */
  ComponentStorage() noexcept = default;

  /**
   * Constructor which sets the allocator for the entities.
   * \param allocator The allocator for the entities.
   */
  ComponentStorage(EntityAllocator* allocator) noexcept : Entities{allocator} {}

  /**
   * Reserves enough space to emplace \p size compoennts.
   * \param size The number of entities to reserve.
   */
  auto reserve(SizeType size) noexcept -> void {
    components_.reserve(size);
    Entities::reserve(size);
  }

  /**
   * Emplaces a component into the storage.
   *
   * \note If the entity is already assosciated with this component, then this
   *       will cause undefined behaviour in release, or assert in debug.
   *
   * \param  entity The entity to emplace the component for.
   * \param  args   Arguments for the construciton of the component.
   * \tparam Args  The type of the args.
   */
  template <typename... Args>
  auto emplace(const Entity& entity, Args&&... args) -> void {
    // Many components are aggregates, and emplace back doesn't work with
    // aggregates, so we need to differentiate.
    if constexpr (std::is_aggregate_v<Component>) {
      components_.push_back(Component{std::forward<Args>(args)...});
    } else {
      components_.emplace_back(std::forward<Args>(args)...);
    }
    Entities::emplace(entity);
  }

  /**
   * Removes the component assosciated with the entity from the storage.
   *
   * \note If the entity does not exist then this will assert in debug builds,
   *       while in release builds it will cause undefined behaviour.
   *
   * \param entity The entity to remove.
   */
  auto erase(const Entity& entity) noexcept -> void {
    auto back                            = std::move(components_.back());
    components_[Entities::index(entity)] = std::move(back);
    components_.pop_back();
    Entities::erase(entity);
  }

  /**
   * Swaps two components in the storage.
   *
   *
   * \note If either of the entities assosciated with the components are not
   *       present then this will cause undefined behaviour in release, or
   *       assert in debug.
   *
   * \param a A component to swap with.
   * \param b A component to swap with.
   */
  auto swap(const Entity& a, const Entity& b) noexcept -> void {
    std::swap(components_[Entities::index(a)], components_[Entities::index(b)]);
    Entities::swap(a, b);
  }

  /**
   * Gets the component assosciated with the given entity.
   *
   * \note If the entity does not exist, this causes endefined behaviour in
   *       release, or asserts in debug.
   *
   * \param entity The entity to get the component for.
   * \return A reference to the component.
   */
  auto get(const Entity& entity) -> Component& {
    return components_[Entities::index(entity)];
  }

  /**
   * Gets the component assosciated with the given entity.
   *
   * \note If the entity does not exist, this causes endefined behaviour in
   *       release, or asserts in debug.
   *
   * \param entity The entity to get the component for.
   * \return A const reference to the component.
   */
  auto get(const Entity& entity) const -> const Component& {
    return components_[Entities::index(entity)];
  }

  /*==--- [iteration] ------------------------------------------------------==*/

  /**
   * Returns an iterator to the beginning of the components.
   *
   * The returned iterator points to the *most recently inserted* component and
   * iterates from *most* recent to *least* recently inserted.
   *
   * This iterator *is not* invalidated by insertion, but may be invalidated by
   * deletion.
   *
   * \return An iterator to the most recent component.
   */
  snowflake_nodiscard auto begin() noexcept -> Iterator {
    using size_type = typename Iterator::difference_type;
    return Iterator{components_, static_cast<size_type>(components_.size())};
  }

  /**
   * Returns an iterator to the end of the components.
   *
   * The returned iterator points to the *least recently inserted* component
   * and iterates from *most* recent to *least* recently inserted.
   *
   * This iterator *is not* invalidated by insertion, but may be invalidated by
   * deletion.
   *
   * \return An iterator to the most recent component.
   */
  snowflake_nodiscard auto end() noexcept -> Iterator {
    using size_type = typename Iterator::difference_type;
    return Iterator{components_, size_type{0}};
  }

  /**
   * Returns a const iterator to the beginning of components.
   *
   * The returned iterator points to the *most recently inserted* component and
   * iterates from *most* recent to *least* recently inserted.
   *
   * This iterator *is not* invalidated by insertion, but may be invalidated by
   * deletion.
   *
   * \return An iterator to the most recent component.
   */
  snowflake_nodiscard auto cbegin() const noexcept -> ConstIterator {
    using size_type = typename ConstIterator::difference_type;
    return ConstIterator{
      components_, static_cast<size_type>(components_.size())};
  }

  /**
   * Returns a const iterator to the end of the components.
   *
   * The returned iterator points to the *least recently inserted* component
   * and iterates from *most* recent to *least* recently inserted.
   *
   * This iterator *is not* invalidated by insertion, but may be invalidated by
   * deletion.
   *
   * \return An iterator to the most recent component.
   */
  snowflake_nodiscard auto cend() const noexcept -> ConstIterator {
    using size_type = typename ConstIterator::difference_type;
    return ConstIterator{components_, size_type{0}};
  }

  /**
   * Returns a reverse iterator to the beginning of the components.
   *
   * The returned iterator points to the *least recently inserted* component
   * and iterates from *lest* recent to *most* recently inserted.
   *
   * This iterator *is* invalidated by insertion and deletion.
   *
   * \return An iterator to the least recent entity in the sparse set.
   */
  snowflake_nodiscard auto rbegin() const noexcept -> ReverseIterator {
    return components_.data();
  }

  /**
   * Returns a reverse iterator to the end of the set.
   *
   * The returned iterator points to the *most recently inserted* component
   * and iterates from *least* recent to *most* recently inserted.
   *
   * This iterator *is* invalidated by insertion and deletion.
   *
   * \return An iterator to the least recent entity in the sparse set.
   */
  snowflake_nodiscard auto rend() const noexcept -> ReverseIterator {
    return rbegin() + components_.size();
  }

  /**
   * Returns a const reverse iterator to the beginning of the components.
   *
   * The returned iterator points to the *least recently inserted* component
   * and iterates from *lest* recent to *most* recently inserted.
   *
   * This iterator *is* invalidated by insertion and deletion.
   *
   * \return An iterator to the least recent entity in the sparse set.
   */
  snowflake_nodiscard auto crbegin() const noexcept -> ConstReverseIterator {
    return components_.data();
  }

  /**
   * Returns a const reverse iterator to the end of the set.
   *
   * The returned iterator points to the *most recently inserted* component
   * and iterates from *least* recent to *most* recently inserted.
   *
   * This iterator *is* invalidated by insertion and deletion.
   *
   * \return An iterator to the least recent entity in the sparse set.
   */
  snowflake_nodiscard auto crend() const noexcept -> ConstReverseIterator {
    return rbegin() + components_.size();
  }

  /*==--- [algorithms] -----------------------------------------------------==*/

  /**
   * Finds a component, if it exists.
   *
   * If the component doesn't exist, this returns an iterator to the end of the
   * set, otherwise it returns an iterator to the found component.
   *
   * \param entity The entity to find.
   * \return A valid iterator if found, otherwise an iterator to the end of the
   *         storage.
   */
  snowflake_nodiscard auto find(const Entity& entity) noexcept -> Iterator {
    return Entities::exists(entity)
             ? --Iterator(end() - Entities::index(entity))
             : end();
  }

  /**
   * Finds a component, if it exists.
   *
   * If the component doesn't exist, this returns an iterator to the end of the
   * set, otherwise it returns an iterator to the found component.
   *
   * \param entity The entity to find.
   * \return A valid iterator if found, otherwise an iterator to the end of the
   *         storage.
   */
  snowflake_nodiscard auto
  find(const Entity& entity) const noexcept -> ConstIterator {
    return Entities::exists(entity)
             ? --ConstIterator(end() - Entities::index(entity))
             : end();
  }

 private:
  Components components_ = {}; //!< Container of components.
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_COMPONENT_STORAGE_HPP
