//==--- snowflake/ecs/entity.hpp --------------------------- -*- C++ -*- ---==//
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

#ifndef SNOWFLAKE_ECS_ENTITY_HPP
#define SNOWFLAKE_ECS_ENTITY_HPP

#include <cstdint>
#include <snowflake/util/portability.hpp>

namespace snowflake {

/**
 * Entity class which is simply a handle to access the relevant components for
 * a given entity.
 */
class Entity {
 public:
  /**
   * Defines the type used for the entity id.
   */
  using IdType = uint32_t;

  /**
   * Defines the value of a null id for the entity.
   */
  static constexpr IdType null_id = std::numeric_limits<IdType>::max();

  /*==--- [construction] ---------------------------------------------------==*/

  // clang-format off
  
  /**
   * Constructor to initialize the entity with a valid id.
   * \param id The id for the entity.
   */
  explicit Entity(IdType id) noexcept : id_(id) {}

  /**
   * Default constructor which initializes an invalid entity.
   */
  constexpr Entity() noexcept              = default;

  /** Copy constrctor -- defaulted. */
  constexpr Entity(const Entity&) noexcept = default;
  /** Move constrctor -- defaulted. */
  constexpr Entity(Entity&&) noexcept      = default;

  /** Copy assignment -- defaulted. */
  constexpr auto operator=(const Entity&) noexcept -> Entity& = default;
  /** Move addignment -- defaulted. */
  constexpr auto operator=(Entity&&) noexcept      -> Entity& = default;
  // clang-format on

  /*==--- [operator overloads] ---------------------------------------------==*/

  /**
   * Equality comparison to compare this entity with the \p other.
   * \p other The other entity to compare against.
   * \return __true__ if the entities are the same.
   */
  constexpr auto operator==(Entity other) const noexcept -> bool {
    return id_ == other.id_;
  }

  /**
   * Inquality comparison to compare this entity with the \p other.
   * \p other The other entity to compare against.
   * \return __true__ if the entities are __not__ the same.
   */
  constexpr auto operator!=(Entity other) const noexcept -> bool {
    return id_ != other.id_;
  }

  /**
   * Less than comparison operator to enable sorting of entities.
   * \param other The other entitiy to compare to.
   * \return __true__ if this entity is strictly less than the \p other.
   */
  constexpr auto operator<(Entity other) const noexcept -> bool {
    return id_ < other.id_;
  }

  /*==--- [interface] ------------------------------------------------------==*/

  /**
   * Gets the id of the entity.
   * \return The entity id.
   */
  snowflake_nodiscard constexpr auto id() const noexcept -> IdType {
    return id_;
  }

  /**
   * Determines if the entity is invalid.
   * \return __true__ if the entity is invalid.
   */
  snowflake_nodiscard constexpr auto invalid() const noexcept -> bool {
    return id_ == null_id;
  }

  /**
   * Resets the entity by making it invalid.
   */
  constexpr auto reset() noexcept -> void {
    id_ = null_id;
  }

  /**
   * Overload of operator bool to evaluate the validity of the entity.
   * \return __true__ if the entity ius valid.
   */
  constexpr explicit operator bool() noexcept {
    return !invalid();
  }

 private:
  IdType id_ = null_id; //!< Id for the entity.

  /**
   * Constructor to initialize the entity with a valid id.
   * \param id The id for the entity.
   */
  explicit Entity(IdType id) noexcept : id_(id) {}
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_ENTITY_HPP
