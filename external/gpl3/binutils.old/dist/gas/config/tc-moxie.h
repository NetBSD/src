/* tc-moxie.h -- Header file for tc-moxie.c.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with GAS; see the file COPYING.  If not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#define TC_MOXIE 1
#ifndef TARGET_BYTES_BIG_ENDIAN
#define TARGET_BYTES_BIG_ENDIAN 1
#endif
#define WORKING_DOT_WORD

/* This macro is the BFD architecture to pass to `bfd_set_arch_mach'.  */
#define TARGET_FORMAT (target_big_endian ? "elf32-bigmoxie" : "elf32-littlemoxie")

#define TARGET_ARCH bfd_arch_moxie

#define md_undefined_symbol(NAME)           0

/* These macros must be defined, but is will be a fatal assembler
   error if we ever hit them.  */
#define md_estimate_size_before_relax(A, B) (as_fatal (_("estimate size\n")), 0)
#define md_convert_frag(B, S, F)            as_fatal (_("convert_frag\n"))

/* If you define this macro, it should return the offset between the
   address of a PC relative fixup and the position from which the PC
   relative adjustment should be made.  On many processors, the base
   of a PC relative instruction is the next instruction, so this
   macro would return the length of an instruction.  */
#define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from (FIX)
extern long md_pcrel_from (struct fix *);

#define md_section_align(SEGMENT, SIZE)     (SIZE)
