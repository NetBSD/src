/* S/390-specific support for ELF.
   Copyright (C) 2017-2019 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* Used to pass info between ld and bfd.  */
struct s390_elf_params
{
  /* Tell the kernel to allocate 4k page tables.  */
  int pgste;
};

bfd_boolean bfd_elf_s390_set_options (struct bfd_link_info *info,
				      struct s390_elf_params *params);
