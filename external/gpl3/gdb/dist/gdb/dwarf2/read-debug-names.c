/* Reading code for .debug_names

   Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#include "read-debug-names.h"
#include "dwarf2/aranges.h"
#include "dwarf2/cooked-index.h"

#include "complaints.h"
#include "cp-support.h"
#include "dwz.h"
#include "mapped-index.h"
#include "read.h"
#include "stringify.h"
#include "extract-store-integer.h"
#include "gdbsupport/thread-pool.h"

/* This is just like cooked_index_functions, but overrides a single
   method so the test suite can distinguish the .debug_names case from
   the ordinary case.  */
struct dwarf2_debug_names_index : public cooked_index_functions
{
  /* This dumps minimal information about .debug_names.  It is called
     via "mt print objfiles".  The gdb.dwarf2/gdb-index.exp testcase
     uses this to verify that .debug_names has been loaded.  */
  void dump (struct objfile *objfile) override
  {
    gdb_printf (".debug_names: exists\n");
    cooked_index_functions::dump (objfile);
  }
};

/* This is like a cooked index, but as it has been ingested from
   .debug_names, it can't be used to write out an index.  */
class debug_names_index : public cooked_index
{
public:

  using cooked_index::cooked_index;

  cooked_index *index_for_writing () override
  { return nullptr; }

  quick_symbol_functions_up make_quick_functions () const override
  { return quick_symbol_functions_up (new dwarf2_debug_names_index); }
};

/* A description of the mapped .debug_names.  */

struct mapped_debug_names_reader
{
  const gdb_byte *scan_one_entry (const char *name,
				  const gdb_byte *entry,
				  cooked_index_entry **result,
				  std::optional<ULONGEST> &parent);
  void scan_entries (uint32_t index, const char *name, const gdb_byte *entry);
  void scan_all_names ();

  dwarf2_per_objfile *per_objfile = nullptr;
  bfd *abfd = nullptr;
  bfd_endian dwarf5_byte_order {};
  bool dwarf5_is_dwarf64 = false;

  /* True if the augmentation string indicates the index was produced by
     GDB.  */
  bool augmentation_is_gdb = false;

  /* If AUGMENTATION_IS_GDB is true, this indicates the version.  Otherwise,
     this value is meaningless.  */
  unsigned int gdb_augmentation_version = 0;

  uint8_t offset_size = 0;
  uint32_t cu_count = 0;
  uint32_t tu_count = 0, bucket_count = 0, name_count = 0;
  const gdb_byte *cu_table_reordered = nullptr;
  const gdb_byte *tu_table_reordered = nullptr;
  const uint32_t *bucket_table_reordered = nullptr;
  const uint32_t *hash_table_reordered = nullptr;
  const gdb_byte *name_table_string_offs_reordered = nullptr;
  const gdb_byte *name_table_entry_offs_reordered = nullptr;
  const gdb_byte *entry_pool = nullptr;

  struct index_val
  {
    ULONGEST dwarf_tag;
    struct attr
    {
      /* Attribute name DW_IDX_*.  */
      ULONGEST dw_idx;

      /* Attribute form DW_FORM_*.  */
      ULONGEST form;

      /* Value if FORM is DW_FORM_implicit_const.  */
      LONGEST implicit_const;
    };
    std::vector<attr> attr_vec;
  };

  gdb::unordered_map<ULONGEST, index_val> abbrev_map;

  /* List of CUs in the same order as found in the index header (DWARF 5 section
     6.1.1.4.2).  */
  std::vector<dwarf2_per_cu *> comp_units;

  /* List of local TUs in the same order as found in the index (DWARF 5 section
     6.1.1.4.3).  */
  std::vector<dwarf2_per_cu *> type_units;

  /* Even though the scanning of .debug_names and creation of the
     cooked index entries is done serially, we create multiple shards
     so that the finalization step can be parallelized.  The shards
     are filled in a round robin fashion.  It's convenient to use a
     result object rather than an actual shard.  */
  std::vector<cooked_index_worker_result> indices;

  /* Next shard to insert an entry in.  */
  int next_shard = 0;

  /* Maps entry pool offsets to cooked index entries.  */
  gdb::unordered_map<ULONGEST, cooked_index_entry *>
    entry_pool_offsets_to_entries;

