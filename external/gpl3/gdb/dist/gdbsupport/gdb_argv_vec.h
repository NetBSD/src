/* Wrap std::vector<char *> that owns the 'char *' strings.

   Copyright (C) 2025 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_GDB_ARGV_VEC_H
#define GDBSUPPORT_GDB_ARGV_VEC_H

#include "gdbsupport/common-utils.h"
#include <vector>

namespace gdb
{

/* A class that wraps around a vector of 'char *' strings.  The class owns
   the strings within the vector and will release them by calling xfree()
   on each string when this classes destructor is called.  */
class argv_vec final
{
  /* Vector of strings.  This class owns the strings within the vector.
     This is at the start of the class so we can use decltype().  */
  std::vector<char *> m_args;

public:

  using value_type = decltype (m_args)::value_type;
  using iterator = decltype (m_args)::iterator;
  using const_iterator = decltype (m_args)::const_iterator;
  using reference = decltype (m_args)::reference;
  using const_reference = decltype (m_args)::const_reference;

  argv_vec ()
  {
    /* Nothing.  */
  }

  /* Move the owned strings from OTHER.  */
  argv_vec (argv_vec &&other)
  {
    this->m_args = std::move (other.m_args);
  }

  /* Don't currently support copying the strings from OTHER.  */
  argv_vec (const argv_vec &other) = delete;

  /* Move elements from OTHER.  Free currently owned strings.  */
  argv_vec &operator= (argv_vec &&other)
  {
    free_vector_argv (m_args);
    this->m_args = std::move (other.m_args);
    return *this;
  }

  /* Don't currently support copying the strings from OTHER.  */
  argv_vec &operator= (const argv_vec &other) = delete;

  /* Release the owned strings.  */
  ~argv_vec ()
  {
    free_vector_argv (m_args);
  }

  /* Append VALUE to the end of m_args.  This class takes ownership of
     VALUE, and will release VALUE by calling xfree() on it when this
     object is destroyed.  */
  void push_back (const value_type &value)
  {
    m_args.push_back (value);
  }

  /* Append VALUE to the end of m_args.  This class takes ownership of
     VALUE, and will release VALUE by calling xfree() on it when this
     object is destroyed.  */
  void push_back (value_type &&value)
  {
    m_args.push_back (value);
  }

  /* Non constant iterator to start of m_args.  */
  iterator begin ()
  {
    return m_args.begin ();
  }

  /* Non constant iterator to end of m_args.  */
  iterator end ()
  {
    return m_args.end ();
  }

  /* Constant iterator to start of m_args.  */
  const_iterator begin () const
  {
    return m_args.begin ();
  }

  /* Constant iterator to end of m_args.  */
  const_iterator end () const
  {
    return m_args.end ();
  }

  /* Return contiguous block of 'char *' pointers.  Ideally this would be
     const, but the execve interface to which this data is passed doesn't
     accept 'const char **'.  */
  char **argv ()
  {
    return m_args.data ();
  }

  /* Return a constant reference to the underlying vector.  */
  const decltype (m_args) &get () const
  {
    return m_args;
  }

  /* Return true when m_args is empty.  */
  bool empty () const
  {
    return m_args.empty ();
  }
};
} /* namespac gdb */

#endif /* GDBSUPPORT_GDB_ARGV_VEC_H */
