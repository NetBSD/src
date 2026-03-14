/* String-interning set

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

#ifndef GDBSUPPORT_STRING_SET_H
#define GDBSUPPORT_STRING_SET_H

#include "gdbsupport/unordered_set.h"
#include <string_view>

namespace gdb
{

/* This is a string-interning set.  It manages storage for strings,
   ensuring that just a single copy of a given string is kept.  The
   underlying C string will remain valid for the lifetime of this
   object.  */

class string_set
{
public:

  /* Insert STR into this set.  Returns a pointer to the interned
     string.  */
  const char *insert (const char *str)
  {
    /* We need to take the length to hash the string anyway, so it's
       convenient to just wrap it here.  */
    return insert (std::string_view (str));
  }

  /* An overload accepting a string view.  */
  const char *insert (std::string_view str)
  {
    return m_set.insert (str).first->get ();
  }

  /* An overload that takes ownership of the string.  */
  const char *insert (gdb::unique_xmalloc_ptr<char> str)
  {
    return m_set.insert (local_string (std::move (str))).first->get ();
  }

private:

  /* The type of string we store.  Note that we do not store
     std::string here to avoid the small-string optimization
     invalidating a pointer on rehash.  */
  struct local_string
  {
    explicit local_string (std::string_view str)
      : contents (xstrndup (str.data (), str.size ())),
	len (str.size ())
    { }

    explicit local_string (gdb::unique_xmalloc_ptr<char> str)
      : contents (std::move (str)),
	len (strlen (contents.get ()))
    { }

    const char *get () const
    { return contents.get (); }

    std::string_view as_view () const
    { return std::string_view (contents.get (), len); }

    /* \0-terminated string contents.  */
    gdb::unique_xmalloc_ptr<char> contents;
    /* Length of the string.  */
    size_t len;
  };

  /* Equality object for the set.  */
  struct str_eq
  {
    using is_transparent = void;

    bool operator() (std::string_view lhs, const local_string &rhs)
      const noexcept
    {
      return lhs == rhs.as_view ();
    }

    bool operator() (const local_string &lhs, const local_string &rhs)
      const noexcept
    {
      return (*this) (lhs.as_view (), rhs);
    }
  };

  /* Hash object for the set.  */
  struct str_hash
  {
    using is_transparent = void;
    using is_avalanching = void;

    uint64_t operator() (const local_string &rhs) const noexcept
    {
      return (*this) (rhs.as_view ());
    }

    uint64_t operator() (std::string_view rhs) const noexcept
    {
      return ankerl::unordered_dense::hash<std::string_view> () (rhs);
    }
  };

  /* The strings.  */
  gdb::unordered_set<local_string, str_hash, str_eq> m_set;
};

}

#endif /* GDBSUPPORT_STRING_SET_H */
