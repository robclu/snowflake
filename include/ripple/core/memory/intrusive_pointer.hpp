//==--- ripple/core/memory/intrusive_pointer.hpp ----------- -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  intrusive_pointer.hpp
/// \brief This file defines an intrusive shared pointer class.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_CORE_MEMORY_INTRUSIVE_POINTER_HPP
#define RIPPLE_CORE_MEMORY_INTRUSIVE_POINTER_HPP

#include "ref_tracker.hpp"
#include <memory>

namespace ripple {

//==--- [forward declarations & aliases] -----------------------------------==//

/// Forwrad declaration of an intrusive pointer.
/// \tparam T The type to wrap in an intrusive pointer.
template <typename T>
class IntrusivePtr;

/// Provides reference tracking and deleting functionality which can be
/// inherited to enable IntrusivePtr functionality.
/// \tparam T                The type of the pointer.
/// \tparam Deleter          The type of the deleter for the object.
/// \tparam ReferenceTracker The type of the refrence tracker.
template <
  typename T,
  typename Deleter          = std::default_delete<T>,
  typename ReferenceTracker = DefaultRefTracker>
class IntrusivePtrEnabled;

/// Alias for explicit single threaded intrusive pointer enable.
/// \tparam T       The type to enable intrusive pointer functionality for.
/// \tparam Deleter The type of the deleter.
template <typename T, typename Deleter = std::default_delete<T>>
using SingleThreadedIntrusivePtrEnabled =
  IntrusivePtrEnabled<T, Deleter, SingleThreadedRefTracker>;

/// Alias for explicit multi threaded intrusive pointer enable.
/// \tparam T       The type to enable intrusive pointer functionality for.
/// \tparam Deleter The type of the deleter.
template <typename T, typename Deleter = std::default_delete<T>>
using MultiThreadedIntrusivePtrEnabled =
  IntrusivePtrEnabled<T, Deleter, MultiThreadedRefTracker>;

/// Creates an intrusive pointer of type `IntrusivePtr<T>`, using the \p args to
/// construct the type T.
/// \param  args The args for construction of the type T.
/// \tparam T    The type to create an intrusive pointer for.
/// \tparam Args The types of the construction arguments.
template <typename T, typename... Args>
auto make_intrusive_ptr(Args&&... args) -> IntrusivePtr<T>;

//==--- [intrusive ptr enable] ---------------------------------------------==//

/// Provides reference tracking and deleting functionality which can be
/// inherited to enable IntrusivePtr functionality.
/// \tparam T                The type of the pointer.
/// \tparam Deleter          The type of the deleter for the object.
/// \tparam ReferenceTracker The type of the refrence tracker.
template <typename T, typename Deleter, typename ReferenceTracker>
class IntrusivePtrEnabled {
  /// Defines the type of this class.
  using Self = IntrusivePtrEnabled;

 public:
  //==--- [aliases] --------------------------------------------------------==//

  // clang-format off
  /// Defines the type of the base which requires the functionality.
  using EnabledType      = T;
  /// Defines the type of the intrusive pointer.
  using IntrusivePointer = IntrusivePtr<EnabledType>;
  /// Defines the type of the deleter.
  using DeleterType      = Deleter;
  /// Defines thee type of the reference tracker.
  using RefTracker       = ReferenceTracker;
  // clang-format on

  //==--- [construction] ---------------------------------------------------==//

  /// Constructor, which initializes the count and checks that the reference
  /// tracker implements the RefTracker interface.
  IntrusivePtrEnabled() {
    static_assert(
      is_ref_tracker_v<RefTracker>,
      "Reference tracker for intrusive ptr enabled type must implement the "
      "RefTracker interface.");
  }

  /// Here we enable move semantics as a performance optimization. It would be
  /// quite fine to just copy the intrusive pointer and then deleted the old
  /// one. However, in a multi-threaded context, this will result in two atomic
  /// operations, when we can just move the reference tracker and have none.
  /// \param other The other type to move.
  IntrusivePtrEnabled(IntrusivePtrEnabled&& other) noexcept = default;

  /// Here we enable move semantics as a performance optimization. It would be
  /// quite fine to just copy the intrusive pointer and then deleted the old
  /// one. However, in a multi-threaded context, this will result in two atomic
  /// operations, when we can just move the reference tracker and have none.
  /// \param other The other type to move.
  auto operator=(IntrusivePtrEnabled&& other) noexcept -> Self& = default;

  //==--- [deleted] --------------------------------------------------------==//

  // clang-format off
  /// Copy constructor -- deleted.
  IntrusivePtrEnabled(const IntrusivePtrEnabled&)    = delete;
  /// Copy assignment operator -- deleted.
  auto operator=(const IntrusivePtrEnabled&) -> void = delete;
  // clang-format on