  /* Cooked index entries for which the parent needs to be resolved.

     The second value of the pair is the DW_IDX_parent value.  Its meaning
     depends on the augmentation string:

       - GDB2: an index in the name table
       - GDB3: an offset offset into the entry pool  */
  std::vector<std::pair<cooked_index_entry *, ULONGEST>> needs_parent;

  /* All the cooked index entries created, in the same order and groups as
     listed in the name table.

     The size of the outer vector is equal to the number of entries in the name
     table (NAME_COUNT).  */
  std::vector<std::vector<cooked_index_entry *>> all_entries;
};

/* Scan a single entry from the entries table.  Set *RESULT and PARENT
   (if needed) and return the updated pointer on success, or return
   nullptr on error, or at the end of the table.  */

const gdb_byte *
mapped_debug_names_reader::scan_one_entry (const char *name,
					   const gdb_byte *entry,
					   cooked_index_entry **result,
					   std::optional<ULONGEST> &parent)
{
  unsigned int bytes_read;
  const auto offset_in_entry_pool = entry - entry_pool;
  const ULONGEST abbrev = read_unsigned_leb128 (abfd, entry, &bytes_read);
  entry += bytes_read;
  if (abbrev == 0)
    return nullptr;

  const auto indexval_it = abbrev_map.find (abbrev);
  if (indexval_it == abbrev_map.cend ())
    {
      complaint (_("Wrong .debug_names undefined abbrev code %s "
		   "[in module %s]"),
		 pulongest (abbrev), bfd_get_filename (abfd));
      return nullptr;
    }

  const auto &indexval = indexval_it->second;
  cooked_index_flag flags = 0;
  sect_offset die_offset {};
  enum language lang = language_unknown;
  dwarf2_per_cu *per_cu = nullptr;
  for (const auto &attr : indexval.attr_vec)
    {
      ULONGEST ull;
      switch (attr.form)
	{
	case DW_FORM_implicit_const:
	  ull = attr.implicit_const;
	  break;
	case DW_FORM_flag_present:
	  ull = 1;
	  break;
	case DW_FORM_udata:
	  ull = read_unsigned_leb128 (abfd, entry, &bytes_read);
	  entry += bytes_read;
	  break;
	case DW_FORM_ref_addr:
	  ull = read_offset (abfd, entry, offset_size);
	  entry += offset_size;
	  break;
	case DW_FORM_data1:
	  ull = *entry++;
	  break;
	case DW_FORM_data2:
	  ull = read_2_bytes (abfd, entry);
	  entry += 2;
	  break;
	case DW_FORM_data4:
	  ull = read_4_bytes (abfd, entry);
	  entry += 4;
	  break;
	case DW_FORM_ref4:
	  ull = read_4_bytes (abfd, entry);
	  entry += 4;
	  break;
	case DW_FORM_ref8:
	  ull = read_8_bytes (abfd, entry);
	  entry += 8;
	  break;
	case DW_FORM_ref_sig8:
	  ull = read_8_bytes (abfd, entry);
	  entry += 8;
	  break;
	default:
	  /* A warning instead of a complaint, because this one is
	     more like a bug in gdb.  */
	  warning (_("Unsupported .debug_names form %s [in module %s].\n"
		     "This normally should not happen, please file a bug report."),
		   dwarf_form_name (attr.form),
		   bfd_get_filename (abfd));
	  return nullptr;
	}
      switch (attr.dw_idx)
	{
	case DW_IDX_compile_unit:
	  {
	    /* Don't crash on bad data.  */
	    if (ull >= this->comp_units.size ())
	      {
		complaint (_(".debug_names entry has bad CU index %s"
			     " [in module %s]"),
			   pulongest (ull),
			   bfd_get_filename (abfd));
		continue;
	      }

	    per_cu = this->comp_units[ull];
	    break;
	  }
	case DW_IDX_type_unit:
	  {
	    /* Don't crash on bad data.  */
	    if (ull >= this->type_units.size ())
	      {
		complaint (_(".debug_names entry has bad TU index %s"
			     " [in module %s]"),
			   pulongest (ull),
			   bfd_get_filename (abfd));
		continue;
	      }

	    per_cu = this->type_units[ull];
	    break;
	  }
	case DW_IDX_die_offset:
	  die_offset = sect_offset (ull);
	  /* In a per-CU index (as opposed to a per-module index), index
	     entries without CU attribute implicitly refer to the single CU.  */
	  if (per_cu == nullptr)
	    per_cu = this->comp_units[0];
	  break;
	case DW_IDX_parent:
	  parent = ull;
	  break;
	case DW_IDX_GNU_internal:
	  if (augmentation_is_gdb && ull != 0)
	    flags |= IS_STATIC;
	  break;
	case DW_IDX_GNU_main:
	  if (augmentation_is_gdb && ull != 0)
	    flags |= IS_MAIN;
	  break;
	case DW_IDX_GNU_language:
	  if (augmentation_is_gdb)
	    lang = dwarf_lang_to_enum_language (ull);
	  break;
	case DW_IDX_GNU_linkage_name:
	  if (augmentation_is_gdb && ull != 0)
	    flags |= IS_LINKAGE;
	  break;
	}
    }

  /* Skip if we couldn't find a valid CU/TU index.  */
  if (per_cu != nullptr)
    {
      *result
	= indices[next_shard].add (die_offset, (dwarf_tag) indexval.dwarf_tag,
				   flags, lang, name, nullptr, per_cu);

      ++next_shard;
      if (next_shard == indices.size ())
	next_shard = 0;

      entry_pool_offsets_to_entries.emplace (offset_in_entry_pool, *result);
    }

  return entry;
}

