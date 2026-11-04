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

#include "dwarf2/cooked-indexer.h"
#include "dwarf2/cooked-index-worker.h"
#include "dwarf2/error.h"

/* See cooked-indexer.h.  */

cooked_indexer::cooked_indexer (cooked_index_worker_result *storage,
				dwarf2_per_cu *per_cu, enum language language)
  : m_index_storage (storage),
    m_per_cu (per_cu),
    m_language (language),
    m_die_range_map (storage->get_parent_map ())
{
}

/* See cooked-indexer.h.  */

void
cooked_indexer::check_bounds (cutu_reader *reader)
{
  dwarf2_cu *cu = reader->cu ();

  if (cu->per_cu->addresses_seen)
    return;

  unrelocated_addr best_lowpc = {}, best_highpc = {};
  /* Possibly set the default values of LOWPC and HIGHPC from
     `DW_AT_ranges'.  */
  dwarf2_find_base_address (reader->top_level_die (), cu);
  enum pc_bounds_kind cu_bounds_kind
    = dwarf2_get_pc_bounds (reader->top_level_die (), &best_lowpc, &best_highpc,
			    cu, m_index_storage->get_addrmap (), cu->per_cu);
  if (cu_bounds_kind == PC_BOUNDS_HIGH_LOW && best_lowpc < best_highpc)
    {
      /* Store the contiguous range if it is not empty; it can be
	 empty for CUs with no code.  addrmap requires CORE_ADDR, so
	 we cast here.  */
      m_index_storage->get_addrmap ()->set_empty ((CORE_ADDR) best_lowpc,
						  (CORE_ADDR) best_highpc - 1,
						  cu->per_cu);

      cu->per_cu->addresses_seen = true;
    }
}

/* Helper function that returns true if TAG can have a linkage
   name.  */

static bool
tag_can_have_linkage_name (enum dwarf_tag tag)
{
  switch (tag)
    {
    case DW_TAG_variable:
    case DW_TAG_subprogram:
      return true;

    default:
      return false;
    }
}

/* See cooked-indexer.h.  */

cutu_reader *
cooked_indexer::ensure_cu_exists (cutu_reader *reader,
				  const section_and_offset &sect_off,
				  bool for_scanning)
{
  /* Lookups for type unit references are always in the CU, and
     cross-CU references will crash.  */
  if (reader->section () == sect_off.section
      && reader->cu ()->header.offset_in_unit_p (sect_off.offset))
    return reader;

  dwarf2_per_objfile *per_objfile = reader->cu ()->per_objfile;
  dwarf2_per_cu *per_cu = dwarf2_find_containing_unit (sect_off, per_objfile);

  /* When scanning, we only want to visit a given CU a single time.
     Doing this check here avoids self-imports as well.  */
  if (for_scanning)
    {
      bool nope = false;
      if (!per_cu->scanned.compare_exchange_strong (nope, true))
	return nullptr;
    }

  cutu_reader *result = m_index_storage->get_reader (per_cu);
  if (result == nullptr)
    {
      const abbrev_table_cache &abbrev_table_cache
	= m_index_storage->get_abbrev_table_cache ();
      auto new_reader
	= std::make_unique<cutu_reader> (*per_cu, *per_objfile, nullptr,
					 nullptr, false, language_minimal,
					 &abbrev_table_cache);

      if (new_reader->is_dummy ())
	return nullptr;

      result = m_index_storage->preserve (std::move (new_reader));
    }

  if (result->is_dummy ())
    return nullptr;

  if (for_scanning)
    check_bounds (result);

  return result;
}

/* See cooked-indexer.h.  */

