/* Shards for the cooked index

   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_COOKED_INDEX_SHARD_H
#define GDB_DWARF2_COOKED_INDEX_SHARD_H

#include "dwarf2/cooked-index-entry.h"
#include "dwarf2/types.h"
#include "gdbsupport/gdb_obstack.h"
#include "addrmap.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/string-set.h"

/* An index of interesting DIEs.  This is "cooked", in contrast to a
   mapped .debug_names or .gdb_index, which are "raw".  An entry in
   the index is of type cooked_index_entry.

   Operations on the index are described below.  They are chosen to
   make it relatively simple to implement the symtab "quick"
   methods.  */
class cooked_index_shard
{
public:
  cooked_index_shard () = default;
  DISABLE_COPY_AND_ASSIGN (cooked_index_shard);

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
			   cooked_index_flag flags, enum language lang,
			   const char *name,
			   cooked_index_entry_ref parent_entry,
			   dwarf2_per_cu *per_cu);

  /* Install a new fixed addrmap from the given mutable addrmap.  */
  void install_addrmap (addrmap_mutable *map)
  {
    gdb_assert (m_addrmap == nullptr);
    m_addrmap = new (&m_storage) addrmap_fixed (&m_storage, map);
  }

  friend class cooked_index;

  /* A simple range over part of m_entries.  */
  typedef iterator_range<std::vector<cooked_index_entry *>::const_iterator>
       range;

  /* Return a range of all the entries.  */
  range all_entries () const
  {
    return { m_entries.cbegin (), m_entries.cend () };
  }

  /* Look up an entry by name.  Returns a range of all matching
     results.  If COMPLETING is true, then a larger range, suitable
     for completion, will be returned.  */
  range find (const std::string &name, bool completing) const;

private:

  /* Return the entry that is believed to represent the program's
     "main".  This will return NULL if no such entry is available.  */
  const cooked_index_entry *get_main () const
  {
    return m_main;
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu *lookup (unrelocated_addr addr)
  {
    if (m_addrmap == nullptr)
      return nullptr;

    return (static_cast<dwarf2_per_cu *> (m_addrmap->find ((CORE_ADDR) addr)));
  }

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  cooked_index_entry *create (sect_offset die_offset,
			      enum dwarf_tag tag,
			      cooked_index_flag flags,
			      enum language lang,
			      const char *name,
			      cooked_index_entry_ref parent_entry,
			      dwarf2_per_cu *per_cu);

  /* When GNAT emits mangled ("encoded") names in the DWARF, and does
     not emit the module structure, we still need this structuring to
     do lookups.  This function recreates that information for an
     existing entry, modifying ENTRY as appropriate.  Any new entries
     are added to NEW_ENTRIES.  */
  void handle_gnat_encoded_entry
       (cooked_index_entry *entry, htab_t gnat_entries,
	std::vector<cooked_index_entry *> &new_entries);

  /* Finalize the index.  This should be called a single time, when
     the index has been fully populated.  It enters all the entries
     into the internal table and fixes up all missing parent links.
     This may be invoked in a worker thread.  */
  void finalize (const parent_map_map *parent_maps);

  /* Storage for the entries.  */
  auto_obstack m_storage;
  /* List of all entries.  */
  std::vector<cooked_index_entry *> m_entries;
  /* If we found an entry with 'is_main' set, store it here.  */
  cooked_index_entry *m_main = nullptr;
  /* The addrmap.  This maps address ranges to dwarf2_per_cu objects.  */
  addrmap_fixed *m_addrmap = nullptr;
  /* Storage for canonical names.  */
  gdb::string_set m_names;
};

using cooked_index_shard_up = std::unique_ptr<cooked_index_shard>;

#endif /* GDB_DWARF2_COOKED_INDEX_SHARD_H */