/* Scan all the entries for NAME, at name slot INDEX.  */

void
mapped_debug_names_reader::scan_entries (uint32_t index,
					 const char *name,
					 const gdb_byte *entry)
{
  std::vector<cooked_index_entry *> these_entries;
  
  while (true)
    {
      std::optional<ULONGEST> parent;
      cooked_index_entry *this_entry;
      entry = scan_one_entry (name, entry, &this_entry, parent);

      if (entry == nullptr)
	break;

      these_entries.push_back (this_entry);
      if (parent.has_value ())
	needs_parent.emplace_back (this_entry, *parent);
    }

  all_entries[index] = std::move (these_entries);
}

/* Scan the name table and create all the entries.  */

void
mapped_debug_names_reader::scan_all_names ()
{
  all_entries.resize (name_count);

  /* In the first pass, create all the entries.  */
  for (uint32_t i = 0; i < name_count; ++i)
    {
      const ULONGEST namei_string_offs
	= extract_unsigned_integer ((name_table_string_offs_reordered
				     + i * offset_size),
				    offset_size, dwarf5_byte_order);
      const char *name = read_indirect_string_at_offset (per_objfile,
							 namei_string_offs);

      const ULONGEST namei_entry_offs
	= extract_unsigned_integer ((name_table_entry_offs_reordered
				     + i * offset_size),
				    offset_size, dwarf5_byte_order);
      const gdb_byte *entry = entry_pool + namei_entry_offs;

      scan_entries (i, name, entry);
    }

  /* Resolve the parent pointers for all entries that have a parent.

     If the augmentation string is "GDB2", the DW_IDX_parent value is an index
     into the name table.  Since there may be multiple index entries associated
     to that name, we have a little heuristic to figure out which is the right
     one.

     Otherwise, the DW_IDX_parent value is an offset into the entry pool, which
     is not ambiguous.  */
  for (auto &[entry, parent_val] : needs_parent)
    {
      if (augmentation_is_gdb && gdb_augmentation_version == 2)
	{
	  /* Name entries are indexed from 1 in DWARF.  */
	  std::vector<cooked_index_entry *> &entries
	    = all_entries[parent_val - 1];

	  for (const auto &parent : entries)
	    if (parent->lang == entry->lang)
	      {
		entry->set_parent (parent);
		break;
	      }
	}
      else
	{
	  const auto parent_it
	    = entry_pool_offsets_to_entries.find (parent_val);

	  if (parent_it == entry_pool_offsets_to_entries.cend ())
	    {
	      complaint (_ ("Parent entry not found for .debug_names entry"));
	      continue;
	    }

	  entry->set_parent (parent_it->second);
	}
    }
}

/* A reader for .debug_names.  */

struct cooked_index_worker_debug_names : public cooked_index_worker
{
  cooked_index_worker_debug_names (dwarf2_per_objfile *per_objfile,
			    mapped_debug_names_reader &&map)
    : cooked_index_worker (per_objfile),
      m_map (std::move (map))
  { }

  void do_reading () override;

  mapped_debug_names_reader m_map;
};