const gdb_byte *
cooked_indexer::scan_attributes (dwarf2_per_cu *scanning_per_cu,
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
				 bool for_specification)
{
  bool is_declaration = false;
  std::optional<section_and_offset> origin;
  std::optional<unrelocated_addr> low_pc;
  std::optional<unrelocated_addr> high_pc;
  bool high_pc_relative = false;

  for (int i = 0; i < abbrev->num_attrs; ++i)
    {
      attribute attr;
      info_ptr = reader->read_attribute (&attr, &abbrev->attrs[i], info_ptr);

      /* Store the data if it is of an attribute we want to keep in a
	 partial symbol table.  */
      switch (attr.name)
	{
	case DW_AT_name:
	  switch (abbrev->tag)
	    {
	    case DW_TAG_compile_unit:
	    case DW_TAG_partial_unit:
	    case DW_TAG_type_unit:
	      /* Compilation units have a DW_AT_name that is a filename, not
		 a source language identifier.  */
	      break;

	    default:
	      if (*name == nullptr)
		*name = attr.as_string ();
	      break;
	    }
	  break;

	case DW_AT_linkage_name:
	case DW_AT_MIPS_linkage_name:
	  /* Note that both forms of linkage name might appear.  We
	     assume they will be the same, and we only store the last
	     one we see.  */
	  if (*linkage_name == nullptr)
	    *linkage_name = attr.as_string ();
	  break;

	/* DWARF 4 has defined a dedicated DW_AT_main_subprogram
	   attribute to indicate the starting function of the program...  */
	case DW_AT_main_subprogram:
	  if (attr.as_boolean ())
	    *flags |= IS_MAIN;
	  break;

	/* ... however with older versions the DW_CC_program value of
	   the DW_AT_calling_convention attribute was used instead as
	   the only means available.  We handle both variants then.  */
	case DW_AT_calling_convention:
	  {
	    std::optional<ULONGEST> value = attr.unsigned_constant ();
	    if (value.has_value () && *value == DW_CC_program)
	      *flags |= IS_MAIN;
	  }
	  break;

	case DW_AT_declaration:
	  is_declaration = attr.as_boolean ();
	  break;

	case DW_AT_sibling:
	  if (sibling_offset != nullptr)
	    *sibling_offset = attr.get_ref_die_offset ();
	  break;

	case DW_AT_specification:
	case DW_AT_abstract_origin:
	case DW_AT_extension:
	  origin = { &get_section_for_ref (attr, reader->cu ()),
		     attr.get_ref_die_offset () };
	  break;

	case DW_AT_external:
	  if (attr.as_boolean ())
	    *flags &= ~IS_STATIC;
	  break;

	case DW_AT_enum_class:
	  if (attr.as_boolean ())
	    *is_enum_class = true;
	  break;

	case DW_AT_low_pc:
	  low_pc = attr.as_address ();
	  break;

	case DW_AT_high_pc:
	  high_pc = attr.as_address ();
	  if (reader->cu ()->header.version >= 4 && attr.form_is_constant ())
	    high_pc_relative = true;
	  break;

	case DW_AT_location:
	  if (!scanning_per_cu->addresses_seen && attr.form_is_block ())
	    {
	      struct dwarf_block *locdesc = attr.as_block ();
	      CORE_ADDR addr;
	      dwarf2_cu *cu = reader->cu ();

	      if (decode_locdesc (locdesc, cu, &addr)
		  && (addr != 0
		      || cu->per_objfile->per_bfd->has_section_at_zero))
		{
		  low_pc = (unrelocated_addr) addr;
		  /* For variables, we don't want to try decoding the
		     type just to find the size -- for gdb's purposes
		     we only need the address of a variable.  */
		  high_pc = (unrelocated_addr) (addr + 1);
		  high_pc_relative = false;
		}
	    }
	  break;

	case DW_AT_ranges:
	  if (!scanning_per_cu->addresses_seen)
	    {
	      /* Offset in the .debug_ranges or .debug_rnglist section
		 (depending on DWARF version).  */
	      ULONGEST ranges_offset = attr.as_unsigned ();

	      /* See dwarf2_cu::gnu_ranges_base's doc for why we might
		 want to add this value.  */
	      ranges_offset += reader->cu ()->gnu_ranges_base;

	      unrelocated_addr lowpc, highpc;
	      dwarf2_ranges_read (ranges_offset, &lowpc, &highpc, reader->cu (),
				  m_index_storage->get_addrmap (),
				  scanning_per_cu, abbrev->tag);
	    }
	  break;
	}
    }

  /* We don't want to examine declarations, but if we found a
     declaration when handling DW_AT_specification or the like, then
     that is ok.  Similarly, we allow an external variable without a
     location; those are resolved via minimal symbols.  */
  if (is_declaration && !for_specification
      && !(abbrev->tag == DW_TAG_variable && (*flags & IS_STATIC) == 0))
    {
      /* We always want to recurse into some types, but we may not
	 want to treat them as definitions.  */
      if ((abbrev->tag == DW_TAG_class_type
	   || abbrev->tag == DW_TAG_structure_type
	   || abbrev->tag == DW_TAG_union_type
	   || abbrev->tag == DW_TAG_namespace)
	  && abbrev->has_children)
	*flags |= IS_TYPE_DECLARATION;
      else
	{
	  *linkage_name = nullptr;
	  *name = nullptr;
	}
    }
  else if ((*name == nullptr
	    || (*linkage_name == nullptr
		&& tag_can_have_linkage_name (abbrev->tag))
	    || (*parent_entry == nullptr && m_language != language_c))
	   && origin.has_value ())
    {
      cutu_reader *new_reader
	= ensure_cu_exists (reader, *origin, false);
      if (new_reader == nullptr)
	error (_(DWARF_ERROR_PREFIX
		 "cannot follow reference to DIE at %s"
		 " [in module %s]"),
	       sect_offset_str (origin->offset),
	       bfd_get_filename (reader->abfd ()));

      const gdb_byte *new_info_ptr
	= (new_reader->buffer () + to_underlying (origin->offset));

      if (*parent_entry == nullptr)
	{
	  /* We only perform immediate lookups of parents for DIEs
	     from earlier in this CU.  This avoids any problem
	     with a NULL result when when we see a reference to a
	     DIE in another CU that we may or may not have
	     imported locally.  */
	  parent_map::addr_type addr = parent_map::form_addr (new_info_ptr);
	  if (new_reader->cu () != reader->cu ()
	      || new_info_ptr > watermark_ptr)
	    *maybe_defer = addr;
	  else
	    *parent_entry = m_die_range_map->find (addr);
	}

      unsigned int bytes_read;
      const abbrev_info *new_abbrev
	= new_reader->peek_die_abbrev (new_info_ptr, &bytes_read);

      if (new_abbrev == nullptr)
	error (_(DWARF_ERROR_PREFIX
		 "Unexpected null DIE at offset %s [in module %s]"),
	       sect_offset_str (origin->offset),
	       bfd_get_filename (new_reader->abfd ()));

      new_info_ptr += bytes_read;

      if (new_reader->cu () == reader->cu () && new_info_ptr == watermark_ptr)
	{
	  /* Self-reference, we're done.  */
	}
      else
	scan_attributes (scanning_per_cu, new_reader, new_info_ptr,
			 new_info_ptr, new_abbrev, name, linkage_name,
			 flags, nullptr, parent_entry, maybe_defer,
			 is_enum_class, true);
    }

  if (!for_specification)
    {
      /* Older versions of GNAT emit full-qualified encoded names.  In
	 this case, also use this name as the linkage name.  */
      if (m_language == language_ada
	  && *linkage_name == nullptr
	  && *name != nullptr
	  && strstr (*name, "__") != nullptr)
	*linkage_name = *name;

      if (!scanning_per_cu->addresses_seen && low_pc.has_value ()
	  && (reader->cu ()->per_objfile->per_bfd->has_section_at_zero
	      || *low_pc != (unrelocated_addr) 0)
	  && high_pc.has_value ())
	{
	  if (high_pc_relative)
	    high_pc = (unrelocated_addr) ((ULONGEST) *high_pc
					  + (ULONGEST) *low_pc);

	  if (*high_pc > *low_pc)
	    {
	      /* Need CORE_ADDR casts for addrmap.  */
	      m_index_storage->get_addrmap ()->set_empty
		((CORE_ADDR) *low_pc, (CORE_ADDR) *high_pc - 1,
		 scanning_per_cu);
	    }
	}

      if (abbrev->tag == DW_TAG_namespace && *name == nullptr)
	*name = "(anonymous namespace)";

      /* Keep in sync with new_symbol.  */
      if (abbrev->tag == DW_TAG_subprogram
	  && (m_language == language_ada
	      || m_language == language_fortran))
	*flags &= ~IS_STATIC;
    }

  return info_ptr;
}

