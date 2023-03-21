/* DWARF abbrev table

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

#ifndef GDB_DWARF2_ABBREV_H
#define GDB_DWARF2_ABBREV_H

#include "hashtab.h"

/* This data structure holds the information of an abbrev.  */
struct abbrev_info
  {
    unsigned int number;	/* number identifying abbrev */
    enum dwarf_tag tag;		/* dwarf tag */
    unsigned short has_children;		/* boolean */
    unsigned short num_attrs;	/* number of attributes */
    struct attr_abbrev *attrs;	/* an array of attribute descriptions */
  };

struct attr_abbrev
  {
    ENUM_BITFIELD(dwarf_attribute) name : 16;
    ENUM_BITFIELD(dwarf_form) form : 16;

    /* It is valid only if FORM is DW_FORM_implicit_const.  */
    LONGEST implicit_const;
  };

struct abbrev_table;
typedef std::unique_ptr<struct abbrev_table> abbrev_table_up;

/* Top level data structure to contain an abbreviation table.  */

struct abbrev_table
{
  static abbrev_table_up read (struct objfile *objfile,
			       struct dwarf2_section_info *section,
			       sect_offset sect_off);

  /* Look up an abbrev in the table.
     Returns NULL if the abbrev is not found.  */

  struct abbrev_info *lookup_abbrev (unsigned int abbrev_number)
  {
    struct abbrev_info search;
    search.number = abbrev_number;

    return (struct abbrev_info *) htab_find_with_hash (m_abbrevs.get (),
						       &search,
						       abbrev_number);
  }

  /* Where the abbrev table came from.
     This is used as a sanity check when the table is used.  */
  const sect_offset sect_off;

private:

  explicit abbrev_table (sect_offset off);

  DISABLE_COPY_AND_ASSIGN (abbrev_table);

  /* Allocate space for a struct abbrev_info object in
     ABBREV_TABLE.  */
  struct abbrev_info *alloc_abbrev ();

  /* Add an abbreviation to the table.  */
  void add_abbrev (struct abbrev_info *abbrev);

  /* Hash table of abbrevs.  */
  htab_up m_abbrevs;

  /* Storage for the abbrev table.  */
  auto_obstack m_abbrev_obstack;
};

#endif /* GDB_DWARF2_ABBREV_H */
