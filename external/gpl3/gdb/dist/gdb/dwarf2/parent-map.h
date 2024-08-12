/* DIE indexing 

   Copyright (C) 2024 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_PARENT_MAP_H
#define GDB_DWARF2_PARENT_MAP_H

#include <algorithm>

class cooked_index_entry;

/* A class that handles mapping from a DIE range to a parent
   entry.

   The generated DWARF can sometimes have the declaration for a method
   in a class (or perhaps namespace) scope, with the definition
   appearing outside this scope... just one of the many bad things
   about DWARF.  In order to handle this situation, we defer certain
   entries until the end of scanning, at which point we'll know the
   containing context of all the DIEs that we might have scanned.  */
class parent_map
{
public:

  parent_map () = default;
  ~parent_map () = default;

  /* Move only.  */
  DISABLE_COPY_AND_ASSIGN (parent_map);
  parent_map (parent_map &&) = default;
  parent_map &operator= (parent_map &&) = default;

  /* A reasonably opaque type that is used here to combine a section
     offset and the 'dwz' flag into a single value.  */
  enum addr_type : CORE_ADDR { };

  /* Turn a section offset into a value that can be used in a parent
     map.  */
  static addr_type form_addr (sect_offset offset, bool is_dwz)
  {
    CORE_ADDR value = to_underlying (offset);
    if (is_dwz)
      value |= ((CORE_ADDR) 1) << (8 * sizeof (CORE_ADDR) - 1);
    return addr_type (value);
  }

  /* Add a new entry to this map.  DIEs from START to END, inclusive,
     are mapped to PARENT.  */
  void add_entry (addr_type start, addr_type end,
		  const cooked_index_entry *parent)
  {
    gdb_assert (parent != nullptr);
    m_map.set_empty (start, end, (void *) parent);
  }

  /* Look up an entry in this map.  */
  const cooked_index_entry *find (addr_type search) const
  {
    return static_cast<const cooked_index_entry *> (m_map.find (search));
  }

  /* Return a fixed addrmap that is equivalent to this map.  */
  addrmap_fixed *to_fixed (struct obstack *obstack) const
  {
    return new (obstack) addrmap_fixed (obstack, &m_map);
  }

private:

  /* An addrmap that maps from section offsets to cooked_index_entry *.  */
  addrmap_mutable m_map;
};

/* Keep a collection of parent_map objects, and allow for lookups
   across all of them.  */
class parent_map_map
{
public:

  parent_map_map () = default;
  ~parent_map_map () = default;

  DISABLE_COPY_AND_ASSIGN (parent_map_map);

  /* Add a parent_map to this map.  */
  void add_map (const parent_map &map)
  {
    m_maps.push_back (map.to_fixed (&m_storage));
  }

  /* Look up an entry in this map.  */
  const cooked_index_entry *find (parent_map::addr_type search) const
  {
    for (const auto &iter : m_maps)
      {
	const cooked_index_entry *result
	  = static_cast<const cooked_index_entry *> (iter->find (search));
	if (result != nullptr)
	  return result;
      }
    return nullptr;
  }

private:

  /* Storage for the convert maps.  */
  auto_obstack m_storage;

  /* While conceptually this class is a combination of parent_maps, in
     practice it is just a number of fixed maps.  This is important
     because we want to allow concurrent lookups, but a mutable
     addrmap is based on a splay-tree, which is not thread-safe, even
     for nominally read-only lookups.  */
  std::vector<addrmap_fixed *> m_maps;
};

#endif /* GDB_DWARF2_PARENT_MAP_H */