/* See cooked-indexer.h.  */

const gdb_byte *
cooked_indexer::index_imported_unit (cutu_reader *reader,
				     const gdb_byte *info_ptr,
				     const abbrev_info *abbrev)
{
  std::optional<section_and_offset> target;

  for (int i = 0; i < abbrev->num_attrs; ++i)
    {
      /* Note that we never need to reprocess attributes here.  */
      attribute attr;
      info_ptr = reader->read_attribute (&attr, &abbrev->attrs[i], info_ptr);

      if (attr.name == DW_AT_import)
	target = { &get_section_for_ref (attr, reader->cu ()),
		   attr.get_ref_die_offset () };
    }

  /* Did not find DW_AT_import.  */
  if (!target.has_value ())
    return info_ptr;

  cutu_reader *new_reader
    = ensure_cu_exists (reader, *target, true);
  if (new_reader != nullptr)
    {
      index_dies (new_reader, new_reader->info_ptr (), nullptr, false);

      reader->cu ()->add_dependence (new_reader->cu ()->per_cu);
    }

  return info_ptr;
}

/* See cooked-indexer.h.  */

const gdb_byte *
cooked_indexer::recurse (cutu_reader *reader,
			 const gdb_byte *info_ptr,
			 std::variant<const cooked_index_entry *,
				      parent_map::addr_type> parent,
			 bool fully)
{
  info_ptr = index_dies (reader, info_ptr, parent, fully);

  if (!std::holds_alternative<const cooked_index_entry *> (parent))
    return info_ptr;
  const cooked_index_entry *parent_entry
    = std::get<const cooked_index_entry *> (parent);

  if (parent_entry != nullptr)
    {
      /* Both start and end are inclusive, so use both "+ 1" and "- 1" to
	 limit the range to the children of parent_entry.  */
      parent_map::addr_type start
	= parent_map::form_addr (reader->buffer ()
				 + to_underlying (parent_entry->die_offset)
				 + 1);
      parent_map::addr_type end = parent_map::form_addr (info_ptr - 1);
      m_die_range_map->add_entry (start, end, parent_entry);
    }

  return info_ptr;
}