void
cooked_index_worker_debug_names::do_reading ()
{
  complaint_interceptor complaint_handler;

  /* Arbitrarily put all exceptions into the first result.  */
  m_map.indices[0].catch_error ([&] ()
    {
      m_map.scan_all_names ();
    });

  bool first = true;
  for (auto &iter : m_map.indices)
    {
      if (first)
	{
	  iter.done_reading (complaint_handler.release ());
	  first = false;
	}
      else
	iter.done_reading ({});
    }

  m_results = std::move (m_map.indices);
  done_reading ();

  bfd_thread_cleanup ();
}

/* Build the list of TUs (mapped_debug_names_reader::type_units) from the index
   header and verify that it matches the list of TUs read from the DIEs in
   `.debug_info`.

   Return true if they match, false otherwise.  */

static bool
build_and_check_tu_list_from_debug_names (dwarf2_per_objfile *per_objfile,
					  mapped_debug_names_reader &map,
					  dwarf2_section_info *section)
{
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  section->read (objfile);

  for (uint32_t i = 0; i < map.tu_count; ++i)
    {
      /* Read one entry from the TU list.  */
      sect_offset sect_off
	= (sect_offset) (extract_unsigned_integer
			 (map.tu_table_reordered + i * map.offset_size,
			  map.offset_size,
			  map.dwarf5_byte_order));

      /* Find the matching dwarf2_per_cu.  */
      dwarf2_per_cu *per_cu = dwarf2_find_unit ({ section, sect_off },
						per_bfd);

      if (per_cu == nullptr || !per_cu->is_debug_types)
	{
	  warning (_("Section .debug_names has incorrect entry in TU table,"
		     " ignoring .debug_names."));
	  return false;
	}

      map.type_units.emplace_back (per_cu);
    }

  return true;
}

/* DWARF-5 debug_names reader.  */

/* A helper function that reads the .debug_names section in SECTION
   and fills in MAP.  FILENAME is the name of the file containing the
   section; it is used for error reporting.

   Returns true if all went well, false otherwise.  */

