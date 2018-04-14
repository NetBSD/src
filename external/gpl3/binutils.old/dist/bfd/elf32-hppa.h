/* ELF32/HPPA support

   This file contains ELF32/HPPA relocation support as specified
   in the Stratus FTX/Golf Object File Format (SED-1762) dated
   February 1994.

   Copyright (C) 1990-2016 Free Software Foundation, Inc.

   Written by:

   Center for Software Science
   Department of Computer Science
   University of Utah

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

#ifndef _ELF32_HPPA_H
#define _ELF32_HPPA_H

#include "elf-bfd.h"
#include "libhppa.h"
#include "elf/hppa.h"

#ifdef __cplusplus
extern "C" {
#endif

void elf32_hppa_init_stub_bfd
  (bfd *, struct bfd_link_info *);

int elf32_hppa_setup_section_lists
  (bfd *, struct bfd_link_info *);

void elf32_hppa_next_input_section
  (struct bfd_link_info *, asection *);

bfd_boolean elf32_hppa_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_boolean, bfd_signed_vma,
   asection * (*) (const char *, asection *), void (*) (void));

bfd_boolean elf32_hppa_set_gp
  (bfd *, struct bfd_link_info *);

bfd_boolean elf32_hppa_build_stubs
  (struct bfd_link_info *);

elf_hppa_reloc_type elf32_hppa_reloc_final_type
  (bfd *, elf_hppa_reloc_type, int, unsigned int);

extern elf_hppa_reloc_type ** _bfd_elf32_hppa_gen_reloc_type
  (bfd *, elf_hppa_reloc_type, int, unsigned int, int, asymbol *);

/* Define groups of basic relocations.  FIXME:  These should
   be the only basic relocations created by GAS.  The rest
   should be internal to the BFD backend.

   The idea is both SOM and ELF define these basic relocation
   types so they map into a SOM or ELF specific relocation
   as appropriate.  This allows GAS to share much more code
   between the two target object formats.  */

#define R_HPPA_NONE			R_PARISC_NONE
#define R_HPPA				R_PARISC_DIR32
#define R_HPPA_GOTOFF			R_PARISC_DPREL21L
#define R_HPPA_PCREL_CALL		R_PARISC_PCREL21L
#define R_HPPA_ABS_CALL			R_PARISC_DIR17F
#define R_HPPA_COMPLEX			R_PARISC_UNIMPLEMENTED

#ifdef __cplusplus
}
#endif

#endif /* _ELF32_HPPA_H */
