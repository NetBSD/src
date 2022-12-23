/* Emulation code used by all ELF targets.
   Copyright (C) 1991-2020 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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

struct elf_sym_strtab;
struct elf_strtab_hash;
struct ctf_file;

extern void ldelf_map_segments (bfd_boolean);
extern int ldelf_emit_ctf_early (void);
extern void ldelf_examine_strtab_for_ctf
  (struct ctf_file *ctf_output, struct elf_sym_strtab *syms,
   bfd_size_type symcount, struct elf_strtab_hash *symstrtab);
