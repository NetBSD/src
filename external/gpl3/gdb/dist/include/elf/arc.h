/* ARC ELF support for BFD.
   Copyright (C) 1995-2017 Free Software Foundation, Inc.
   Contributed by Doug Evans, (dje@cygnus.com)

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

/* This file holds definitions specific to the ARC ELF ABI.  */

#ifndef _ELF_ARC_H
#define _ELF_ARC_H

#include "elf/reloc-macros.h"

/* Relocations.  */

#define ARC_RELOC_HOWTO(TYPE, VALUE, SIZE, BITSIZE, RELOC_FUNCTION, OVERFLOW, FORMULA) \
  RELOC_NUMBER(R_##TYPE, VALUE)

START_RELOC_NUMBERS (elf_arc_reloc_type)
#include "arc-reloc.def"
END_RELOC_NUMBERS (R_ARC_max)

#undef ARC_RELOC_HOWTO

/* Processor specific flags for the ELF header e_flags field.  */

#define EF_ARC_MACH_MSK	 0x000000ff
#define EF_ARC_OSABI_MSK 0x00000f00
#define EF_ARC_ALL_MSK	 (EF_ARC_MACH_MSK | EF_ARC_OSABI_MSK)

/* Various CPU types.  These numbers are exposed in the ELF header flags
   (e_flags field), and so must never change.  */
#define E_ARC_MACH_ARC600	0x00000002
#define E_ARC_MACH_ARC601	0x00000004
#define E_ARC_MACH_ARC700	0x00000003
#define EF_ARC_CPU_ARCV2EM      0x00000005
#define EF_ARC_CPU_ARCV2HS      0x00000006

/* ARC Linux specific ABIs.  */
#define E_ARC_OSABI_ORIG	0x00000000   /* MUST be 0 for back-compat.  */
#define E_ARC_OSABI_V2		0x00000200
#define E_ARC_OSABI_V3		0x00000300
#define E_ARC_OSABI_CURRENT	E_ARC_OSABI_V3

/* Leave bits 0xf0 alone in case we ever have more than 16 cpu types.  */

/* File contains position independent code.  */

#define EF_ARC_PIC 0x00000100

#endif /* _ELF_ARC_H */
