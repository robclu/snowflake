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
  /** Defines the first id for components. */
  static constexpr Type start_id = 0;

  /**
   * Gets the next valid id, at *runtime*.
   * \return The next valid *runtime* id for a component.
   */
  snowflake_nodiscard static auto next() noexcept -> Type {
    static Type current{start_id};
    return current++;
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

  /**
   * Dynamic id value.
   * \return A unique id for the dynamic component.
   */
  static auto id() noexcept -> uint16_t {
    static const uint16_t v = ComponentIdDynamic::next();
    return v;
  }
};

/**
 * Specialization of the component id traits for a class which is a static
 * component id.
 * \tparam Value The value of the id.
 */
template <uint16_t IdValue>
struct ComponentIdTraits<ComponentIdStatic<IdValue>> {
  // clang-format off
  /** Type is a static component id. */
  static constexpr bool     is_static = true;
  /** The value of the component id. */
  static constexpr uint16_t value     = IdValue;
  // clang-format on

  /**
   * Dynamic id value.
   * \return A unique id for the dynamic component.
   */
  static constexpr auto id() noexcept -> uint16_t {
    return value;
  }
};

namespace detail {

/**
 * Helper struct to get the component id traits for a generic type T. This
 * prevents types which inherit from ComponentIsStatic to avoid havinf to
 * implement the traits type themselves.
 *
 * \tparam Derived The type of the derived type to get the component id traits
 *         for.
 */
template <typename Derived>
struct GetComponentIdTraits {
 private:
  /** The decayed type of the derived class. */
  using U = std::decay_t<Derived>;

  /**
   * Overload for types which are static component id's.
   * \tparam V The value of the id.
   */
  template <auto V>
  static auto
  test(ComponentIdStatic<V>*) -> ComponentIdTraits<ComponentIdStatic<V>>;

  /**
   * Overload for types which are not static component id's.
   */
  static auto test(...) -> ComponentIdTraits<U>;

 public:
  /** Defines the type for the traits. */
  using type = decltype(test(std::declval<U*>()));
};

} // namespace detail

/**
 * Returns the component traits for a decayed type T.
 * \tparam T The type to get the component traits for.
 */
template <typename T>
using component_id_traits_t = typename detail::GetComponentIdTraits<T>::type;

/**
 * Returns the id of the component, if the component is static, or a null id if
 * the type is not a static component id.
 * \tparam T The type to get the id for.
 */
template <typename T>
static constexpr auto component_id_v = component_id_traits_t<T>::value;

/**
 * Returns the id of the component, at runtime. This overload can be used for
 * for both static and dynamic component ids.
 * \tparam T The component to get the id for.
 */
template <typename T>
static auto component_id() -> uint16_t {
  return component_id_traits_t<T>::id();
}

/**
 * True if the type T has a constexpr component id value, false otherwise.
 * \tparam T The type to check if has a constexpr compononent id value.
 */
template <typename T>
static constexpr auto constexpr_component_id_v =
  component_id_traits_t<T>::is_static;

/**
 * Defines a valid type (int) if the type T has a constexpr component id value.
 * \tparam T The type to base the enable on.
 */
template <typename T>
using constexpr_component_enable_t =
  std::enable_if_t<constexpr_component_id_v<T>, int>;

/**
 * Defines a valid type (int) if the type T does not have a constexpr component
 * id value.
 * \tparam T The type to base the enable on.
 */
template <typename T>
using nonconstexpr_component_enable_t =
  std::enable_if_t<!constexpr_component_id_v<T>, int>;

} // namespace snowflake

#endif // SNOWFLAKE_ECS_COMPONENT_ID_HPP