/* DIE parent maps

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

#include "dwarf2/cooked-index-entry.h"
#include "dwarf2/read.h"
#include "dwarf2/parent-map.h"

/* Dump MAP as parent_map.  */

static void
dump_parent_map (dwarf2_per_bfd *per_bfd, const struct addrmap *map)
{
  auto_obstack temp_storage;

  auto annotate_cooked_index_entry
    = [&] (struct ui_file *outfile, CORE_ADDR start_addr, const void *value)
	{
	  const cooked_index_entry *parent_entry
	    = (const cooked_index_entry *)value;

	  gdb_printf (outfile, "\n\t");

	  bool found = false;
	  for (auto sections : {per_bfd->infos, per_bfd->types})
	    for (auto section : sections)
	      if ((CORE_ADDR)section.buffer <= start_addr
		&& start_addr < (CORE_ADDR) (section.buffer + section.size))
	      {
		gdb_printf (outfile, "(section: %s, offset: 0x%" PRIx64 ")",
			    section.get_name (),
			    start_addr - (CORE_ADDR)section.buffer);
		found = true;
		break;
	      }

	  if (!found)
	    gdb_printf (outfile, "()");

	  if (parent_entry == nullptr)
	    {
	      gdb_printf (outfile, " -> ()");
	      return;
	    }

	  gdb_printf (outfile, " -> (0x%" PRIx64 ": %s)",
		      to_underlying (parent_entry->die_offset),
		      parent_entry->full_name (&temp_storage));
	};

  addrmap_dump (const_cast<addrmap *> (map), gdb_stdlog, nullptr,
		annotate_cooked_index_entry);
}

/* See parent-map.h.  */

void
parent_map::dump (dwarf2_per_bfd *per_bfd) const
{
  dump_parent_map (per_bfd, &m_map);
}

/* See parent-map.h.  */

void
parent_map_map::dump (dwarf2_per_bfd *per_bfd) const
{
  for (const auto &iter : m_maps)
    {
      gdb_printf (gdb_stdlog, "map start:\n");
      dump_parent_map (per_bfd, iter);
    }
}
