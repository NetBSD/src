/* Target sections.

   Copyright (C) 2020-2023 Free Software Foundation, Inc.

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

#ifndef GDB_TARGET_SECTION_H
#define GDB_TARGET_SECTION_H

/* Struct target_section maps address ranges to file sections.  It is
   mostly used with BFD files, but can be used without (e.g. for handling
   raw disks, or files not in formats handled by BFD).  */

struct target_section
{
  target_section (CORE_ADDR addr_, CORE_ADDR end_, struct bfd_section *sect_,
		  void *owner_ = nullptr)
    : addr (addr_),
      endaddr (end_),
      the_bfd_section (sect_),
      owner (owner_)
  {
  }

  /* Lowest address in section.  */
  CORE_ADDR addr;
  /* Highest address in section, plus 1.  */
  CORE_ADDR endaddr;

  /* The BFD section.  */
  struct bfd_section *the_bfd_section;

  /* The "owner" of the section.
     It can be any unique value.  It is set by add_target_sections
     and used by remove_target_sections.
     For example, for executables it is a pointer to exec_bfd and
     for shlibs it is the so_list pointer.  */
  void *owner;
};

/* Holds an array of target sections.  */

using target_section_table = std::vector<target_section>;

#endif /* GDB_TARGET_SECTION_H */
