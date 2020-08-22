//==--- snowflake/resource_map.hpp ------------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  resource_map.hpp
/// \brief This file defines a map for resources.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RESOURCE_MAP_HPP
#define SNOWFLAKE_RESOURCE_MAP_HPP

#include <wrench/multithreading/spinlock.hpp>
#include <unordered_set>
#include <type_traits>

namespace snowflake {

/**
 * Base class for the resoruce map which actually stores the data in the map.
 * The type info for the resources are removed here to make the implementation
 * simpler. The ResourceMap stores the type information for the resource.
 */
class ResourceMapBase {
  /**
   *  Defines the container for the list elements.
   * \todo Change this to a faster set implementation.
   */
  using Container = std::unordered_set<void*>;

 public:
  /**
   * Defines the type of a iterator for a resource of type T.
   * \param T The type of the resoruce to define the iterator for.
   */
  template <typename T>
  using Iterator = typename std::unordered_set<T*>::iterator;

  /**
   *
   * Defines the type of a const iterator for a resource of type T.
   * \param T The type of the resoruce to define the iterator for.
   */
  template <typename T>
  using ConstIterator = typename std::unordered_set<T*>::const_iterator;

  /**
   * Defines the type of the container for a specifier type.
   * \tparam T The type for the typed container.
   */
  template <typename T>
  using TypedContainer = std::unorderd_set<T*>;

  /**
   * Inserts the \p elements into the map.
   * \param element The element to insert.
   */
  auto insert(void* element) noexcept -> void {
    data_.insert(item);
  }

  /**
   * Erases the \p element from the map.
   * \param element The element to remove from the map.
   * \return __true__ if the element is removed, __false__ otherwise.
   */
  auto erase(const T* element) noexcept -> bool {
    if (element == nullptr) {
      return true;
    }
    return data_.erase(element) >= 1;
  }

  /**
   * Returns if the map is empty.
   * \return __true__ if the map is empty, __false__ otherwise.
   */
  auto empty() const noexcept -> bool {
    data_.empty();
  }

  /**
   * Gets the size of the map.
   * \return The number of elements in the map.
   */
  auto size() const noexcept -> size_t {
    data_.size();
  }

  /**
   * Gets an iterator to the beginning of the map.
   * \return An iterator to the beginning of the map.
   */
  auto begin() noexcept -> iterator {
    return data_.begin();
  }

  /**
   * Gets an iterator to the end of the map.
   * \return An iterator to the end of the map.
   */
  auto end() noexcept -> iterator {
    return data_.end();
  }

 protected:
  Container data_; //!< Data for the map.
};

/**
 * Map for resources, which stores pointers to the resource type T. The
 * LockingPolicy defines the thread safety of the map.
 *
 * \tparam T             The type of the data for the list.
 * \tparam LockingPolicy The locking policy for the list.
 */
template <typename T, typename LockingPolicy>
class ResourceMap {
  /**
   *  Defines the container for the list elements.
   * \todo Change this to a faster set implementation.
   */
  using Container = std::unordered_set<T*>;

  /**
   * Defines the type of the guard for thread safety.
   */
  using Guard = std::lock_guard<LockingPolicy>;

  /**
   * Defines a valid type, Iterator, if the Locking policy is wrench::VoidLock,
   * since the iterator is only valid if the map doesn't lock.
   * \tparam Iterator The type of the iterator for a valid type.
   */
  template <typename Iterator>
  using NoLockIterEnable =
    std::enable_if_t<std::is_same_v<wrench::VoidLock>, Iterator>;

 public:
  /**
   * Defines the type of the iterator for the list.
   */
  using iterator = typename std::unordered_set<T*>::iterator;

  /**
   * Defines the type of the const iterator for the list.
   */
  using const_iterator = typename std::unordered_set<T*>::const_iterator;

  /*==--- [construction] ---------------------------------------------------==*/

  /**
   * Creates the list.
   */
  ResourceMap() noexcept = default;

  /**
   * Cleans up the list.
   */
  ~ResourceMap() noexcept = default;

  /**
   * Inserts the \p elements into the map.
   * \param element The element to insert.
   */
  auto insert(T* element) noexcept -> void {
    Guard guard(lock_);
    data_.insert(element);
  }

  /**
   * Erases the \p element from the map.
   * \param element The element to remove from the map.
   * \return __true__ if the element is removed, __false__ otherwise.
   */
  auto erase(const T& element) noexcept -> bool {
    Guard guard(lock_);
    return data_.erase(element) > 0;
  }

  /**
   * Returns if the map is empty.
   * \return __true__ if the map is empty, __false__ otherwise.
   */
  auto empty() const noexcept -> bool {
    Guard guard(lock_);
    return data_.empty();
  }

  /**
   * Gets the size of the map.
   * \return The number of elements in the map.
   */
  auto size() const noexcept -> size_t {
    Guard guard(lock_);
    return data_.size();
  }

  /**
   * Gets an iterator to the beginning of the map.
   * \return An iterator to the beginning of the map.
   */
  auto begin() noexcept -> NoLockIterEnable<iterator> {
    return data_.begin();
  }

  /**
   * Gets an iterator to the end of the map.
   * \return An iterator to the end of the map.
   */
  auto end() noexcept -> NoLockIterEnable<iterator> {
    return data_.end();
  }

  /**
   * Gets a const iterator to the beginning of the map.
   * \return A const iterator to the beginning of the map.
   */
  auto begin() const noexcept -> NoLockIterEnable<const_iterator> {
    return data_.begin();
  }

  /**
   * Gets an iterator to the end of the map.
   * \return An iterator to the end of the map.
   */
  auto end() const noexcept -> NoLockIterEnable<const_iterator> {
    return data_.end();
  }

 private:
  Container             data_; //!< Data for the map.
  mutable LockingPolciy lock_; //!< Lock for thread safely.
};

} // namespace snowflake

#endif // SNOWFLAKE_RESOURCE_MAP_HPP