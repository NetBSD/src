/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2025 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

#include "dwarf2/unit-head.h"
#include "dwarf2/leb.h"
#include "dwarf2/section.h"
#include "dwarf2/stringify.h"
#include "dwarf2/error.h"

/* See unit-head.h.  */

const gdb_byte *
read_unit_head (struct unit_head *header, const gdb_byte *info_ptr,
		struct dwarf2_section_info *section, ruh_kind section_kind)
{
  int signed_addr;
  unsigned int bytes_read;
  const char *filename = section->get_file_name ();
  bfd *abfd = section->get_bfd_owner ();

  header->set_length (read_initial_length (abfd, info_ptr, &bytes_read));
  header->initial_length_size = bytes_read;
  header->offset_size = (bytes_read == 4) ? 4 : 8;
  info_ptr += bytes_read;
  unsigned version = read_2_bytes (abfd, info_ptr);
  if (version < 2 || version > 5)
    error (_(DWARF_ERROR_PREFIX
	     "wrong version in unit header "
	     "(is %d, should be 2, 3, 4 or 5) [in module %s]"),
	   version, filename);
  header->version = version;
  info_ptr += 2;
  if (header->version < 5)
    switch (section_kind)
      {
      case ruh_kind::COMPILE:
	header->unit_type = DW_UT_compile;
	break;
      case ruh_kind::TYPE:
	header->unit_type = DW_UT_type;
	break;
      default:
	internal_error (_("read_unit_head: invalid section_kind"));
      }
  else
    {
      header->unit_type
	= static_cast<enum dwarf_unit_type> (read_1_byte (abfd, info_ptr));
      info_ptr += 1;
      switch (header->unit_type)
	{
	case DW_UT_compile:
	case DW_UT_partial:
	case DW_UT_skeleton:
	case DW_UT_split_compile:
	  if (section_kind != ruh_kind::COMPILE)
	    error (_(DWARF_ERROR_PREFIX
		     "wrong unit_type in unit header "
		     "(is %s, should be %s) [in module %s]"),
		   dwarf_unit_type_name (header->unit_type),
		   dwarf_unit_type_name (DW_UT_type), filename);
	  break;
	case DW_UT_type:
	case DW_UT_split_type:
	  section_kind = ruh_kind::TYPE;
	  break;
	default:
	  error (_(DWARF_ERROR_PREFIX
		   "wrong unit_type in unit header "
		   "(is %#04x, should be one of: %s, %s, %s, %s or %s) "
		   "[in module %s]"),
		 header->unit_type, dwarf_unit_type_name (DW_UT_compile),
		 dwarf_unit_type_name (DW_UT_skeleton),
		 dwarf_unit_type_name (DW_UT_split_compile),
		 dwarf_unit_type_name (DW_UT_type),
		 dwarf_unit_type_name (DW_UT_split_type), filename);
	}

      header->addr_size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
    }
  header->abbrev_sect_off
    = (sect_offset) header->read_offset (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;
  if (header->version < 5)
    {
      header->addr_size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
    }
  signed_addr = bfd_get_sign_extend_vma (abfd);
  if (signed_addr < 0)
    internal_error (_("read_unit_head: dwarf from non elf file"));
  header->signed_addr_p = signed_addr;

  bool header_has_signature =
    (section_kind == ruh_kind::TYPE
     || header->unit_type == DW_UT_skeleton
     || header->unit_type == DW_UT_split_compile);

  if (header_has_signature)
    {
      header->signature = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;
    }

  if (section_kind == ruh_kind::TYPE)
    {
      LONGEST type_offset;
      type_offset = header->read_offset (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      header->type_offset_in_tu = (cu_offset) type_offset;
      if (to_underlying (header->type_offset_in_tu) != type_offset)
	error (_(DWARF_ERROR_PREFIX
		 "Too big type_offset in unit "
		 "header (is %s) [in module %s]"),
	       plongest (type_offset), filename);
    }

  return info_ptr;
}

/* Subroutine of read_and_check_unit_head to to simplify it.
   Perform various error checking on the header.  */

static void
error_check_unit_head (unit_head *header, dwarf2_section_info *section,
		       dwarf2_section_info *abbrev_section)
{
  const char *filename = section->get_file_name ();

  if (to_underlying (header->abbrev_sect_off) >= abbrev_section->size)
    error (_(DWARF_ERROR_PREFIX
	     "bad offset (%s) in unit header "
	     "(offset %s + 6) [in module %s]"),
	   sect_offset_str (header->abbrev_sect_off),
	   sect_offset_str (header->sect_off), filename);

  /* Cast to ULONGEST to use 64-bit arithmetic when possible to
     avoid potential 32-bit overflow.  */
  if (((ULONGEST) header->sect_off + header->get_length_with_initial ())
      > section->size)
    error (_(DWARF_ERROR_PREFIX
	     "bad length (0x%x) in unit header "
	     "(offset %s + 0) [in module %s]"),
	   header->get_length_without_initial (), sect_offset_str (header->sect_off),
	   filename);
}

/* See unit-head.h.  */

const gdb_byte *
read_and_check_unit_head (unit_head *header, dwarf2_section_info *section,
			  dwarf2_section_info *abbrev_section,
			  const gdb_byte *info_ptr, ruh_kind section_kind)
{
  const gdb_byte *beg_of_unit = info_ptr;

  header->sect_off = (sect_offset) (beg_of_unit - section->buffer);

  info_ptr = read_unit_head (header, info_ptr, section, section_kind);

  header->first_die_offset_in_unit = (cu_offset) (info_ptr - beg_of_unit);

  error_check_unit_head (header, section, abbrev_section);

  return info_ptr;
}

unrelocated_addr
unit_head::read_address (bfd *abfd, const gdb_byte *buf,
			 unsigned int *bytes_read) const
{
  ULONGEST retval = 0;

  if (signed_addr_p)
    {
      switch (addr_size)
	{
	case 2:
	  retval = bfd_get_signed_16 (abfd, buf);
	  break;
	case 4:
	  retval = bfd_get_signed_32 (abfd, buf);
	  break;
	case 8:
	  retval = bfd_get_signed_64 (abfd, buf);
	  break;
	default:
	  internal_error (_("read_address: bad switch, signed [in module %s]"),
			  bfd_get_filename (abfd));
	}
    }
  else
    {
      switch (addr_size)
	{
	case 2:
	  retval = bfd_get_16 (abfd, buf);
	  break;
	case 4:
	  retval = bfd_get_32 (abfd, buf);
	  break;
	case 8:
	  retval = bfd_get_64 (abfd, buf);
	  break;
	default:
	  internal_error (_("read_address: bad switch, "
			    "unsigned [in module %s]"),
			  bfd_get_filename (abfd));
	}
    }

  *bytes_read = addr_size;
  return (unrelocated_addr) retval;
}
