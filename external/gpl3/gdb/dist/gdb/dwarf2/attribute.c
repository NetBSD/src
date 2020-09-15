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

#include "defs.h"
#include "dwarf2/attribute.h"
#include "dwarf2/stringify.h"
#include "complaints.h"

/* See attribute.h.  */

CORE_ADDR
attribute::value_as_address () const
{
  CORE_ADDR addr;

  if (form != DW_FORM_addr && form != DW_FORM_addrx
      && form != DW_FORM_GNU_addr_index)
    {
      /* Aside from a few clearly defined exceptions, attributes that
	 contain an address must always be in DW_FORM_addr form.
	 Unfortunately, some compilers happen to be violating this
	 requirement by encoding addresses using other forms, such
	 as DW_FORM_data4 for example.  For those broken compilers,
	 we try to do our best, without any guarantee of success,
	 to interpret the address correctly.  It would also be nice
	 to generate a complaint, but that would require us to maintain
	 a list of legitimate cases where a non-address form is allowed,
	 as well as update callers to pass in at least the CU's DWARF
	 version.  This is more overhead than what we're willing to
	 expand for a pretty rare case.  */
      addr = DW_UNSND (this);
    }
  else
    addr = DW_ADDR (this);

  return addr;
}

/* See attribute.h.  */

const char *
attribute::value_as_string () const
{
  if (form == DW_FORM_strp || form == DW_FORM_line_strp
      || form == DW_FORM_string
      || form == DW_FORM_strx
      || form == DW_FORM_strx1
      || form == DW_FORM_strx2
      || form == DW_FORM_strx3
      || form == DW_FORM_strx4
      || form == DW_FORM_GNU_str_index
      || form == DW_FORM_GNU_strp_alt)
    return DW_STRING (this);
  return nullptr;
}

/* See attribute.h.  */

bool
attribute::form_is_block () const
{
  return (form == DW_FORM_block1
	  || form == DW_FORM_block2
	  || form == DW_FORM_block4
	  || form == DW_FORM_block
	  || form == DW_FORM_exprloc);
}

/* See attribute.h.  */

bool
attribute::form_is_section_offset () const
{
  return (form == DW_FORM_data4
          || form == DW_FORM_data8
	  || form == DW_FORM_sec_offset
	  || form == DW_FORM_loclistx);
}

/* See attribute.h.  */

bool
attribute::form_is_constant () const
{
  switch (form)
    {
    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_implicit_const:
      return true;
    default:
      return false;
    }
}

/* See attribute.h.  */

void
attribute::get_ref_die_offset_complaint () const
{
  complaint (_("unsupported die ref attribute form: '%s'"),
	     dwarf_form_name (form));
}

/* See attribute.h.  */

LONGEST
attribute::constant_value (int default_value) const
{
  if (form == DW_FORM_sdata || form == DW_FORM_implicit_const)
    return DW_SND (this);
  else if (form == DW_FORM_udata
	   || form == DW_FORM_data1
	   || form == DW_FORM_data2
	   || form == DW_FORM_data4
	   || form == DW_FORM_data8)
    return DW_UNSND (this);
  else
    {
      /* For DW_FORM_data16 see attribute::form_is_constant.  */
      complaint (_("Attribute value is not a constant (%s)"),
		 dwarf_form_name (form));
      return default_value;
    }
}
