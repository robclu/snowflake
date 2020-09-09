//==--- snowflake/ecs/component_id.hpp --------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  component_id.hpp
/// \brief This file defines functionality for creating component ids.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ECS_COMPONENT_ID_HPP
#define SNOWFLAKE_ECS_COMPONENT_ID_HPP

#include <cstdint>
#include <limits>
#include <snowflake/util/portability.hpp>
#include <wrench/utils/type_traits.hpp>

namespace snowflake {

/**
 * Compile-time component id struct.
 * \tparam Value The value of the id.
 */
template <uint16_t Value>
struct ComponentIdStatic {
  /**
   * Overload of operator to convert to the underlying type.
   */
  constexpr operator uint16_t() const noexcept {
    return Value;
  }
};

/**
 * Thin wrapper around an unsigned int to represent the id of a component. This
 * class represents a runtime component id.
 */
struct ComponentIdDynamic {
  /** The type used for component ids. */
  using Type = uint16_t;

  /** Null id value for components. */
  static constexpr Type null_id = std::numeric_limits<Type>::max();

  Type value = null_id; //!< Value of the component id.

  /**
   * Gets the next valid id, at *runtime*.
   * \return The next valid *runtime* id for a component.
   */
  snowflake_nodiscard static auto next() noexcept -> Type {
    static Type current{0};
    return current++;
  }

  /**
   * Determines if the id is invalid.
   * \return __true__ if the id is invalid.
   */
  snowflake_nodiscard constexpr auto invalid() const noexcept -> bool {
    return value == null_id;
  }

  /**
   * Overload of operator bool to evaluate the validity of the id.
   * \return __true__ if the id is valid.
   */
  constexpr explicit operator bool() noexcept {
    return !invalid();
  }

  /**
   * Overload of operator to convert to the underlying type.
   */
  constexpr operator Type() const noexcept {
    return value;
  }
};

/**
 * Returns true if the type T is a base of the static component id type.
 * \tparam T The type to determine
 */
template <typename T>
struct ComponentIdTraits {
  // clang-format off
  /** Type is not a static component id. */
  static constexpr bool     is_static = false;
  /** The value of the component id. */
  static constexpr uint16_t value     = ComponentIdDynamic::null_id;
  // clang-format on
};

/**
 * Specialization of the component id traits for a class which is a static
 * component id.
 * \tparam Value The value of the id.
 */
template <uint16_t IdValue>
struct ComponentIdTraits<ComponentIdStatic<IdValue>*> {
  // clang-format off
  /** Type is a static component id. */
  static constexpr bool     is_static = true;
  /** The value of the component id. */
  static constexpr uint16_t value     = IdValue;
  // clang-format on
};

/**
 * Returns the component traits for a decayed type T.
 * \tparam T The type to get the component traits for.
 */
template <typename T>
using component_id_traits_t = ComponentIdTraits<std::decay_t<T>>;

/**
 * Returns the id of the component, if the component is static, or a null id if
 * the type is not a static component id.
 * \tparam T The type to get the id for.
 */
template <typename T>
static constexpr auto component_id_v = component_id_traits_t<T>::value;

} // namespace snowflake

#endif // SNOWFLAKE_ECS_COMPONENT_ID_HPP