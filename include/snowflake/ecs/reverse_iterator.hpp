//==--- snowflake/ecs/reverse_iterator.hpp ----------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  reverse_iterator.hpp
/// \brief This file defines a reverse iterator.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_ECS_REVERSE_ITERATOR_HPP
#define SNOWFLAKE_ECS_REVERSE_ITERATOR_HPP

#include <snowflake/util/portability.hpp>
#include <type_traits>

namespace snowflake {

/**
 * Iterator class which will iterate over a container in reverse.
 *
 * This iterator iterates *backwards*, and is designed so such that it can
 * iterate over containers which insert into the end of the container, so that
 * iteration *is not* invalidated when elements are pushed onto the back of the
 * iterator.
 *
 * The compiler *should* optimize the contiguous reverse iteration just
 * as effectively as contiguous forward itertion. Benchmarks this for specific
 * use cases.
 *
 * \note This only works with container types which have operator[], and which
 *       store data contiguously. It's designed for use with vector, array, and
 *       SparseSet, but any container which meets this criteria is valid.
 *
 * \tparam Container The container to iterate over.
 * \tparam IsConst   If the iterator is const.
 * \tparam DiffType  The difference type for the iterator.
 */
template <typename Container, bool IsConst = true, typename DiffType = int64_t>
class ReverseIterator {
  // clang-format off
  /** Defines the value type of the elements in the container. */
  using Value = std::decay_t<decltype(std::declval<Container>()[0])>;
  /** Defines the type of the data. */
  using Data = std::conditional_t<IsConst, const Container, Container>;

  /**
   * Returns type A if this is a const iterator, otherwise type B.
   * \tparam A The type to return if a const iterator.
   * \tparam B The type to return if a non const iterator.
   */
  template <typename A, typename B>
  using GetType = std::conditional_t<IsConst, A, B>;

 public:
  /** Difference type for the iterator. */
  using difference_type   = DiffType;
  /** Value type for the iterator. */
  using value_type        = Value;
  /** Pointer type for the iterator. */
  using pointer           = GetType<const value_type*, value_type*>;
  /** Reference type for the iterator. */
  using reference         = GetType<const value_type&, value_type&>;
  /** Category for the iterator. */
  using iterator_category = std::random_access_iterator_tag;
  // clang-format on

 public:
  /**
   * Default constructor for the iterator.
   */
  ReverseIterator() noexcept = default;

  /**
   * Constructor to iniitalize the iterator from a reference to the container
   * and a \p position into the container.
   *
   * \param data     The container to iterate over.
   * \param position The position in the array.
   */
  ReverseIterator(Data& data, difference_type position) noexcept
  : data_{&data}, pos_{position} {}

  /**
   * Overload of prefix increment operator.
   *
   * \note This iterates *backwards* because this allows elements to be
   *       added to the end of the container without invalidating the iterator.
   *
   * \return A reference to the modified iterator.
   */
  auto operator++() noexcept -> ReverseIterator& {
    --pos_;
    return *this;
  }

  /**
   * Overload of postfix increment operator.
   *
   * \note This iterates *backwards* because this allows elements to be added
   *       to the end of the container without invalidating the iterator.
   *
   * \return The new iterator with the original position.
   */
  auto operator++(int) noexcept -> ReverseIterator {
    // clang-format off
      ReverseIterator curr = *this;
      operator++();
      return curr;
    // clang-format on
  }

  /**
   * Overload of prefix decrement operator.
   *
   * \note This iterates *forwards* because it needs to be the reverse of
   *       operator++, \sa operator++.
   *
   * \return A reference to the modified iterator.
   */
  auto operator--() noexcept -> ReverseIterator& {
    ++pos_;
    return *this;
  }

  /**
   * Overload of postfix decrement operator.
   *
   * \note This iterates *forwards* because it needs to be the reverse of
   *       operator++, \sa operator++.
   *
   * \return The new iterator with the original position.
   */
  auto operator--(int) noexcept -> ReverseIterator {
    // clang-format off
      ReverseIterator curr = *this;
      operator--();
      return curr;
    // clang-format on
  }

  /**
   * Overload of operator+= to offset the iterator.
   *
   * \param amount The amount to offset by.
   * \return A reference to the offset iterator.
   */
  auto operator+=(const difference_type amount) noexcept -> ReverseIterator& {
    pos_ -= amount;
    return *this;
  }

