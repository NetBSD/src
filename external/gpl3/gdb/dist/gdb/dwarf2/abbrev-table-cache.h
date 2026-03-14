/* DWARF abbrev table cache

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

#ifndef GDB_DWARF2_ABBREV_TABLE_CACHE_H
#define GDB_DWARF2_ABBREV_TABLE_CACHE_H

#include "dwarf2/abbrev.h"
#include "gdbsupport/unordered_set.h"

/* An abbrev table cache holds abbrev tables for easier reuse.  */
class abbrev_table_cache
{
public:
  abbrev_table_cache () = default;
  DISABLE_COPY_AND_ASSIGN (abbrev_table_cache);

  abbrev_table_cache (abbrev_table_cache &&) = default;
  abbrev_table_cache &operator= (abbrev_table_cache &&) = default;

  /* Find an abbrev table coming from the abbrev section SECTION at
     offset OFFSET.  Return the table, or nullptr if it has not yet
     been registered.  */
  const abbrev_table *find (dwarf2_section_info *section,
			    sect_offset offset) const
  {
    abbrev_table_search_key key {section, offset};

    if (auto iter = m_tables.find (key);
	iter != m_tables.end ())
      return iter->get ();

    return nullptr;
  }

  /* Add TABLE to this cache.  Ownership of TABLE is transferred to
     the cache.  Note that a table at a given section+offset may only
     be registered once -- a violation of this will cause an assert.
     To avoid this, call the 'find' method first, to see if the table
     has already been read.  */
  void add (abbrev_table_up table);

private:
  /* Key used to search for an existing abbrev table in M_TABLES.  */
  struct abbrev_table_search_key
  {
    const dwarf2_section_info *section;
    sect_offset sect_off;
  };

  struct abbrev_table_hash
  {
    using is_transparent = void;

    std::size_t operator() (const abbrev_table_search_key &key) const noexcept
    {
      return (std::hash<const dwarf2_section_info *> () (key.section)
	      + (std::hash<std::underlying_type_t<decltype (key.sect_off)>> ()
		 (to_underlying (key.sect_off))));
    }

    std::size_t operator() (const abbrev_table_up &table) const noexcept
    { return (*this) ({ table->section, table->sect_off }); }
  };

  struct abbrev_table_eq
  {
    using is_transparent = void;

    bool operator() (const abbrev_table_search_key &key,
		     const abbrev_table_up &table) const noexcept
    { return key.section == table->section && key.sect_off == table->sect_off; }

    bool operator() (const abbrev_table_up &lhs,
		     const abbrev_table_up &rhs) const noexcept
    { return (*this) ({ lhs->section, lhs->sect_off }, rhs); }
  };

  /* Hash table of abbrev tables.  */
  gdb::unordered_set<abbrev_table_up, abbrev_table_hash, abbrev_table_eq> m_tables;
};

#endif /* GDB_DWARF2_ABBREV_TABLE_CACHE_H */