static bool
read_debug_names_from_section (dwarf2_per_objfile *per_objfile,
			       const char *filename,
			       struct dwarf2_section_info *section,
			       mapped_debug_names_reader &map)
{
  struct objfile *objfile = per_objfile->objfile;

  if (section->empty ())
    return false;

  /* Older elfutils strip versions could keep the section in the main
     executable while splitting it for the separate debug info file.  */
  if ((section->get_flags () & SEC_HAS_CONTENTS) == 0)
    return false;

  section->read (objfile);

  map.per_objfile = per_objfile;
  map.dwarf5_byte_order = gdbarch_byte_order (objfile->arch ());

  const gdb_byte *addr = section->buffer;

  bfd *abfd = section->get_bfd_owner ();
  map.abfd = abfd;

  unsigned int bytes_read;
  LONGEST length = read_initial_length (abfd, addr, &bytes_read);
  addr += bytes_read;

  map.dwarf5_is_dwarf64 = bytes_read != 4;
  map.offset_size = map.dwarf5_is_dwarf64 ? 8 : 4;
  if (bytes_read + length != section->size)
    {
      /* There may be multiple per-CU indices.  */
      warning (_("Section .debug_names in %s length %s does not match "
		 "section length %s, ignoring .debug_names."),
	       filename, plongest (bytes_read + length),
	       pulongest (section->size));
      return false;
    }

  /* The version number.  */
  uint16_t version = read_2_bytes (abfd, addr);
  addr += 2;
  if (version != 5)
    {
      warning (_("Section .debug_names in %s has unsupported version %d, "
		 "ignoring .debug_names."),
	       filename, version);
      return false;
    }

  /* Padding.  */
  uint16_t padding = read_2_bytes (abfd, addr);
  addr += 2;
  if (padding != 0)
    {
      warning (_("Section .debug_names in %s has unsupported padding %d, "
		 "ignoring .debug_names."),
	       filename, padding);
      return false;
    }

  /* comp_unit_count - The number of CUs in the CU list.  */
  map.cu_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* local_type_unit_count - The number of TUs in the local TU
     list.  */
  map.tu_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* foreign_type_unit_count - The number of TUs in the foreign TU
     list.  */
  uint32_t foreign_tu_count = read_4_bytes (abfd, addr);
  addr += 4;
  if (foreign_tu_count != 0)
    {
      warning (_("Section .debug_names in %s has unsupported %lu foreign TUs, "
		 "ignoring .debug_names."),
	       filename, static_cast<unsigned long> (foreign_tu_count));
      return false;
    }

  /* bucket_count - The number of hash buckets in the hash lookup
     table.  */
  map.bucket_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* name_count - The number of unique names in the index.  */
  map.name_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* abbrev_table_size - The size in bytes of the abbreviations
     table.  */
  uint32_t abbrev_table_size = read_4_bytes (abfd, addr);
  addr += 4;

  /* augmentation_string_size - The size in bytes of the augmentation
     string.  This value is rounded up to a multiple of 4.  */
  uint32_t augmentation_string_size = read_4_bytes (abfd, addr);
  addr += 4;
  augmentation_string_size += (-augmentation_string_size) & 3;

  const auto augmentation_string
    = gdb::make_array_view (addr, augmentation_string_size);

  if (augmentation_string == gdb::make_array_view (dwarf5_augmentation_1))
    {
      warning (_(".debug_names created by an old version of gdb; ignoring"));
      return false;
    }
  else if (augmentation_string == gdb::make_array_view (dwarf5_augmentation_2))
    {
      map.augmentation_is_gdb = true;
      map.gdb_augmentation_version = 2;
    }
  else if (augmentation_string == gdb::make_array_view (dwarf5_augmentation_3))
    {
      map.augmentation_is_gdb = true;
      map.gdb_augmentation_version = 3;
    }

  if (!map.augmentation_is_gdb)
    {
      warning (_(".debug_names not created by gdb; ignoring"));
      return false;
    }

  /* Skip past augmentation string.  */
  addr += augmentation_string_size;

  /* List of CUs */
  map.cu_table_reordered = addr;
  addr += map.cu_count * map.offset_size;

  /* List of Local TUs */
  map.tu_table_reordered = addr;
  addr += map.tu_count * map.offset_size;

  /* Hash Lookup Table */
  map.bucket_table_reordered = reinterpret_cast<const uint32_t *> (addr);
  addr += map.bucket_count * 4;
  map.hash_table_reordered = reinterpret_cast<const uint32_t *> (addr);
  if (map.bucket_count != 0)
    addr += map.name_count * 4;

  /* Name Table */
  map.name_table_string_offs_reordered = addr;
  addr += map.name_count * map.offset_size;
  map.name_table_entry_offs_reordered = addr;
  addr += map.name_count * map.offset_size;

  const gdb_byte *abbrev_table_start = addr;
  for (;;)
    {
      const ULONGEST index_num = read_unsigned_leb128 (abfd, addr, &bytes_read);
      addr += bytes_read;
      if (index_num == 0)
	break;

      const auto insertpair
	= map.abbrev_map.emplace (index_num, mapped_debug_names_reader::index_val ());
      if (!insertpair.second)
	{
	  warning (_("Section .debug_names in %s has duplicate index %s, "
		     "ignoring .debug_names."),
		   filename, pulongest (index_num));
	  return false;
	}
      mapped_debug_names_reader::index_val &indexval = insertpair.first->second;
      indexval.dwarf_tag = read_unsigned_leb128 (abfd, addr, &bytes_read);
      addr += bytes_read;

      for (;;)
	{
	  mapped_debug_names_reader::index_val::attr attr;
	  attr.dw_idx = read_unsigned_leb128 (abfd, addr, &bytes_read);
	  addr += bytes_read;
	  attr.form = read_unsigned_leb128 (abfd, addr, &bytes_read);
	  addr += bytes_read;
	  if (attr.form == DW_FORM_implicit_const)
	    {
	      attr.implicit_const = read_signed_leb128 (abfd, addr,
							&bytes_read);
	      addr += bytes_read;
	    }
	  if (attr.dw_idx == 0 && attr.form == 0)
	    break;
	  indexval.attr_vec.push_back (std::move (attr));
	}
    }
  if (addr != abbrev_table_start + abbrev_table_size)
    {
      warning (_("Section .debug_names in %s has abbreviation_table "
		 "of size %s vs. written as %u, ignoring .debug_names."),
	       filename, plongest (addr - abbrev_table_start),
	       abbrev_table_size);
      return false;
    }
  map.entry_pool = addr;

  return true;
}

/* A helper for check_cus_from_debug_names that handles the MAP's CU
   list.  */

