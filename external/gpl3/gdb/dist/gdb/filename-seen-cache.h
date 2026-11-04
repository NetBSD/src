/* Filename-seen cache for the GNU debugger, GDB.

   Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

#ifndef GDB_FILENAME_SEEN_CACHE_H
#define GDB_FILENAME_SEEN_CACHE_H

#include "gdbsupport/unordered_set.h"
#include "filenames.h"

/* Cache to watch for file names already seen.  */

class filename_seen_cache
{
public:
  filename_seen_cache () = default;

  DISABLE_COPY_AND_ASSIGN (filename_seen_cache);

  /* Empty the cache.  */
  void clear ()
  { m_tab.clear (); }

  /* If FILE is not already in the table of files of the cache, add it and
     return false; otherwise return true.

     NOTE: We don't manage space for FILE, we assume FILE lives as
     long as the caller needs.  */
  bool seen (const char *file)
  { return !m_tab.insert (file).second; }

private:
  struct hash
  {
    std::size_t operator() (const char *s) const noexcept
    { return filename_hash (s); }
  };

  struct eq
  {
    bool operator() (const char *lhs, const char *rhs) const noexcept
    { return filename_eq (lhs, rhs); }
  };

  /* Table of files seen so far.  */
  gdb::unordered_set<const char *, hash, eq> m_tab;
};

#endif /* GDB_FILENAME_SEEN_CACHE_H */
