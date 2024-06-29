/* 32-bit ELF support for C-SKY.
   Copyright (C) 2019-2022 Free Software Foundation, Inc.

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

extern bool elf32_csky_build_stubs
  (struct bfd_link_info *);
extern bool elf32_csky_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section *(*) (const char*, struct bfd_section*),
   void (*) (void));
extern void elf32_csky_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern int elf32_csky_setup_section_lists
  (bfd *, struct bfd_link_info *);
