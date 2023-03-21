/* DWARF DIEs

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_DIE_H
#define GDB_DWARF2_DIE_H

/* This data structure holds a complete die structure.  */
struct die_info
{
  /* Return the named attribute or NULL if not there, but do not
     follow DW_AT_specification, etc.  */
  struct attribute *attr (dwarf_attribute name)
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == name)
	return &attrs[i];
    return NULL;
  }

  /* Return the address base of the compile unit, which, if exists, is
     stored either at the attribute DW_AT_GNU_addr_base, or
     DW_AT_addr_base.  */
  gdb::optional<ULONGEST> addr_base ()
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == DW_AT_addr_base
	  || attrs[i].name == DW_AT_GNU_addr_base)
	{
	  /* If both exist, just use the first one.  */
	  return DW_UNSND (&attrs[i]);
	}
    return gdb::optional<ULONGEST> ();
  }

  /* Return range lists base of the compile unit, which, if exists, is
     stored either at the attribute DW_AT_rnglists_base or
     DW_AT_GNU_ranges_base.  */
  ULONGEST ranges_base ()
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == DW_AT_rnglists_base
	  || attrs[i].name == DW_AT_GNU_ranges_base)
	{
	  /* If both exist, just use the first one.  */
	  return DW_UNSND (&attrs[i]);
	}
    return 0;
  }


  /* DWARF-2 tag for this DIE.  */
  ENUM_BITFIELD(dwarf_tag) tag : 16;

  /* Number of attributes */
  unsigned char num_attrs;

  /* True if we're presently building the full type name for the
     type derived from this DIE.  */
  unsigned char building_fullname : 1;

  /* True if this die is in process.  PR 16581.  */
  unsigned char in_process : 1;

  /* True if this DIE has children.  */
  unsigned char has_children : 1;

  /* Abbrev number */
  unsigned int abbrev;

  /* Offset in .debug_info or .debug_types section.  */
  sect_offset sect_off;

  /* The dies in a compilation unit form an n-ary tree.  PARENT
     points to this die's parent; CHILD points to the first child of
     this node; and all the children of a given node are chained
     together via their SIBLING fields.  */
  struct die_info *child;	/* Its first child, if any.  */
  struct die_info *sibling;	/* Its next sibling, if any.  */
  struct die_info *parent;	/* Its parent, if any.  */

  /* An array of attributes, with NUM_ATTRS elements.  There may be
     zero, but it's not common and zero-sized arrays are not
     sufficiently portable C.  */
  struct attribute attrs[1];
};

#endif /* GDB_DWARF2_DIE_H */