  //==--- [implementation] -------------------------------------------------==//

  /// Releases the reference to the pointed to object, deleting the object if
  /// the reference count gets to zero.
  void release_reference() {
    if (_ref_tracker.release()) {
      _ref_tracker.destroy_resource(
        static_cast<EnabledType*>(this), DeleterType());
    }
  }

  /// Adds a reference to the tracked reference count.
  void add_reference() {
    _ref_tracker.add_reference();
  }

 protected:
  /// Creates a new intrusive pointer from the pointed to object, incrementing
  /// the reference count.
  auto reference_from_this() -> IntrusivePointer;

 private:
  RefTracker _ref_tracker; //!< The reference tracker.
};

//==--- [intrusive pointer] ------------------------------------------------==//

/// The IntrusivePtr type is a shared pointer implementation which is
/// intrusive. The reference count is stored in the intrusive pointer. It has
/// a smaller memroy footprint than `shared_ptr` and usually gives better
/// performance.
///
/// It additionally requires classes to inherit from `IntrusivePtrEnable<T,
/// Deleter, ReferenceType>`, which allows for custom specialization of the
/// deleter and referenc types, for single/multi-threaded use cases.
///
/// For a non-thread-safe, single-threaded optimized referencing, use
/// `SingleThreadedRefTracker`, while for a thread-safe, multi-threaded
/// optimized referencing, using `MultiThreadedRefTracker`.
///
/// Instances of instrusive pointer types should be create with
/// `make_intrusive_ptr(...)`, rather than through direct construction.
///
/// \tparam T The type to wrap in an intrusive pointer.
template <typename T>
class IntrusivePtr {
  /// Enable access for intrusive pointer type with different templates.
  template <typename U>
  friend class IntrusivePtr;

 public:
  //==--- [aliases] --------------------------------------------------------==//

  using Self     = IntrusivePtr; //!< The type of this class.
  using Ptr      = T*;           //!< Pointer type.
  using Ref      = T&;           //!< Reference type.
  using ConstPtr = const T*;     //!< Const pointer type.
  using ConstRef = const T&;     //!< Const reference type.

  /// Defines the type of intrusive enabled base for the type T.
  using IntrusiveEnabledBase = IntrusivePtrEnabled<
    typename T::EnabledType,
    typename T::DeleterType,
    typename T::RefTracker>;

  //==--- [construction] ---------------------------------------------------==//

  /// Default constructor.
  IntrusivePtr() = default;

  /// Constructor which takes a pointer \p ptr.
  /// \param data A pointer to the data.
  explicit IntrusivePtr(Ptr data) : _data(data) {}

  /// Copy constructor to create the intrusive pointer from \p other.
  IntrusivePtr(const Self& other) {
    *this = other;
  }

  /// Move the \p other intrusive pointer into this one.
  /// \param other The other intrusive pointer to move into this one.
  IntrusivePtr(Self&& other) noexcept {
    *this = std::move(other);
  }

  /// Copy constructor to create the intrusive pointer from \p other. This will
  /// fail at compile time if U is not derived from T, or convertible to T.
  /// \param  other The other intrusive pointer to copy from.
  /// \tparam U     The type of the other's pointed to data.
  template <typename U>
  IntrusivePtr(const IntrusivePtr<U>& other) {
    static_assert(
      std::is_base_of_v<T, U> || std::is_convertible_v<U, T>,
      "Types of pointed to data for the intrusive pointer are not compatible.");
    *this = other;
  }

  /// Moves the \p other intrusive pointer into this one. This will fail at
  /// compile time if U is not derived from T or is convertible to T.
  /// \param other The other type to move into this one.
  template <typename U>
  IntrusivePtr(IntrusivePtr<U>&& other) noexcept {
    static_assert(
      std::is_base_of_v<T, U> || std::is_convertible_v<U, T>,
      "Types of pointed to data for the intrusive pointer are not compatible.");
    *this = std::move(other);
  }

  /// Destructor to clean up the pointer.
  ~IntrusivePtr() {
    reset();
  }

  //==--- [operator overloads] ---------------------------------------------==//

  /// Copy assignment operator to set the intrusive pointer from the \p other
  /// intrusive pointer.
  /// \param other The other pointer to set this one from.
  auto operator=(const Self& other) -> Self& {
    if (this != &other) {
      // Reset incase this still points to something valid:
      reset();

      _data = other._data;
      if (_data) {
        as_intrusive_enabled()->add_reference();
      }
    }
    return *this;
  }