/* See cooked-indexer.h.  */

const gdb_byte *
cooked_indexer::index_dies (cutu_reader *reader,
			    const gdb_byte *info_ptr,
			    std::variant<const cooked_index_entry *,
					 parent_map::addr_type> parent,
			    bool fully)
{
  const gdb_byte *end_ptr
    = (reader->buffer () + to_underlying (reader->cu ()->header.sect_off)
       + reader->cu ()->header.get_length_with_initial ());

  while (info_ptr < end_ptr)
    {
      sect_offset this_die = (sect_offset) (info_ptr - reader->buffer ());
      unsigned int bytes_read;
      const abbrev_info *abbrev
	= reader->peek_die_abbrev (info_ptr, &bytes_read);
      info_ptr += bytes_read;
      if (abbrev == nullptr)
	break;

      if (abbrev->tag == DW_TAG_imported_unit)
	{
	  info_ptr = index_imported_unit (reader, info_ptr, abbrev);
	  continue;
	}

      parent_map::addr_type defer {};
      if (std::holds_alternative<parent_map::addr_type> (parent))
	defer = std::get<parent_map::addr_type> (parent);
      const cooked_index_entry *parent_entry = nullptr;
      if (std::holds_alternative<const cooked_index_entry *> (parent))
	parent_entry = std::get<const cooked_index_entry *> (parent);

      /* If a DIE parent is a DW_TAG_subprogram, then the DIE is only
	 interesting if it's a DW_TAG_subprogram or a DW_TAG_entry_point.  */
      bool die_interesting
	= (abbrev->interesting
	   && (parent_entry == nullptr
	       || parent_entry->tag != DW_TAG_subprogram
	       || abbrev->tag == DW_TAG_subprogram
	       || abbrev->tag == DW_TAG_entry_point));

      if (!die_interesting)
	{
	  info_ptr = reader->skip_one_die (info_ptr, abbrev, !fully);
	  if (fully && abbrev->has_children)
	    info_ptr = index_dies (reader, info_ptr, parent, fully);
	  continue;
	}

      const char *name = nullptr;
      const char *linkage_name = nullptr;
      cooked_index_flag flags = IS_STATIC;
      sect_offset sibling {};
      const cooked_index_entry *this_parent_entry = parent_entry;
      bool is_enum_class = false;

      /* The scope of a DW_TAG_entry_point cooked_index_entry is the one of
	 its surrounding subroutine.  */
      if (abbrev->tag == DW_TAG_entry_point)
	this_parent_entry = parent_entry->get_parent ();
      info_ptr
	= scan_attributes (reader->cu ()->per_cu, reader, info_ptr, info_ptr,
			   abbrev, &name, &linkage_name, &flags, &sibling,
			   &this_parent_entry, &defer, &is_enum_class, false);
      /* A DW_TAG_entry_point inherits its static/extern property from
	 the enclosing subroutine.  */
      if (abbrev->tag == DW_TAG_entry_point)
	{
	  flags &= ~IS_STATIC;
	  flags |= parent_entry->flags & IS_STATIC;
	}

      if (abbrev->tag == DW_TAG_namespace
	  && m_language == language_cplus
	  && strcmp (name, "::") == 0)
	{
	  /* GCC 4.0 and 4.1 had a bug (PR c++/28460) where they
	     generated bogus DW_TAG_namespace DIEs with a name of "::"
	     for the global namespace.  Work around this problem
	     here.  */
	  name = nullptr;
	}

      cooked_index_entry *this_entry = nullptr;
      if (name != nullptr)
	{
	  if (defer != 0)
	    this_entry
	      = m_index_storage->add (this_die, abbrev->tag,
				      flags | IS_PARENT_DEFERRED, name,
				      defer, m_per_cu);
	  else
	    this_entry
	      = m_index_storage->add (this_die, abbrev->tag, flags, name,
				      this_parent_entry, m_per_cu);
	}

      if (linkage_name != nullptr)
	{
	  /* We only want this to be "main" if it has a linkage name
	     but not an ordinary name.  */
	  if (name != nullptr)
	    flags = flags & ~IS_MAIN;
	  /* Set the IS_LINKAGE on for everything except when functions
	     have linkage name present but name is absent.  */
	  if (name != nullptr
	      || (abbrev->tag != DW_TAG_subprogram
		  && abbrev->tag != DW_TAG_inlined_subroutine
		  && abbrev->tag != DW_TAG_entry_point))
	    flags = flags | IS_LINKAGE;
	  m_index_storage->add (this_die, abbrev->tag, flags,
				linkage_name, nullptr, m_per_cu);
	}

      if (abbrev->has_children)
	{
	  switch (abbrev->tag)
	    {
	    case DW_TAG_class_type:
	    case DW_TAG_interface_type:
	    case DW_TAG_structure_type:
	    case DW_TAG_union_type:
	      if (m_language != language_c && this_entry != nullptr)
		{
		  info_ptr = recurse (reader, info_ptr, this_entry, fully);
		  continue;
		}
	      break;

	    case DW_TAG_enumeration_type:
	      /* Some versions of gdc could emit an "enum class"
		 without a name, which is nonsensical.  These are
		 skipped.  */
	      if (is_enum_class && this_entry == nullptr)
		continue;

	      /* We need to recurse even for an anonymous enumeration.
		 Which scope we record as the parent scope depends on
		 whether we're reading an "enum class".  If so, we use
		 the enum itself as the parent, yielding names like
		 "enum_class::enumerator"; otherwise we inject the
		 names into our own parent scope.  */
	      {
		std::variant<const cooked_index_entry *,
			     parent_map::addr_type> recurse_parent;
		if (is_enum_class)
		  {
		    gdb_assert (this_entry != nullptr);
		    recurse_parent = this_entry;
		  }
		else if (defer != 0)
		  recurse_parent = defer;
		else
		  recurse_parent = this_parent_entry;

		info_ptr = recurse (reader, info_ptr, recurse_parent, fully);
	      }
	      continue;

	    case DW_TAG_module:
	      if (this_entry == nullptr)
		break;
	      [[fallthrough]];
	    case DW_TAG_namespace:
	      /* We don't check THIS_ENTRY for a namespace, to handle
		 the ancient G++ workaround pointed out above.  */
	      info_ptr = recurse (reader, info_ptr, this_entry, fully);
	      continue;

	    case DW_TAG_subprogram:
	      if ((m_language == language_fortran
		   || m_language == language_ada)
		  && this_entry != nullptr)
		{
		  info_ptr = recurse (reader, info_ptr, this_entry, true);
		  continue;
		}
	      break;
	    }

	  if (sibling != sect_offset (0))
	    {
	      const gdb_byte *sibling_ptr
		= reader->buffer () + to_underlying (sibling);

	      if (sibling_ptr < info_ptr)
		complaint (_("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > reader->buffer_end ())
		reader->section ()->overflow_complaint ();
	      else
		info_ptr = sibling_ptr;
	    }
	  else
	    info_ptr = reader->skip_children (info_ptr);
	}
    }

  return info_ptr;
}

/* See cooked-indexer.h.  */

void
cooked_indexer::make_index (cutu_reader *reader)
{
  check_bounds (reader);
  find_file_and_directory (reader->top_level_die (), reader->cu ());

  if (!reader->top_level_die ()->has_children)
    return;

  index_dies (reader, reader->info_ptr (), nullptr, false);
}
