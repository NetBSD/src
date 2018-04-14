/* tc-ft32.h -- Header file for tc-ft32.c.

   Copyright (C) 2013-2016 Free Software Foundation, Inc.
   Contributed by FTDI (support@ftdichip.com)

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

#define TC_FT32 1
#define TARGET_BYTES_BIG_ENDIAN 0
#define WORKING_DOT_WORD

/* This macro is the BFD architecture to pass to `bfd_set_arch_mach'.  */
#define TARGET_FORMAT  "elf32-ft32"

#define TARGET_ARCH bfd_arch_ft32

#define md_undefined_symbol(NAME)           0

/* These macros must be defined, but is will be a fatal assembler
   error if we ever hit them.  */
#define md_estimate_size_before_relax(A, B) (as_fatal (_("estimate size\n")), 0)
#define md_convert_frag(B, S, F)            (as_fatal (_("convert_frag\n")))

/* If you define this macro, it should return the offset between the
   address of a PC relative fixup and the position from which the PC
   relative adjustment should be made.  On many processors, the base
   of a PC relative instruction is the next instruction, so this
   macro would return the length of an instruction.  */
// #define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from (FIX)
// extern long md_pcrel_from (struct fix *);

/* PC relative operands are relative to the start of the opcode, and
   the operand is always one byte into the opcode.  */
#define md_pcrel_from(FIX)						\
	((FIX)->fx_where + (FIX)->fx_frag->fr_address - 1)

#define md_section_align(SEGMENT, SIZE)     (SIZE)

#define md_operand(x)
