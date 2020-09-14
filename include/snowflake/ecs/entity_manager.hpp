//==--- snowflake/ecs/entity_manager.hpp ------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  entity_manaher.hpp
/// \brief This file defines a manager for entities.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ECS_ENTITY_MANAGER_HPP
#define SNOWFLAKE_ECS_ENTITY_MANAGER_HPP

#include "entity.hpp"
#include "component_storage.hpp"
#include <wrench/memory/unique_ptr.hpp>

namespace snowflake {

/**
 * Manager class for entites and the components that are assosciated with the
 * entities.
 *
 * This class manages the creation and deletion of entities and components.
 * Storing the entities in a sparse set, and manages pools for each of the
 * components.
 *
 * This system is designed first for fast *iteration* performance, rather than
 * fast *insertion* and *deletion*. However, the insertion and deletion is fast
 * for the most part, *unless* a page needs to be allocated for an entity. This
 * overhead can be removed by preallocating enough space.
 *
 * \todo Add thread safety information.
 *
 * \tparam Entity    The type of the entities to manage.
 * \tparam Allocator The type of the allocator for the enitites.
 */
template <
  typename Entity,
  typename Allocator = wrench::ObjectPoolAllocator<Entity>>
class EntityManager {
  /** Defines the type of the pool data. */
  using PoolData = SparseSet<Entity, Allocator>;

  /**
   * Pool for a specific type of component.
   * \tparam Component The type of the component for the pool.
   */
  template <typename Component>
  struct ComponentPool final
  : public ComponentStorage<Entity, Component, Allocator> {
    /** Defines the type of the storage. */
    using Storage = ComponentStorage<Entity, Component, Allocator>;

    /**
     * Emplaces a component into the pool for a specific entity.
     * \param  manager The manager for the entities.
     * \param  entitiy The entity to add the component for.
     * \param  args    Arguments for the construction of the component.
     * \tparam Args    Type of the arguments.
     */
    template <typename... Args>
    auto emplace(EntityManager& manager, const Entity& entity, Args&&... args)
      -> void {
      // TODO: Add construction callback to call back into manager ...

      Storage::emplace(entity, std::forward<Args>(args)...);
    }

    /**
     * Removes the entity from the pool.
     * \param manager The manager for the entity.
     * \param entity  The entity to remove from the pool.
     */
    auto remove(EntityManager& manager, const Entity& entity) -> void {
      // TODO: Add destruction callback to notify manager ...

      Storage::remove(entity);
    }

    /* \todo sources and sinnks to the pool. */
  };

  /**
   * Thin wrapper around component storage for a component pool, which just
   * stores a pointer to the actual pool, and an id for the pool.
   *
   * This mainly serves to remove the component type from the pool so that the
   * manager can store a vector of pools.
   */
  struct ComponentPoolHandle {
    /** Type of the pointer to the pool. */
    using PoolPtr = wrench::UniquePtr<PoolData>;
    /** Type of the id for the pool. */
    using IdType = typename ComponentIdDynamic::Type;

    /**
     * Initializes the data for the pool.
     * \param id_value The value of the id for the pool.
     */
    auto initialize(IdType id_value) -> void {
      if (pool == nullptr) {
        pool = wrench::make_unique<PoolData>();
        id   = id_value;
      }
    }

    PoolPtr pool = nullptr;                     //!< Pointer to the pool.
    IdType  id   = ComponentIdDynamic::null_id; //!< Id of the component.
  };

  /** Defines the type of the pool for static component ids. */
  using Pools = std::vector<ComponentPoolHandle>;
  /** Defines the type of the entities. */
  using Entities = std::vector<Entity>;

 public:
  /**
   * Creates a new entity.
   * \return The created entity.
   */
  snowflake_nodiscard auto create() -> Entity {
    Entity entity;
    if (next_ == Entity::null_id) {
      entity = entities_.emplace_back(entities_.size());
    } else {
      entity = Entity{static_cast<typename Entity::IdType>(next_)};
      next_  = entities_[next_];
    }
    return entity;
  }

  /**
   * Recycles an entity, and all the components assosciated with it.
   * \param entity The entity to recycle.
   */
  auto recycle(const Entity& entity) -> void {
    using Id = typename Entity::IdType;
    // The entity is recycled by setting the value to the current next value,
    // which forms a chain of recycled entites.
    entities_[static_cast<size_t>(entity)] = Entity{static_cast<Id>(next_)};
    next_                                  = static_cast<size_t>(entity);
  }

  /**
   * Emplaces a component into the manager for the \p entity.
   * \param  entity The entity to add a component for.
   * \param  args   Arguments for the construction of the component.
   * \tparam Component The type of the component.
   * \tparam Args      The type of the arguments.
   */
  template <typename Component, typename... Args>
  auto emplace(const Entity& entity, Args&&... args) -> void {
    ensure_component<Component>().emplace(
      *this, entity, std::forward<Args>(args)...);
  }

  /**
   * Gets a reference to the component for the entity.
   * \param  entity    The entity to get the component for.
   * \tparam Component The type of the component.
   * \return A reference to the component for the entity.
   */
  template <typename Component>
  snowflake_nodiscard auto get(const Entity& entity) -> Component& {
    return ensure_component<Component>().get(entity);
  }

