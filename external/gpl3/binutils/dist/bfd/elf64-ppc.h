/* PowerPC64-specific support for 64-bit ELF.
   Copyright 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2012
   Free Software Foundation, Inc.

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

void ppc64_elf_init_stub_bfd
  (bfd *, struct bfd_link_info *);
bfd_boolean ppc64_elf_edit_opd
  (struct bfd_link_info *, bfd_boolean);
asection *ppc64_elf_tls_setup
  (struct bfd_link_info *, int, int *);
bfd_boolean ppc64_elf_tls_optimize
  (struct bfd_link_info *);
bfd_boolean ppc64_elf_edit_toc
  (struct bfd_link_info *);
bfd_boolean ppc64_elf_has_small_toc_reloc
  (asection *);
bfd_vma ppc64_elf_toc
  (bfd *);
int ppc64_elf_setup_section_lists
  (struct bfd_link_info *, asection *(*) (const char *, asection *),
   void (*) (void));
void ppc64_elf_start_multitoc_partition
  (struct bfd_link_info *);
bfd_boolean ppc64_elf_next_toc_section
  (struct bfd_link_info *, asection *);
bfd_boolean ppc64_elf_layout_multitoc
  (struct bfd_link_info *);
void ppc64_elf_finish_multitoc_partition
  (struct bfd_link_info *);
bfd_boolean ppc64_elf_check_init_fini
  (struct bfd_link_info *);
bfd_boolean ppc64_elf_next_input_section
  (struct bfd_link_info *, asection *);
bfd_boolean ppc64_elf_size_stubs
(struct bfd_link_info *, bfd_signed_vma, bfd_boolean, int, int);
bfd_boolean ppc64_elf_build_stubs
  (bfd_boolean, struct bfd_link_info *, char **);
void ppc64_elf_restore_symbols
  (struct bfd_link_info *info);
