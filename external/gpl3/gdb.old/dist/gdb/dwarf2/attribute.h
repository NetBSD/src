/* DWARF attributes

   Copyright (C) 1994-2020 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_ATTRIBUTE_H
#define GDB_DWARF2_ATTRIBUTE_H

#include "dwarf2.h"
#include "gdbtypes.h"

/* Blocks are a bunch of untyped bytes.  */
struct dwarf_block
{
  size_t size;

  /* Valid only if SIZE is not zero.  */
  const gdb_byte *data;
};

/* Attributes have a name and a value.  */
struct attribute
{
  /* Read the given attribute value as an address, taking the
     attribute's form into account.  */
  CORE_ADDR value_as_address () const;

  /* If the attribute has a string form, return the string value;
     otherwise return NULL.  */
  const char *value_as_string () const;

  /* Return non-zero if ATTR's value is a section offset --- classes
     lineptr, loclistptr, macptr or rangelistptr --- or zero, otherwise.
     You may use DW_UNSND (attr) to retrieve such offsets.

     Section 7.5.4, "Attribute Encodings", explains that no attribute
     may have a value that belongs to more than one of these classes; it
     would be ambiguous if we did, because we use the same forms for all
     of them.  */

  bool form_is_section_offset () const;

  /* Return non-zero if ATTR's value falls in the 'constant' class, or
     zero otherwise.  When this function returns true, you can apply
     the constant_value method to it.

     However, note that for some attributes you must check
     attr_form_is_section_offset before using this test.  DW_FORM_data4
     and DW_FORM_data8 are members of both the constant class, and of
     the classes that contain offsets into other debug sections
     (lineptr, loclistptr, macptr or rangelistptr).  The DWARF spec says
     that, if an attribute's can be either a constant or one of the
     section offset classes, DW_FORM_data4 and DW_FORM_data8 should be
     taken as section offsets, not constants.

     DW_FORM_data16 is not considered as constant_value cannot handle
     that.  */

  bool form_is_constant () const;

  /* DW_ADDR is always stored already as sect_offset; despite for the forms
     besides DW_FORM_ref_addr it is stored as cu_offset in the DWARF file.  */

  bool form_is_ref () const
  {
    return (form == DW_FORM_ref_addr
	    || form == DW_FORM_ref1
	    || form == DW_FORM_ref2
	    || form == DW_FORM_ref4
	    || form == DW_FORM_ref8
	    || form == DW_FORM_ref_udata
	    || form == DW_FORM_GNU_ref_alt);
  }

  /* Check if the attribute's form is a DW_FORM_block*
     if so return true else false.  */

  bool form_is_block () const;

  /* Return DIE offset of this attribute.  Return 0 with complaint if
     the attribute is not of the required kind.  */

  sect_offset get_ref_die_offset () const
  {
    if (form_is_ref ())
      return (sect_offset) u.unsnd;
    get_ref_die_offset_complaint ();
    return {};
  }

  /* Return the constant value held by this attribute.  Return
     DEFAULT_VALUE if the value held by the attribute is not
     constant.  */

  LONGEST constant_value (int default_value) const;


  ENUM_BITFIELD(dwarf_attribute) name : 16;
  ENUM_BITFIELD(dwarf_form) form : 15;

  /* Has DW_STRING already been updated by dwarf2_canonicalize_name?  This
     field should be in u.str (existing only for DW_STRING) but it is kept
     here for better struct attribute alignment.  */
  unsigned int string_is_canonical : 1;

  union
    {
      const char *str;
      struct dwarf_block *blk;
      ULONGEST unsnd;
      LONGEST snd;
      CORE_ADDR addr;
      ULONGEST signature;
    }
  u;

private:

  /* Used by get_ref_die_offset to issue a complaint.  */

  void get_ref_die_offset_complaint () const;
};

/* Get at parts of an attribute structure.  */

#define DW_STRING(attr)    ((attr)->u.str)
#define DW_STRING_IS_CANONICAL(attr) ((attr)->string_is_canonical)
#define DW_UNSND(attr)     ((attr)->u.unsnd)
#define DW_BLOCK(attr)     ((attr)->u.blk)
#define DW_SND(attr)       ((attr)->u.snd)
#define DW_ADDR(attr)	   ((attr)->u.addr)
#define DW_SIGNATURE(attr) ((attr)->u.signature)

#endif /* GDB_DWARF2_ATTRIBUTE_H */