  /**
   * Gets a const reference to the component for the entity.
   * \param  entity    The entity to get the component for.
   * \tparam Component The type of the component.
   * \return A const reference to the component for the entity.
   */
  template <typename Component>
  snowflake_nodiscard auto get(const Entity& entity) const -> const Component& {
    return get_component<Component>().get(entity);
  }

  /**
   * Returns the number of components of the Component type.
   *
   * \note If the component has not been created, this will assert in debug, and
   *       cause undefined bahaviour in release.
   *
   * \tparam Component The type of the component to get the size of.
   * \return The number of component of type Component.
   */
  template <typename Component>
  snowflake_nodiscard auto size() const -> size_t {
    return get_component<Component>().size();
  }

  /**
   * Returns the number of entities that have been created.
   * \note This counts entities which have been created and then recycled.
   * \return The number of created entities.
   */
  snowflake_nodiscard auto entities_created() const noexcept -> size_t {
    return entities_.size();
  }

  /**
   * Returns the number of active entities.
   * \note The cost of this linear in the number of entities which have been
   *       recycled and not recreated.
   * \return The number of active entities.
   */
  snowflake_nodiscard auto entities_active() const noexcept -> size_t {
    size_t recycled = 0, next = next_;
    while (next != Entity::null_id && recycled < entities_.size()) {
      ++recycled;
      next = entities_[next];
    }
    return entities_created() - recycled;
  }

  /**
   * Gets the number of free entities which can be created without any
   * allocation.
   * \return The number of free entities.
   */
  snowflake_nodiscard auto entities_free() const noexcept -> size_t {
    return entities_created() - entities_active();
  }

 private:
  Entities   entities_         = {};      //!< All entities in the manager.
  Pools      static_id_pools_  = {};      //!< Pools with compile time ids.
  Pools      dynamic_id_pools_ = {};      //!< Pools with non compile time ids.
  Allocator* allocator_        = nullptr; //!< Allocator for the entities.
  size_t     next_             = Entity::null_id; //!< Index of the next entity.

  /**
   * Fetches the pool for a specific component. If the requested component type
   * doesn't exist then this will allocate a new pool for the component type.
   *
   * This overload is enabled for component types for which an id is defined at
   * compile time.
   *
   * \tparam Component The type of the component to fetch the pool for.
   */
  template <typename Component, constexpr_component_enable_t<Component> = 0>
  snowflake_nodiscard auto
  get_component() const -> const ComponentPool<Component>& {
    constexpr auto comp_id = component_id_v<Component>;
    assert(comp_id < static_id_pools_.size() && "Invalid component!");
    auto& pool = static_id_pools_[comp_id];
    assert(pool.pool != nullptr && "Unallocated component pool!");

    return *static_cast<const ComponentPool<Component>*>(pool.pool.get());
  }

  /**
   * Fetches the pool for a specific component. If the requested component
   * type doesn't exist then this will allocate a new pool for the component
   * type.
   *
   * This overload is enabled for types for which the component id is *not*
   * defined at compile time. It needs to search through the components
   *
   * \tparam Component The type of the component to fetch the pool for.
   */
  template <typename Component, nonconstexpr_component_enable_t<Component> = 0>
  snowflake_nodiscard auto
  get_component() const -> const ComponentPool<Component>& {
    const auto comp_id = component_id<Component>();
    assert(comp_id < dynamic_id_pools_.size());
    auto& pool = dynamic_id_pools_[comp_id];
    assert(pool.pool != nullptr && "Unallocated component pool!");

    return *static_cast<const ComponentPool<Component>*>(pool.pool.get());
  }

  /**
   * Fetches the pool for a specific component. If the requested component
   * type doesn't exist then this will allocate a new pool for the component
   * type.
   *
   * This overload is enabled for component types for which an id is defined
   * at compile time.
   *
   * \tparam Component The type of the component to fetch the pool for.
   */
  template <typename Component, constexpr_component_enable_t<Component> = 0>
  snowflake_nodiscard auto ensure_component() -> ComponentPool<Component>& {
    constexpr auto comp_id = component_id_v<Component>;
    while (comp_id >= static_id_pools_.size()) {
      static_id_pools_.emplace_back();
    }
    auto& pool = static_id_pools_[comp_id];
    pool.initialize(comp_id);

    return *static_cast<ComponentPool<Component>*>(pool.pool.get());
  }

  /**
   * Fetches the pool for a specific component. If the requested component
   * type doesn't exist then this will allocate a new pool for the component
   * type.
   *
   * This overload is enabled for types for which the component id is *not*
   * defined at compile time. It needs to search through the components
   *
   * \tparam Component The type of the component to fetch the pool for.
   */
  template <typename Component, nonconstexpr_component_enable_t<Component> = 0>
  snowflake_nodiscard auto ensure_component() -> ComponentPool<Component>& {
    const auto comp_id = component_id<Component>();
    while (comp_id >= dynamic_id_pools_.size()) {
      dynamic_id_pools_.emplace_back();
    }
    auto& pool = dynamic_id_pools_[comp_id];
    pool.initialize(comp_id);

    return *static_cast<ComponentPool<Component>*>(pool.pool.get());
  }
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_ENTITY_MANAGER_HPP
