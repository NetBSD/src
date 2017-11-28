/* An optional object.

   Copyright (C) 2017 Free Software Foundation, Inc.

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

#ifndef GDB_OPTIONAL_H
#define GDB_OPTIONAL_H

namespace gdb
{

/* This class attempts to be a compatible subset of std::optional,
   which is slated to be available in C++17.  This class optionally
   holds an object of some type -- by default it is constructed not
   holding an object, but later the object can be "emplaced".  This is
   similar to using std::unique_ptr, but in-object allocation is
   guaranteed.  */
template<typename T>
class optional
{
public:

  constexpr optional ()
    : m_dummy (),
      m_instantiated (false)
  {
  }

  ~optional ()
  {
    if (m_instantiated)
      destroy ();
  }

  /* These aren't deleted in std::optional, but it was simpler to
     delete them here, because currently the users of this class don't
     need them, and making them depend on the definition of T is
     somewhat complicated.  */
  optional (const optional &other) = delete;
  optional<T> &operator= (const optional &other) = delete;

  template<typename... Args>
  void emplace (Args &&... args)
  {
    if (m_instantiated)
      destroy ();
    new (&m_item) T (std::forward<Args>(args)...);
    m_instantiated = true;
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

private:

  /* Destroy the object.  */
  void destroy ()
  {
    gdb_assert (m_instantiated);
    m_instantiated = false;
    m_item.~T ();
  }

  /* The get operations have m_instantiated as a precondition.  */
  T &get () noexcept { return m_item; }
  constexpr const T &get () const noexcept { return m_item; }

  /* The object.  */
  union
  {
    struct { } m_dummy;
    T m_item;
  };

  /* True if the object was ever emplaced.  */
  bool m_instantiated;
};

}

#endif /* GDB_OPTIONAL_H */
