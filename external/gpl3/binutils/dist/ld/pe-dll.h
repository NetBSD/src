/* pe-dll.h: Header file for routines used to build Windows DLLs.
   Copyright 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.

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

#ifndef PE_DLL_H
#define PE_DLL_H

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "deffile.h"

extern def_file *pe_def_file;
extern int pe_dll_export_everything;
extern int pe_dll_exclude_all_symbols;
extern int pe_dll_do_default_excludes;
extern int pe_dll_kill_ats;
extern int pe_dll_stdcall_aliases;
extern int pe_dll_warn_dup_exports;
extern int pe_dll_compat_implib;
extern int pe_dll_extra_pe_debug;
extern int pe_use_nul_prefixed_import_tables;
extern int pe_use_coff_long_section_names;
extern int pe_leading_underscore;

typedef enum { EXCLUDESYMS, EXCLUDELIBS, EXCLUDEFORIMPLIB } exclude_type;

extern void pe_dll_id_target
  (const char *);
extern void pe_dll_add_excludes
  (const char *, const exclude_type);
extern void pe_dll_generate_def_file
  (const char *);
extern void pe_dll_generate_implib
  (def_file *, const char *, struct bfd_link_info *);
extern void pe_process_import_defs
  (bfd *, struct bfd_link_info *);
extern bfd_boolean pe_implied_import_dll
  (const char *);
extern void pe_dll_build_sections
  (bfd *, struct bfd_link_info *);
extern void pe_exe_build_sections
  (bfd *, struct bfd_link_info *);
extern void pe_dll_fill_sections
  (bfd *, struct bfd_link_info *);
extern void pe_exe_fill_sections
  (bfd *, struct bfd_link_info *);
extern void pe_walk_relocs_of_symbol
  (struct bfd_link_info *, const char *, int (*) (arelent *, asection *));
extern void pe_create_import_fixup
  (arelent * rel, asection *, bfd_vma);
extern bfd_boolean pe_bfd_is_dll
  (bfd *);
extern void pe_output_file_set_long_section_names
  (bfd *);

#endif /* PE_DLL_H */