  /// Move assignment operator to move the \p other intrusive pointer into
  /// this one.
  /// \param other The other pointer to move into this one.
  auto operator=(Self&& other) noexcept -> Self& {
    if (this != &other) {
      reset();
      _data       = other._data;
      other._data = nullptr;
    }
    return *this;
  }

  /// Copy assignment operator to set the intrusive pointer from the \p other
  /// intrusive pointer, which wraps a __different__ type to this one. If the
  /// type of the \p other is not a base of T, or convertible to T, then this
  /// will cause a compile time error.
  /// \param  other The other pointer to set this one from.
  /// \tparam U     The type of the other pointer.
  template <typename U>
  auto operator=(const IntrusivePtr<U>& other) -> Self& {
    static_assert(
      std::is_base_of_v<T, U> || std::is_convertible_v<U, T>,
      "Types of pointed to data for the intrusive pointer are not compatible.");

    // Reset incase this class points to valid data:
    reset();
    _data = static_cast<Ptr>(other._data);

    if (_data) {
      as_intrusive_enabled()->add_reference();
    }
    return *this;
  }

  /// Move assignment operator to move the \p other intrusive pointer into this
  /// one. This will fail if U is not derived from T or convertible to T.
  /// \param  other The other pointer to set this one from.
  /// \tparam U     The type of the other pointer.
  template <typename U>
  auto operator=(IntrusivePtr<U>&& other) noexcept -> Self& {
    static_assert(
      std::is_base_of_v<T, U> || std::is_convertible_v<U, T>,
      "Types of pointed to data for the intrusive pointer are not compatible.");

    reset();
    _data       = static_cast<Ptr>(other._data);
    other._data = nullptr;
    return *this;
  }

  //==--- [access] ---------------------------------------------------------==//

  /// Returns a reference to the data.
  auto operator*() -> Ref {
    return *_data;
  }
  /// Returns a const reference to the data.
  auto operator*() const -> ConstRef {
    return *_data;
  }

  /// Returns a pointer to the data.
  auto operator->() -> Ptr {
    return _data;
  }
  /// Returns a const pointer to the data.
  auto operator->() const -> ConstPtr {
    return _data;
  }

  /// Returns a pointer to the data.
  auto get() -> Ptr {
    return _data;
  }

  /// Returns a const pointer to the data.
  auto get() const -> ConstPtr {
    return _data;
  }

  //==--- [conparison ops] -------------------------------------------------==//

  /// Returns true if the data is not a nullptr.
  explicit operator bool() const {
    return _data != nullptr;
  };

  /// Returns true if the pointer to \p other's data is the same as the pointer
  /// to this data.
  /// \param other The other pointer to compare with.
  auto operator==(const IntrusivePtr& other) const -> bool {
    return _data == other.data;
  }
  /// Returns true if the pointer to \p other's data is not the same as the
  /// pointer to this data.
  /// \param other The other pointer to compare with.
  auto operator!=(const IntrusivePtr& other) const -> bool {
    return _data != other._data;
  }

  //==--- [reset] ----------------------------------------------------------==//

  /// Resets the intrusive pointer by releasing the reference, and resetting the
  /// pointer to the data.
  auto reset() -> void {
    if (_data) {
      as_intrusive_enabled()->release_reference();
      _data = nullptr;
    }
  }

 private:
  Ptr _data = nullptr; //!< Pointer to the data.

  /// Returns a pointer to the upcasted intrusive pointer enabled base class.
  auto as_intrusive_enabled() -> IntrusiveEnabledBase* {
    static_assert(
      std::is_convertible_v<T*, IntrusiveEnabledBase*>,
      "IntrusivePtr requires type T to implement the IntrusivePtrEnabled "
      "interface.");
    return static_cast<IntrusiveEnabledBase*>(_data);
  }
};

//==--- [intrusive ptr enabled implemenatations] ---------------------------==//

/// Implementation for creating a reference from an intrusive pointer enbled
/// type.
/// \tparam T       The to create an intrusive pointer for.
/// \tparam Deleter The deleter for the object.
/// \tparam Tracker The type of the reference tracker.
template <typename T, typename Deleter, typename Tracker>
auto IntrusivePtrEnabled<T, Deleter, Tracker>::reference_from_this()
  -> IntrusivePtr<T> {
  add_reference();
  return IntrusivePtr<T>(static_cast<T*>(this));
}

//==--- [helper implementations] -------------------------------------------==//

/// Implementation of the intrusive pointer creation function.
template <typename T, typename... Args>
auto make_intrusive_ptr(Args&&... args) -> IntrusivePtr<T> {
  return IntrusivePtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace ripple

#endif // RIPPLE_CORE_MEMORY_INTRUSIVE_POINTER_HPP