static bool
build_and_check_cu_list_from_debug_names (dwarf2_per_bfd *per_bfd,
					  mapped_debug_names_reader &map,
					  dwarf2_section_info &section)
{
  int nr_cus = per_bfd->num_comp_units;

  if (map.cu_count != nr_cus)
    {
      warning (_("Section .debug_names has incorrect number of CUs in CU table,"
		 " ignoring .debug_names."));
      return false;
    }

  for (uint32_t i = 0; i < map.cu_count; ++i)
    {
      sect_offset sect_off
	= (sect_offset) (extract_unsigned_integer
			 (map.cu_table_reordered + i * map.offset_size,
			  map.offset_size,
			  map.dwarf5_byte_order));

      /* Find the matching dwarf2_per_cu.  */
      dwarf2_per_cu *per_cu = dwarf2_find_unit ({ &section, sect_off }, per_bfd);

      if (per_cu == nullptr || per_cu->is_debug_types)
	{
	  warning (_("Section .debug_names has incorrect entry in CU table,"
		     " ignoring .debug_names."));
	  return false;
	}

      map.comp_units.emplace_back (per_cu);
    }

  return true;
}

/* Build the list of CUs (mapped_debug_names_reader::compile_units) from the
   index header and verify that it matches the list of CUs read from the DIEs in
   `.debug_info`.

   Return true if they match, false otherwise.  */

static bool
build_and_check_cu_lists_from_debug_names (dwarf2_per_bfd *per_bfd,
					   mapped_debug_names_reader &map,
					   mapped_debug_names_reader &dwz_map)
{
  if (!build_and_check_cu_list_from_debug_names (per_bfd, map,
						 per_bfd->infos[0]))
    return false;

  if (dwz_map.cu_count == 0)
    return true;

  dwz_file *dwz = per_bfd->get_dwz_file ();
  return build_and_check_cu_list_from_debug_names (per_bfd, dwz_map, dwz->info);
}

/* See read-debug-names.h.  */

bool
dwarf2_read_debug_names (dwarf2_per_objfile *per_objfile)
{
  scoped_remove_all_units remove_all_units (*per_objfile->per_bfd);
  mapped_debug_names_reader map;
  mapped_debug_names_reader dwz_map;
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  if (!read_debug_names_from_section (per_objfile, objfile_name (objfile),
				      &per_bfd->debug_names, map))
    return false;

  /* Don't use the index if it's empty.  */
  if (map.name_count == 0)
    return false;

  /* If there is a .dwz file, read it so we can get its CU list as
     well.  */
  dwz_file *dwz = per_bfd->get_dwz_file ();
  if (dwz != NULL)
    {
      if (!read_debug_names_from_section (per_objfile,
					  bfd_get_filename (dwz->dwz_bfd.get ()),
					  &dwz->debug_names, dwz_map))
	{
	  warning (_("could not read '.debug_names' section from %s; skipping"),
		   bfd_get_filename (dwz->dwz_bfd.get ()));
	  return false;
	}
    }

  create_all_units (per_objfile);
  if (!build_and_check_cu_lists_from_debug_names (per_bfd, map, dwz_map))
    return false;

  if (map.tu_count != 0)
    {
      /* We can only handle a single .debug_info and .debug_types when we have
	 an index.  */
      if (per_bfd->infos.size () > 1
	  || per_bfd->types.size () > 1)
	return false;

      dwarf2_section_info *section
	= (per_bfd->types.size () == 1
	   ? &per_bfd->types[0]
	   : &per_bfd->infos[0]);

      if (!build_and_check_tu_list_from_debug_names (per_objfile, map,
						     section))
	return false;
    }

  per_bfd->debug_aranges.read (per_objfile->objfile);

  /* There is a single address map for the whole index (coming from
     .debug_aranges).  We only need to install it into a single shard
     for it to get searched by cooked_index.  So, we make the first
     result object here, so we can store the addrmap, then move it
     into place later.  */
  cooked_index_worker_result first;
  deferred_warnings warnings;
  read_addrmap_from_aranges (per_objfile, &per_bfd->debug_aranges,
			     first.get_addrmap (), &warnings);
  warnings.emit ();

  const auto n_workers
    = std::max<std::size_t> (gdb::thread_pool::g_thread_pool->thread_count (),
			     1);

  /* Create as many index shard as there are worker threads,
     preserving the first one.  */
  map.indices.push_back (std::move (first));
  map.indices.resize (n_workers);

  auto cidn = (std::make_unique<cooked_index_worker_debug_names>
	       (per_objfile, std::move (map)));
  auto idx = std::make_unique<debug_names_index> (std::move (cidn));
  per_bfd->start_reading (std::move (idx));
  remove_all_units.disable ();

  return true;
}
