/* MIPS ELF specific backend routines.
   Copyright 2002 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "elf/common.h"
#include "elf/internal.h"

extern void _bfd_mips_elf_symbol_processing
  PARAMS ((bfd *, asymbol *));
extern boolean _bfd_mips_elf_section_processing
  PARAMS ((bfd *, Elf_Internal_Shdr *));
extern boolean _bfd_mips_elf_section_from_shdr
  PARAMS ((bfd *, Elf_Internal_Shdr *, const char *));
extern boolean _bfd_mips_elf_fake_sections
  PARAMS ((bfd *, Elf_Internal_Shdr *, asection *));
extern boolean _bfd_mips_elf_section_from_bfd_section
  PARAMS ((bfd *, asection *, int *));
extern boolean _bfd_mips_elf_add_symbol_hook
  PARAMS ((bfd *, struct bfd_link_info *, const Elf_Internal_Sym *,
	   const char **, flagword *, asection **, bfd_vma *));
extern boolean _bfd_mips_elf_link_output_symbol_hook
  PARAMS ((bfd *, struct bfd_link_info *, const char *, Elf_Internal_Sym *,
	   asection *));
extern boolean _bfd_mips_elf_create_dynamic_sections
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean _bfd_mips_elf_check_relocs
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   const Elf_Internal_Rela *));
extern boolean _bfd_mips_elf_adjust_dynamic_symbol
  PARAMS ((struct bfd_link_info *, struct elf_link_hash_entry *));
extern boolean _bfd_mips_elf_always_size_sections
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean _bfd_mips_elf_size_dynamic_sections
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean _bfd_mips_elf_relocate_section
  PARAMS ((bfd *, struct bfd_link_info *, bfd *, asection *, bfd_byte *,
	   Elf_Internal_Rela *, Elf_Internal_Sym *, asection **));
extern boolean _bfd_mips_elf_finish_dynamic_symbol
  PARAMS ((bfd *, struct bfd_link_info *, struct elf_link_hash_entry *,
	   Elf_Internal_Sym *));
extern boolean _bfd_mips_elf_finish_dynamic_sections
  PARAMS ((bfd *, struct bfd_link_info *));
extern void _bfd_mips_elf_final_write_processing
  PARAMS ((bfd *, boolean));
extern int _bfd_mips_elf_additional_program_headers
  PARAMS ((bfd *));
extern boolean _bfd_mips_elf_modify_segment_map
  PARAMS ((bfd *));
extern asection * _bfd_mips_elf_gc_mark_hook
  PARAMS ((asection *, struct bfd_link_info *, Elf_Internal_Rela *,
	   struct elf_link_hash_entry *, Elf_Internal_Sym *));
extern boolean _bfd_mips_elf_gc_sweep_hook
  PARAMS ((bfd *, struct bfd_link_info *, asection *,
	   const Elf_Internal_Rela *));
extern void _bfd_mips_elf_copy_indirect_symbol
  PARAMS ((struct elf_backend_data *, struct elf_link_hash_entry *,
	   struct elf_link_hash_entry *));
extern void _bfd_mips_elf_hide_symbol
  PARAMS ((struct bfd_link_info *, struct elf_link_hash_entry *, boolean));
extern boolean _bfd_mips_elf_ignore_discarded_relocs
  PARAMS ((asection *));
extern boolean _bfd_mips_elf_find_nearest_line
  PARAMS ((bfd *, asection *, asymbol **, bfd_vma, const char **,
	   const char **, unsigned int *));
extern boolean _bfd_mips_elf_set_section_contents
  PARAMS ((bfd *, asection *, PTR, file_ptr, bfd_size_type));
extern bfd_byte *_bfd_elf_mips_get_relocated_section_contents
  PARAMS ((bfd *, struct bfd_link_info *, struct bfd_link_order *,
	   bfd_byte *, boolean, asymbol **));
extern struct bfd_link_hash_table *_bfd_mips_elf_link_hash_table_create
  PARAMS ((bfd *));
extern boolean _bfd_mips_elf_final_link
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean _bfd_mips_elf_merge_private_bfd_data
  PARAMS ((bfd *, bfd *));
extern boolean _bfd_mips_elf_set_private_flags
  PARAMS ((bfd *, flagword));
extern boolean _bfd_mips_elf_print_private_bfd_data
  PARAMS ((bfd *, PTR));

extern boolean _bfd_mips_elf_read_ecoff_info
  PARAMS ((bfd *, asection *, struct ecoff_debug_info *));
extern bfd_reloc_status_type _bfd_mips_elf_gprel16_with_gp
  PARAMS ((bfd *, asymbol *, arelent *, asection *, boolean, PTR, bfd_vma));
extern bfd_reloc_status_type _bfd_mips_elf32_gprel16_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));
extern unsigned long _bfd_elf_mips_mach
  PARAMS ((flagword));
