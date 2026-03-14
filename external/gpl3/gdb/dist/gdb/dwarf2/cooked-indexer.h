/* DWARF indexer

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

#ifndef GDB_DWARF2_COOKED_INDEXER_H
#define GDB_DWARF2_COOKED_INDEXER_H

#include "dwarf2/cooked-index-entry.h"
#include "dwarf2/parent-map.h"
#include "dwarf2/types.h"
#include <variant>

struct abbrev_info;
struct cooked_index_worker_result;
struct cutu_reader;
struct dwarf2_per_cu;
struct dwarf2_per_objfile;
struct section_and_offset;

/* An instance of this is created to index a CU.  */

class cooked_indexer
{
public:
  cooked_indexer (cooked_index_worker_result *storage, dwarf2_per_cu *per_cu,
		  enum language language);

  DISABLE_COPY_AND_ASSIGN (cooked_indexer);

  /* Index the given CU.  */
  void make_index (cutu_reader *reader);

private:

  /* A helper function to scan the PC bounds of READER and record them
     in the storage's addrmap.  */
  void check_bounds (cutu_reader *reader);

  /* Ensure that the indicated CU exists.  The cutu_reader for it is
     returned.  FOR_SCANNING is true if the caller intends to scan all
     the DIEs in the CU; when false, this use is assumed to be to look
     up just a single DIE.  */
  cutu_reader *ensure_cu_exists (cutu_reader *reader,
				 const section_and_offset &sect_off,
				 bool for_scanning);

  /* Index DIEs in the READER starting at INFO_PTR.  PARENT is
     the entry for the enclosing scope (nullptr at top level).  FULLY
     is true when a full scan must be done -- in some languages,
     function scopes must be fully explored in order to find nested
     functions.  This returns a pointer to just after the spot where
     reading stopped.  */
  const gdb_byte *index_dies (cutu_reader *reader,
			      const gdb_byte *info_ptr,
			      std::variant<const cooked_index_entry *,
					   parent_map::addr_type> parent,
			      bool fully);

  /* Scan the attributes for a given DIE and update the out
     parameters.  Returns a pointer to the byte after the DIE.  */
  const gdb_byte *scan_attributes (dwarf2_per_cu *scanning_per_cu,
				   cutu_reader *reader,
				   const gdb_byte *watermark_ptr,
				   const gdb_byte *info_ptr,
				   const abbrev_info *abbrev,
				   const char **name,
				   const char **linkage_name,
				   cooked_index_flag *flags,
				   sect_offset *sibling_offset,
				   const cooked_index_entry **parent_entry,
				   parent_map::addr_type *maybe_defer,
				   bool *is_enum_class,
				   bool for_specification);

  /* Handle DW_TAG_imported_unit, by scanning the DIE to find
     DW_AT_import, and then scanning the referenced CU.  Returns a
     pointer to the byte after the DIE.  */
  const gdb_byte *index_imported_unit (cutu_reader *reader,
				       const gdb_byte *info_ptr,
				       const abbrev_info *abbrev);

  /* Recursively read DIEs, recording the section offsets in
     m_die_range_map and then calling index_dies.  */
  const gdb_byte *recurse (cutu_reader *reader,
			   const gdb_byte *info_ptr,
			   std::variant<const cooked_index_entry *,
					parent_map::addr_type> parent_entry,
			   bool fully);

  /* The storage object, where the results are kept.  */
  cooked_index_worker_result *m_index_storage;
  /* The CU that we are reading on behalf of.  This object might be
     asked to index one CU but to treat the results as if they come
     from some including CU; in this case the including CU would be
     recorded here.  */
  dwarf2_per_cu *m_per_cu;
  /* The language that we're assuming when reading.  */
  enum language m_language;

  /* Map from DIE ranges to newly-created entries.  */
  parent_map *m_die_range_map;
};

#endif /* GDB_DWARF2_COOKED_INDEXER_H */