  /**
   * Overload of operator+ to get an iterator at an offset from this one.
   *
   * \param amount The amount to offset by.
   * \return A new iterator offset from this one by \p amount.
   */
  auto
  operator+(const difference_type amount) const noexcept -> ReverseIterator {
    ReverseIterator result = *this;
    return (result += amount);
  }

  /**
   * Overload of operator-= to offset the iterator.
   *
   * \param amount The amount to offset by.
   * \return A reference to the offset iterator.
   */
  auto operator-=(const difference_type amount) noexcept -> ReverseIterator& {
    pos_ += amount;
    return *this;
  }

  /**
   * Overload of operator- to get an iterator at an offset from this one.
   *
   * \param amount The amount to offset by.
   * \return A new iterator offset from this one by \p amount.
   */
  auto
  operator-(const difference_type amount) const noexcept -> ReverseIterator {
    ReverseIterator result = *this;
    return (result -= amount);
  }

  /**
   * Overload of operator- to get the distance between two iterators.
   *
   * \param other The other iterator to get the distance to.
   * \return The distance between two iterators.
   */
  auto
  operator-(const ReverseIterator& other) const noexcept -> difference_type {
    return other.pos_ - pos_;
  }

  /**
   * Index operator to get the iterated type at an offset from this iterators
   * element.
   *
   * \param index The offset of the element from this one.
   * \return A reference to the element.
   */
  snowflake_nodiscard auto
  operator[](const difference_type index) const -> reference {
    assert(data_ != nullptr && "Invalid iterator access!");
    const auto pos = pos_ - index - difference_type{1};
    return (*data_)[pos];
  }

  /**
   * Equality comparison operator.
   * \param other The other iterator to compare to.
   * \return __true__ if the iterators are equal.
   */
  snowflake_nodiscard auto
  operator==(const ReverseIterator& other) const noexcept -> bool {
    return other.pos_ == pos_;
  }

  /**
   * Inequality comparison operator.
   * \param other The other iterator to compare with.
   * \return __true__ if the iterators are not equal.
   */
  snowflake_nodiscard auto
  operator!=(const ReverseIterator& other) const noexcept -> bool {
    return other.pos_ != pos_;
  }

  /**
   * Less than comparison operator.
   * \param other The other iterator to compare to.
   * \return __true__ if this iterator is less than the \p other.
   */
  snowflake_nodiscard auto
  operator<(const ReverseIterator& other) const noexcept -> bool {
    return pos_ > other.pos_;
  }

  /**
   * Greather than comparison operator.
   * \param other The other iterator to compare to.
   * \return __true__ if this iterator is greater than the \p other.
   */
  snowflake_nodiscard auto
  operator>(const ReverseIterator& other) const noexcept -> bool {
    return pos_ < other.pos_;
  }

  /**
   * Less than or equal to comparison operator.
   * \param other The other iterator to compare to.
   * \return __true__ if this iterator is less than or equaly to the \p other.
   */
  snowflake_nodiscard auto
  operator<=(const ReverseIterator& other) const noexcept -> bool {
    return !(*this > other);
  }

  /**
   * Greater than or equal to comparison operator.
   * \param other The other iterator to compare to.
   * \return __true__ if this iterator is less than or equaly to the \p other.
   */
  snowflake_nodiscard auto
  operator>=(const ReverseIterator& other) const noexcept -> bool {
    return !(*this < other);
  }

  // clang-format off
  /**
   * Overload of arrow operator to access a pointer to the iterated object.
   * \return A pointer to the iterated type.
   */
  snowflake_nodiscard auto operator->() const -> pointer {
    assert(data_ != nullptr && "Invalid iterator access!");
    const auto pos = pos_ - difference_type{1};
    return &(*data_)[pos];
  }
  // clang-format on

  /**
   * Overload of derference operator to get a reference to the iterated
   * object.
   * \return A reference to the iterated type.
   */
  snowflake_nodiscard auto operator*() const -> reference {
    assert(data_ != nullptr && "Invalid iterator access!");
    const auto pos = pos_ - difference_type{1};
    return (*data_)[pos];
  }

 private:
  // Note, we store a poitner here so that we can default construct the
  // iterator, and so that the access can be asserted in debug.

  Data*           data_ = nullptr; //!< Pointer to the data.
  difference_type pos_  = 0;       //!< Current index into the dense data.
};

} // namespace snowflake

#endif // SNOWFLAKE_ECS_REVERSE_ITERATOR_HPP
