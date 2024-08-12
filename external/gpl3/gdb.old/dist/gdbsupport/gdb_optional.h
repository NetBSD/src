/* An optional object.

   Copyright (C) 2017-2023 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef COMMON_GDB_OPTIONAL_H
#define COMMON_GDB_OPTIONAL_H

#include "gdbsupport/traits.h"

namespace gdb
{

struct in_place_t
{
  explicit in_place_t () = default;
};

constexpr gdb::in_place_t in_place {};

/* This class attempts to be a compatible subset of std::optional,
   which is slated to be available in C++17.  This class optionally
   holds an object of some type -- by default it is constructed not
   holding an object, but later the object can be "emplaced".  This is
   similar to using std::unique_ptr, but in-object allocation is
   guaranteed.

   Unlike std::optional, we currently only support copy/move
   construction/assignment of an optional<T> from either exactly
   optional<T> or T.  I.e., we don't support copy/move
   construction/assignment from optional<U> or U, when U is a type
   convertible to T.  Making that work depending on the definitions of
   T and U is somewhat complicated, and currently the users of this
   class don't need it.  */

template<typename T>
class optional
{
public:

  constexpr optional ()
    : m_dummy ()
  {}

  template<typename... Args>
  constexpr optional (in_place_t, Args &&... args)
    : m_item (std::forward<Args> (args)...),
      m_instantiated (true)
  {}

  ~optional ()
  { this->reset (); }

  /* Copy and move constructors.  */

  optional (const optional &other)
  {
    if (other.m_instantiated)
      this->emplace (other.get ());
  }

  optional (optional &&other)
    noexcept(std::is_nothrow_move_constructible<T> ())
  {
    if (other.m_instantiated)
      this->emplace (std::move (other.get ()));
  }

  constexpr optional (const T &other)
    : m_item (other),
      m_instantiated (true)
  {}

  constexpr optional (T &&other)
    noexcept (std::is_nothrow_move_constructible<T> ())
    : m_item (std::move (other)),
      m_instantiated (true)
  {}

  /* Assignment operators.  */

  optional &
  operator= (const optional &other)
  {
    if (m_instantiated && other.m_instantiated)
      this->get () = other.get ();
    else
      {
	if (other.m_instantiated)
	  this->emplace (other.get ());
	else
	  this->reset ();
      }

    return *this;
  }

  optional &
  operator= (optional &&other)
    noexcept (And<std::is_nothrow_move_constructible<T>,
	      std::is_nothrow_move_assignable<T>> ())
  {
    if (m_instantiated && other.m_instantiated)
      this->get () = std::move (other.get ());
    else
      {
	if (other.m_instantiated)
	  this->emplace (std::move (other.get ()));
	else
	  this->reset ();
      }
    return *this;
  }

  optional &
  operator= (const T &other)
  {
    if (m_instantiated)
      this->get () = other;
    else
      this->emplace (other);
    return *this;
  }

  optional &
  operator= (T &&other)
    noexcept (And<std::is_nothrow_move_constructible<T>,
	      std::is_nothrow_move_assignable<T>> ())
  {
    if (m_instantiated)
      this->get () = std::move (other);
    else
      this->emplace (std::move (other));
    return *this;
  }

  template<typename... Args>
  T &emplace (Args &&... args)
  {
    this->reset ();
    new (&m_item) T (std::forward<Args>(args)...);
    m_instantiated = true;
    return this->get ();
  }

  /* Observers.  */
  constexpr const T *operator-> () const
  { return std::addressof (this->get ()); }

  T *operator-> ()
  { return std::addressof (this->get ()); }

  constexpr const T &operator* () const &
  { return this->get (); }

  T &operator* () &
  { return this->get (); }

  T &&operator* () &&
  { return std::move (this->get ()); }

  constexpr const T &&operator* () const &&
  { return std::move (this->get ()); }

  constexpr explicit operator bool () const noexcept
  { return m_instantiated; }

  constexpr bool has_value () const noexcept
  { return m_instantiated; }

  /* 'reset' is a 'safe' operation with no precondition.  */
  void reset () noexcept
  {
    if (m_instantiated)
      this->destroy ();
  }

private:

  /* Destroy the object.  */
  void destroy ()
  {
    gdb_assert (m_instantiated);
    m_instantiated = false;
    m_item.~T ();
  }

  /* The get operations have m_instantiated as a precondition.  */
  T &get () noexcept
  {
#if defined(_GLIBCXX_DEBUG) && __cplusplus >= 201402L
    gdb_assert (this->has_value ());
#endif
    return m_item;
  }
  constexpr const T &get () const noexcept
  {
#if defined(_GLIBCXX_DEBUG) && __cplusplus >= 201402L
    gdb_assert (this->has_value ());
#endif
    return m_item;
  }

  /* The object.  */
  union
  {
    struct { } m_dummy;
    T m_item;
    volatile char dont_use; /* Silences -Wmaybe-uninitialized warning, see
			       PR gcc/80635.  */
  };

  /* True if the object was ever emplaced.  */
  bool m_instantiated = false;
};

}

#endif /* COMMON_GDB_OPTIONAL_H */
