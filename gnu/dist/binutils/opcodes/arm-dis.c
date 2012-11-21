/* Instruction printing code for the ARM
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
   Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)
   Modification by James G. Smith (jsmith@cygnus.co.uk)

   This file is part of libopcodes.

   This program is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sysdep.h"

#include "dis-asm.h"
#include "opcode/arm.h"
#include "arm-opc.h"
#include "coff/internal.h"
#include "libcoff.h"
#include "opintl.h"
#include "safe-ctype.h"

/* FIXME: This shouldn't be done here.  */
#include "elf-bfd.h"
#include "elf/internal.h"
#include "elf/arm.h"

#ifndef streq
#define streq(a,b)	(strcmp ((a), (b)) == 0)
#endif

#ifndef strneq
#define strneq(a,b,n)	(strncmp ((a), (b), (n)) == 0)
#endif

#ifndef NUM_ELEM
#define NUM_ELEM(a)     (sizeof (a) / sizeof (a)[0])
#endif

#define WORD_ADDRESS(pc) ((pc) & ~0x3)

/* Format of the disassembler control string :
   
   %%			%
   %<bitfield>d		print the bitfield in decimal
   %<bitfield>x		print the bitfield in hex
   %<bitfield>X		print the bitfield as 1 hex digit without leading "0x"
   %<bitfield>W         print the bitfield plus one in decimal 
   %<bitfield>r		print as an ARM register
   %<bitfield>f		print a floating point constant if >7 else a
			floating point register
   %<code>y		print a single precision VFP reg.
			  Codes: 0=>Sm, 1=>Sd, 2=>Sn, 3=>multi-list, 4=>Sm pair
   %<code>z		print a double precision VFP reg
			  Codes: 0=>Dm, 1=>Dd, 2=>Dn, 3=>multi-list
   %c			print condition code (always bits 28-31)
   %P			print floating point precision in arithmetic insn
   %Q			print floating point precision in ldf/stf insn
   %R			print floating point rounding mode
   %<bitnum>'c		print specified char iff bit is one
   %<bitnum>`c		print specified char iff bit is zero
   %<bitnum>?ab		print a if bit is one else print b
   %p			print 'p' iff bits 12-15 are 15
   %t			print 't' iff bit 21 set and bit 24 clear
   %o			print operand2 (immediate or register + shift)
   %a			print address for ldr/str instruction
   %s                   print address for ldr/str halfword/signextend instruction
   %b			print branch destination
   %B			print arm BLX(1) destination
   %A			print address for ldc/stc/ldf/stf instruction
   %m			print register mask for ldm/stm instruction
   %C			print the PSR sub type.
   %E			print the LSB and WIDTH fields of a BFI or BFC instruction.
   %F			print the COUNT field of a LFM/SFM instruction.
IWMMXT specific format options:
   %<bitfield>g         print as an iWMMXt 64-bit register
   %<bitfield>G         print as an iWMMXt general purpose or control register
   %<bitfield>w         print as an iWMMXt width field - [bhwd]ss/us
   %Z			print the Immediate of a WSHUFH instruction.
   %L			print as an iWMMXt N/M width field.
   %l			like 'A' except use byte offsets for 'B' & 'H' versions
Thumb specific format options:
   %D                   print Thumb register (bits 0..2 as high number if bit 7 set)
   %S                   print Thumb register (bits 3..5 as high number if bit 6 set)
   %<bitfield>I         print bitfield as a signed decimal
   				(top bit of range being the sign bit)
   %M                   print Thumb register mask
   %N                   print Thumb register mask (with LR)
   %O                   print Thumb register mask (with PC)
   %T                   print Thumb condition code (always bits 8-11)
   %I                   print cirrus signed shift immediate: bits 0..3|4..6
   %<bitfield>B         print Thumb branch destination (signed displacement)
   %<bitfield>W         print (bitfield * 4) as a decimal
   %<bitfield>H         print (bitfield * 2) as a decimal
   %<bitfield>a         print (bitfield * 4) as a pc-rel offset + decoded symbol
   %e                   print arm SMI operand (bits 0..7,8..19).  */

/* Note: There is a partial ordering in this table - it must be searched from
   the top to obtain a correct match.  */

static const struct arm_opcode arm_opcodes[] =
{
  /* ARM instructions.  */
  {ARM_EXT_V1, 0xe1a00000, 0xffffffff, "nop\t\t\t(mov r0,r0)"},
  {ARM_EXT_V4T | ARM_EXT_V5, 0x012FFF10, 0x0ffffff0, "bx%c\t%0-3r"},
  {ARM_EXT_V2, 0x00000090, 0x0fe000f0, "mul%c%20's\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V2, 0x00200090, 0x0fe000f0, "mla%c%20's\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V2S, 0x01000090, 0x0fb00ff0, "swp%c%22'b\t%12-15r, %0-3r, [%16-19r]"},
  {ARM_EXT_V3M, 0x00800090, 0x0fa000f0, "%22?sumull%c%20's\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V3M, 0x00a00090, 0x0fa000f0, "%22?sumlal%c%20's\t%12-15r, %16-19r, %0-3r, %8-11r"},

  /* ARM V7 instructions.  */
  {ARM_EXT_V7A, 0x07a00050, 0x0fa00070, "%22?usbfx%c\t%12-15r, %0-3r, #%7-11d, #%16-20W"},
  {ARM_EXT_V7A, 0x07c0001f, 0x0fa0007f, "bfc%c\t%12-15R, %E"},
  {ARM_EXT_V7A, 0x07c00010, 0x0fa00070, "bfi%c\t%12-15R, %0-3r, %E"},
  {ARM_EXT_V7A, 0xf57ff05f, 0xffffffff, "dmb"},
  {ARM_EXT_V7A, 0xf57ff050, 0xfffffff0, "dmb\t#%0-3d"},
  {ARM_EXT_V7A, 0xf57ff05f, 0xffffffff, "dsb"},
  {ARM_EXT_V7A, 0xf57ff050, 0xfffffff0, "dsb\t%#0-3d"},
  {ARM_EXT_V7A, 0xf57ff06f, 0xffffffff, "isb"},
  {ARM_EXT_V7A, 0xf57ff060, 0xfffffff0, "isb\t%#0-3d"},

  /* ARM V6Z instructions.  */
  {ARM_EXT_V6Z, 0x01600070, 0x0ff000f0, "smi%c\t%e"},

  /* ARM V6K instructions.  */
  {ARM_EXT_V6K, 0xf57ff01f, 0xffffffff, "clrex"},
  {ARM_EXT_V6K, 0x01d00f9f, 0x0ff00fff, "ldrexb%c\t%12-15r, [%16-19r]"},
  {ARM_EXT_V6K, 0x01b00f9f, 0x0ff00fff, "ldrexd%c\t%12-15r, [%16-19r]"},
  {ARM_EXT_V6K, 0x01f00f9f, 0x0ff00fff, "ldrexh%c\t%12-15r, [%16-19r]"},
  {ARM_EXT_V6K, 0x01c00f90, 0x0ff00ff0, "strexb%c\t%12-15r, %0-3r, [%16-19r]"},
  {ARM_EXT_V6K, 0x01a00f90, 0x0ff00ff0, "strexd%c\t%12-15r, %0-3r, [%16-19r]"},
  {ARM_EXT_V6K, 0x01e00f90, 0x0ff00ff0, "strexh%c\t%12-15r, %0-3r, [%16-19r]"},

  /* ARM V6K NOP hints.  */
  {ARM_EXT_V6K, 0x0320f001, 0x0fffffff, "yield%c"},
  {ARM_EXT_V6K, 0x0320f002, 0x0fffffff, "wfe%c"},
  {ARM_EXT_V6K, 0x0320f003, 0x0fffffff, "wfi%c"},
  {ARM_EXT_V6K, 0x0320f004, 0x0fffffff, "sev%c"},
  {ARM_EXT_V6K, 0x0320f000, 0x0fffff00, "nop%c\t{%0-7d}"},

  /* ARM V6 instructions. */
  {ARM_EXT_V6, 0xfc500000, 0xfff00000, "mrrc2\t%8-11d, %4-7d, %12-15r, %16-19r, cr%0-3d"},
  {ARM_EXT_V6, 0xfc400000, 0xfff00000, "mcrr2\t%8-11d, %4-7d, %12-15r, %16-19r, cr%0-3d"},
  {ARM_EXT_V6, 0xf1080000, 0xfffdfe3f, "cpsie\t%8'a%7'i%6'f"},
  {ARM_EXT_V6, 0xf1080000, 0xfffdfe20, "cpsie\t%8'a%7'i%6'f,#%0-4d"},
  {ARM_EXT_V6, 0xf10C0000, 0xfffdfe3f, "cpsid\t%8'a%7'i%6'f"},
  {ARM_EXT_V6, 0xf10C0000, 0xfffdfe20, "cpsid\t%8'a%7'i%6'f,#%0-4d"},
  {ARM_EXT_V6, 0xf1000000, 0xfff1fe20, "cps\t#%0-4d"},
  {ARM_EXT_V6, 0x06800010, 0x0ff00ff0, "pkhbt%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06800010, 0x0ff00070, "pkhbt%c\t%12-15r, %16-19r, %0-3r, LSL #%7-11d"},
  {ARM_EXT_V6, 0x06800050, 0x0ff00ff0, "pkhtb%c\t%12-15r, %16-19r, %0-3r, ASR #32"},
  {ARM_EXT_V6, 0x06800050, 0x0ff00070, "pkhtb%c\t%12-15r, %16-19r, %0-3r, ASR #%7-11d"},
  {ARM_EXT_V6, 0x01900f9f, 0x0ff00fff, "ldrex%c\tr%12-15d, [%16-19r]"},
  {ARM_EXT_V6, 0x06200f10, 0x0ff00ff0, "qadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06200f90, 0x0ff00ff0, "qadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06200f30, 0x0ff00ff0, "qaddsubx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06200f70, 0x0ff00ff0, "qsub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06200ff0, 0x0ff00ff0, "qsub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06200f50, 0x0ff00ff0, "qsubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100f10, 0x0ff00ff0, "sadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100f90, 0x0ff00ff0, "sadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100f30, 0x0ff00ff0, "saddaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300f10, 0x0ff00ff0, "shadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300f90, 0x0ff00ff0, "shadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300f30, 0x0ff00ff0, "shaddsubx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300f70, 0x0ff00ff0, "shsub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300ff0, 0x0ff00ff0, "shsub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06300f50, 0x0ff00ff0, "shsubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100f70, 0x0ff00ff0, "ssub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100ff0, 0x0ff00ff0, "ssub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06100f50, 0x0ff00ff0, "ssubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500f10, 0x0ff00ff0, "uadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500f90, 0x0ff00ff0, "uadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500f30, 0x0ff00ff0, "uaddsubx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700f10, 0x0ff00ff0, "uhadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700f90, 0x0ff00ff0, "uhadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700f30, 0x0ff00ff0, "uhaddsubx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700f70, 0x0ff00ff0, "uhsub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700ff0, 0x0ff00ff0, "uhsub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06700f50, 0x0ff00ff0, "uhsubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600f10, 0x0ff00ff0, "uqadd16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600f90, 0x0ff00ff0, "uqadd8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600f30, 0x0ff00ff0, "uqaddsubx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600f70, 0x0ff00ff0, "uqsub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600ff0, 0x0ff00ff0, "uqsub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06600f50, 0x0ff00ff0, "uqsubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500f70, 0x0ff00ff0, "usub16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500ff0, 0x0ff00ff0, "usub8%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06500f50, 0x0ff00ff0, "usubaddx%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06bf0f30, 0x0fff0ff0, "rev%c\t\%12-15r, %0-3r"},
  {ARM_EXT_V6, 0x06bf0fb0, 0x0fff0ff0, "rev16%c\t\%12-15r, %0-3r"},
  {ARM_EXT_V6, 0x06ff0fb0, 0x0fff0ff0, "revsh%c\t\%12-15r, %0-3r"},
  {ARM_EXT_V6, 0xf8100a00, 0xfe50ffff, "rfe%23?id%24?ba\t\%16-19r%21'!"},
  {ARM_EXT_V6, 0x06bf0070, 0x0fff0ff0, "sxth%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x06bf0470, 0x0fff0ff0, "sxth%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06bf0870, 0x0fff0ff0, "sxth%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06bf0c70, 0x0fff0ff0, "sxth%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x068f0070, 0x0fff0ff0, "sxtb16%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x068f0470, 0x0fff0ff0, "sxtb16%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x068f0870, 0x0fff0ff0, "sxtb16%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x068f0c70, 0x0fff0ff0, "sxtb16%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06af0070, 0x0fff0ff0, "sxtb%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x06af0470, 0x0fff0ff0, "sxtb%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06af0870, 0x0fff0ff0, "sxtb%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06af0c70, 0x0fff0ff0, "sxtb%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06ff0070, 0x0fff0ff0, "uxth%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x06ff0470, 0x0fff0ff0, "uxth%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06ff0870, 0x0fff0ff0, "uxth%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06ff0c70, 0x0fff0ff0, "uxth%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06cf0070, 0x0fff0ff0, "uxtb16%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x06cf0470, 0x0fff0ff0, "uxtb16%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06cf0870, 0x0fff0ff0, "uxtb16%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06cf0c70, 0x0fff0ff0, "uxtb16%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06ef0070, 0x0fff0ff0, "uxtb%c %12-15r,%0-3r"},
  {ARM_EXT_V6, 0x06ef0470, 0x0fff0ff0, "uxtb%c %12-15r,%0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06ef0870, 0x0fff0ff0, "uxtb%c %12-15r,%0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06ef0c70, 0x0fff0ff0, "uxtb%c %12-15r,%0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06b00070, 0x0ff00ff0, "sxtah%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06b00470, 0x0ff00ff0, "sxtah%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06b00870, 0x0ff00ff0, "sxtah%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06b00c70, 0x0ff00ff0, "sxtah%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06800070, 0x0ff00ff0, "sxtab16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06800470, 0x0ff00ff0, "sxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06800870, 0x0ff00ff0, "sxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06800c70, 0x0ff00ff0, "sxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06a00070, 0x0ff00ff0, "sxtab%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06a00470, 0x0ff00ff0, "sxtab%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06a00870, 0x0ff00ff0, "sxtab%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06a00c70, 0x0ff00ff0, "sxtab%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06f00070, 0x0ff00ff0, "uxtah%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06f00470, 0x0ff00ff0, "uxtah%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06f00870, 0x0ff00ff0, "uxtah%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06f00c70, 0x0ff00ff0, "uxtah%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06c00070, 0x0ff00ff0, "uxtab16%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06c00470, 0x0ff00ff0, "uxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06c00870, 0x0ff00ff0, "uxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06c00c70, 0x0ff00ff0, "uxtab16%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x06e00070, 0x0ff00ff0, "uxtab%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0x06e00470, 0x0ff00ff0, "uxtab%c\t%12-15r, %16-19r, %0-3r, ROR #8"},
  {ARM_EXT_V6, 0x06e00870, 0x0ff00ff0, "uxtab%c\t%12-15r, %16-19r, %0-3r, ROR #16"},
  {ARM_EXT_V6, 0x06e00c70, 0x0ff00ff0, "uxtab%c\t%12-15r, %16-19r, %0-3r, ROR #24"},
  {ARM_EXT_V6, 0x068000b0, 0x0ff00ff0, "sel%c\t%12-15r, %16-19r, %0-3r"},
  {ARM_EXT_V6, 0xf1010000, 0xfffffc00, "setend\t%9?ble"},
  {ARM_EXT_V6, 0x0700f010, 0x0ff0f0d0, "smuad%5'x%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x0700f050, 0x0ff0f0d0, "smusd%5'x%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x07000010, 0x0ff000d0, "smlad%5'x%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V6, 0x07400010, 0x0ff000d0, "smlald%5'x%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x07000050, 0x0ff000d0, "smlsd%5'x%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V6, 0x07400050, 0x0ff000d0, "smlsld%5'x%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x0750f010, 0x0ff0f0d0, "smmul%5'r%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x07500010, 0x0ff000d0, "smmla%5'r%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V6, 0x075000d0, 0x0ff000d0, "smmls%5'r%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V6, 0xf84d0500, 0xfe5fffe0, "srs%23?id%24?ba\t#%0-4d%21'!"},
  {ARM_EXT_V6, 0x06a00010, 0x0fe00ff0, "ssat%c\t%12-15r, #%16-20W, %0-3r"},
  {ARM_EXT_V6, 0x06a00010, 0x0fe00070, "ssat%c\t%12-15r, #%16-20W, %0-3r, LSL #%7-11d"},
  {ARM_EXT_V6, 0x06a00050, 0x0fe00070, "ssat%c\t%12-15r, #%16-20W, %0-3r, ASR #%7-11d"},
  {ARM_EXT_V6, 0x06a00f30, 0x0ff00ff0, "ssat16%c\t%12-15r, #%16-19W, %0-3r"},
  {ARM_EXT_V6, 0x01800f90, 0x0ff00ff0, "strex%c\t%12-15r, %0-3r, [%16-19r]"},
  {ARM_EXT_V6, 0x00400090, 0x0ff000f0, "umaal%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x0780f010, 0x0ff0f0f0, "usad8%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V6, 0x07800010, 0x0ff000f0, "usada8%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V6, 0x06e00010, 0x0fe00ff0, "usat%c\t%12-15r, #%16-20d, %0-3r"},
  {ARM_EXT_V6, 0x06e00010, 0x0fe00070, "usat%c\t%12-15r, #%16-20d, %0-3r, LSL #%7-11d"},
  {ARM_EXT_V6, 0x06e00050, 0x0fe00070, "usat%c\t%12-15r, #%16-20d, %0-3r, ASR #%7-11d"},
  {ARM_EXT_V6, 0x06e00f30, 0x0ff00ff0, "usat16%c\t%12-15r, #%16-19d, %0-3r"},

  /* V5J instruction.  */
  {ARM_EXT_V5J, 0x012fff20, 0x0ffffff0, "bxj%c\t%0-3r"},

  /* XScale instructions.  */
  {ARM_CEXT_XSCALE, 0x0e200010, 0x0fff0ff0, "mia%c\tacc0, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e280010, 0x0fff0ff0, "miaph%c\tacc0, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e2c0010, 0x0ffc0ff0, "mia%17'T%17`B%16'T%16`B%c\tacc0, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0c400000, 0x0ff00fff, "mar%c\tacc0, %12-15r, %16-19r"},
  {ARM_CEXT_XSCALE, 0x0c500000, 0x0ff00fff, "mra%c\t%12-15r, %16-19r, acc0"},
    
  /* Intel Wireless MMX technology instructions.  */
#define FIRST_IWMMXT_INSN 0x0e130130
#define IWMMXT_INSN_COUNT 47
  {ARM_CEXT_IWMMXT, 0x0e130130, 0x0f3f0fff, "tandc%22-23w%c\t%12-15r"},
  {ARM_CEXT_XSCALE, 0x0e400010, 0x0ff00f3f, "tbcst%6-7w%c\t%16-19g, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e130170, 0x0f3f0ff8, "textrc%22-23w%c\t%12-15r, #%0-2d"},
  {ARM_CEXT_XSCALE, 0x0e100070, 0x0f300ff0, "textrm%3?su%22-23w%c\t%12-15r, %16-19g, #%0-2d"},
  {ARM_CEXT_XSCALE, 0x0e600010, 0x0ff00f38, "tinsr%6-7w%c\t%16-19g, %12-15r, #%0-2d"},
  {ARM_CEXT_XSCALE, 0x0e000110, 0x0ff00fff, "tmcr%c\t%16-19G, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0c400000, 0x0ff00ff0, "tmcrr%c\t%0-3g, %12-15r, %16-19r"},
  {ARM_CEXT_XSCALE, 0x0e2c0010, 0x0ffc0e10, "tmia%17?tb%16?tb%c\t%5-8g, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e200010, 0x0fff0e10, "tmia%c\t%5-8g, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e280010, 0x0fff0e10, "tmiaph%c\t%5-8g, %0-3r, %12-15r"},
  {ARM_CEXT_XSCALE, 0x0e100030, 0x0f300fff, "tmovmsk%22-23w%c\t%12-15r, %16-19g"},
  {ARM_CEXT_XSCALE, 0x0e100110, 0x0ff00ff0, "tmrc%c\t%12-15r, %16-19G"},
  {ARM_CEXT_XSCALE, 0x0c500000, 0x0ff00ff0, "tmrrc%c\t%12-15r, %16-19r, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e130150, 0x0f3f0fff, "torc%22-23w%c\t%12-15r"},
  {ARM_CEXT_XSCALE, 0x0e0001c0, 0x0f300fff, "wacc%22-23w%c\t%12-15g, %16-19g"},
  {ARM_CEXT_XSCALE, 0x0e000180, 0x0f000ff0, "wadd%20-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000020, 0x0f800ff0, "waligni%c\t%12-15g, %16-19g, %0-3g, #%20-22d"},
  {ARM_CEXT_XSCALE, 0x0e800020, 0x0fc00ff0, "walignr%20-21d%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e200000, 0x0fe00ff0, "wand%20'n%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e800000, 0x0fa00ff0, "wavg2%22?hb%20'r%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000060, 0x0f300ff0, "wcmpeq%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e100060, 0x0f100ff0, "wcmpgt%21?su%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0xfc100100, 0xfe500f00, "wldrw\t%12-15G, %A"},
  {ARM_CEXT_XSCALE, 0x0c100000, 0x0e100e00, "wldr%L%c\t%12-15g, %l"},
  {ARM_CEXT_XSCALE, 0x0e400100, 0x0fc00ff0, "wmac%21?su%20'z%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e800100, 0x0fd00ff0, "wmadd%21?su%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000160, 0x0f100ff0, "wmax%21?su%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e100160, 0x0f100ff0, "wmin%21?su%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000100, 0x0fc00ff0, "wmul%21?su%20?ml%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000000, 0x0ff00ff0, "wor%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000080, 0x0f000ff0, "wpack%20-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e300040, 0x0f300ff0, "wror%22-23w%8'g%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e300148, 0x0f300ffc, "wror%22-23w%8'g%c\t%12-15g, %16-19g, %0-3G"},
  {ARM_CEXT_XSCALE, 0x0e000120, 0x0fa00ff0, "wsad%22?hb%20'z%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e0001e0, 0x0f000ff0, "wshufh%c\t%12-15g, %16-19g, #%Z"},
  {ARM_CEXT_XSCALE, 0x0e100040, 0x0f300ff0, "wsll%22-23w%8'g%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e100148, 0x0f300ffc, "wsll%22-23w%8'g%c\t%12-15g, %16-19g, %0-3G"},
  {ARM_CEXT_XSCALE, 0x0e000040, 0x0f300ff0, "wsra%22-23w%8'g%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e000148, 0x0f300ffc, "wsra%22-23w%8'g%c\t%12-15g, %16-19g, %0-3G"},
  {ARM_CEXT_XSCALE, 0x0e200040, 0x0f300ff0, "wsrl%22-23w%8'g%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e200148, 0x0f300ffc, "wsrl%22-23w%8'g%c\t%12-15g, %16-19g, %0-3G"},
  {ARM_CEXT_XSCALE, 0xfc000100, 0xfe500f00, "wstrw\t%12-15G, %A"},
  {ARM_CEXT_XSCALE, 0x0c000000, 0x0e100e00, "wstr%L%c\t%12-15g, %l"},
  {ARM_CEXT_XSCALE, 0x0e0001a0, 0x0f000ff0, "wsub%20-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e0000c0, 0x0f100fff, "wunpckeh%21?su%22-23w%c\t%12-15g, %16-19g"},
  {ARM_CEXT_XSCALE, 0x0e0000e0, 0x0f100fff, "wunpckel%21?su%22-23w%c\t%12-15g, %16-19g"},
  {ARM_CEXT_XSCALE, 0x0e1000c0, 0x0f300ff0, "wunpckih%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e1000e0, 0x0f300ff0, "wunpckil%22-23w%c\t%12-15g, %16-19g, %0-3g"},
  {ARM_CEXT_XSCALE, 0x0e100000, 0x0ff00ff0, "wxor%c\t%12-15g, %16-19g, %0-3g"},

  /* V5 Instructions.  */
  {ARM_EXT_V5, 0xe1200070, 0xfff000f0, "bkpt\t0x%16-19X%12-15X%8-11X%0-3X"},
  {ARM_EXT_V5, 0xfa000000, 0xfe000000, "blx\t%B"},
  {ARM_EXT_V5, 0x012fff30, 0x0ffffff0, "blx%c\t%0-3r"},
  {ARM_EXT_V5, 0x016f0f10, 0x0fff0ff0, "clz%c\t%12-15r, %0-3r"},
  {ARM_EXT_V5, 0xfc100000, 0xfe100000, "ldc2%22'l\t%8-11d, cr%12-15d, %A"},
  {ARM_EXT_V5, 0xfc000000, 0xfe100000, "stc2%22'l\t%8-11d, cr%12-15d, %A"},
  {ARM_EXT_V5, 0xfe000000, 0xff000010, "cdp2\t%8-11d, %20-23d, cr%12-15d, cr%16-19d, cr%0-3d, {%5-7d}"},
  {ARM_EXT_V5, 0xfe000010, 0xff100010, "mcr2\t%8-11d, %21-23d, %12-15r, cr%16-19d, cr%0-3d, {%5-7d}"},
  {ARM_EXT_V5, 0xfe100010, 0xff100010, "mrc2\t%8-11d, %21-23d, %12-15r, cr%16-19d, cr%0-3d, {%5-7d}"},

  /* V5E "El Segundo" Instructions.  */    
  {ARM_EXT_V5E, 0x000000d0, 0x0e1000f0, "ldr%cd\t%12-15r, %s"},
  {ARM_EXT_V5E, 0x000000f0, 0x0e1000f0, "str%cd\t%12-15r, %s"},
  {ARM_EXT_V5E, 0xf450f000, 0xfc70f000, "pld\t%a"},
  {ARM_EXT_V5ExP, 0x01000080, 0x0ff000f0, "smlabb%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V5ExP, 0x010000a0, 0x0ff000f0, "smlatb%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V5ExP, 0x010000c0, 0x0ff000f0, "smlabt%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V5ExP, 0x010000e0, 0x0ff000f0, "smlatt%c\t%16-19r, %0-3r, %8-11r, %12-15r"},

  {ARM_EXT_V5ExP, 0x01200080, 0x0ff000f0, "smlawb%c\t%16-19r, %0-3r, %8-11r, %12-15r"},
  {ARM_EXT_V5ExP, 0x012000c0, 0x0ff000f0, "smlawt%c\t%16-19r, %0-3r, %8-11r, %12-15r"},

  {ARM_EXT_V5ExP, 0x01400080, 0x0ff000f0, "smlalbb%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x014000a0, 0x0ff000f0, "smlaltb%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x014000c0, 0x0ff000f0, "smlalbt%c\t%12-15r, %16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x014000e0, 0x0ff000f0, "smlaltt%c\t%12-15r, %16-19r, %0-3r, %8-11r"},

  {ARM_EXT_V5ExP, 0x01600080, 0x0ff0f0f0, "smulbb%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x016000a0, 0x0ff0f0f0, "smultb%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x016000c0, 0x0ff0f0f0, "smulbt%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x016000e0, 0x0ff0f0f0, "smultt%c\t%16-19r, %0-3r, %8-11r"},

  {ARM_EXT_V5ExP, 0x012000a0, 0x0ff0f0f0, "smulwb%c\t%16-19r, %0-3r, %8-11r"},
  {ARM_EXT_V5ExP, 0x012000e0, 0x0ff0f0f0, "smulwt%c\t%16-19r, %0-3r, %8-11r"},

  {ARM_EXT_V5ExP, 0x01000050, 0x0ff00ff0,  "qadd%c\t%12-15r, %0-3r, %16-19r"},
  {ARM_EXT_V5ExP, 0x01400050, 0x0ff00ff0, "qdadd%c\t%12-15r, %0-3r, %16-19r"},
  {ARM_EXT_V5ExP, 0x01200050, 0x0ff00ff0,  "qsub%c\t%12-15r, %0-3r, %16-19r"},
  {ARM_EXT_V5ExP, 0x01600050, 0x0ff00ff0, "qdsub%c\t%12-15r, %0-3r, %16-19r"},

  /* ARM Instructions.  */
  {ARM_EXT_V1, 0x00000090, 0x0e100090, "str%c%6's%5?hb\t%12-15r, %s"},
  {ARM_EXT_V1, 0x00100090, 0x0e100090, "ldr%c%6's%5?hb\t%12-15r, %s"},
  {ARM_EXT_V1, 0x00000000, 0x0de00000, "and%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00200000, 0x0de00000, "eor%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00400000, 0x0de00000, "sub%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00600000, 0x0de00000, "rsb%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00800000, 0x0de00000, "add%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00a00000, 0x0de00000, "adc%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00c00000, 0x0de00000, "sbc%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x00e00000, 0x0de00000, "rsc%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V3, 0x0120f000, 0x0db0f000, "msr%c\t%22?SCPSR%C, %o"},
  {ARM_EXT_V3, 0x010f0000, 0x0fbf0fff, "mrs%c\t%12-15r, %22?SCPSR"},
  {ARM_EXT_V1, 0x01000000, 0x0de00000, "tst%c%p\t%16-19r, %o"},
  {ARM_EXT_V1, 0x01200000, 0x0de00000, "teq%c%p\t%16-19r, %o"},
  {ARM_EXT_V1, 0x01400000, 0x0de00000, "cmp%c%p\t%16-19r, %o"},
  {ARM_EXT_V1, 0x01600000, 0x0de00000, "cmn%c%p\t%16-19r, %o"},
  {ARM_EXT_V1, 0x01800000, 0x0de00000, "orr%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x01a00000, 0x0de00000, "mov%c%20's\t%12-15r, %o"},
  {ARM_EXT_V1, 0x01c00000, 0x0de00000, "bic%c%20's\t%12-15r, %16-19r, %o"},
  {ARM_EXT_V1, 0x01e00000, 0x0de00000, "mvn%c%20's\t%12-15r, %o"},
  {ARM_EXT_V1, 0x04000000, 0x0e100000, "str%c%22'b%t\t%12-15r, %a"},
  {ARM_EXT_V1, 0x06000000, 0x0e100ff0, "str%c%22'b%t\t%12-15r, %a"},
  {ARM_EXT_V1, 0x04000000, 0x0c100010, "str%c%22'b%t\t%12-15r, %a"},
  {ARM_EXT_V1, 0x06000010, 0x0e000010, "undefined"},
  {ARM_EXT_V1, 0x04100000, 0x0c100000, "ldr%c%22'b%t\t%12-15r, %a"},
  {ARM_EXT_V1, 0x08000000, 0x0e100000, "stm%c%23?id%24?ba\t%16-19r%21'!, %m%22'^"},
  {ARM_EXT_V1, 0x08100000, 0x0e100000, "ldm%c%23?id%24?ba\t%16-19r%21'!, %m%22'^"},
  {ARM_EXT_V1, 0x0a000000, 0x0e000000, "b%24'l%c\t%b"},
  {ARM_EXT_V1, 0x0f000000, 0x0f000000, "swi%c\t%0-23x"},

  /* Floating point coprocessor (FPA) instructions */
  {FPU_FPA_EXT_V1, 0x0e000100, 0x0ff08f10, "adf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e100100, 0x0ff08f10, "muf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e200100, 0x0ff08f10, "suf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e300100, 0x0ff08f10, "rsf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e400100, 0x0ff08f10, "dvf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e500100, 0x0ff08f10, "rdf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e600100, 0x0ff08f10, "pow%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e700100, 0x0ff08f10, "rpw%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e800100, 0x0ff08f10, "rmf%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e900100, 0x0ff08f10, "fml%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ea00100, 0x0ff08f10, "fdv%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0eb00100, 0x0ff08f10, "frd%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ec00100, 0x0ff08f10, "pol%c%P%R\t%12-14f, %16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e008100, 0x0ff08f10, "mvf%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e108100, 0x0ff08f10, "mnf%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e208100, 0x0ff08f10, "abs%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e308100, 0x0ff08f10, "rnd%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e408100, 0x0ff08f10, "sqt%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e508100, 0x0ff08f10, "log%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e608100, 0x0ff08f10, "lgn%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e708100, 0x0ff08f10, "exp%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e808100, 0x0ff08f10, "sin%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e908100, 0x0ff08f10, "cos%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ea08100, 0x0ff08f10, "tan%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0eb08100, 0x0ff08f10, "asn%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ec08100, 0x0ff08f10, "acs%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ed08100, 0x0ff08f10, "atn%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ee08100, 0x0ff08f10, "urd%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ef08100, 0x0ff08f10, "nrm%c%P%R\t%12-14f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0e000110, 0x0ff00f1f, "flt%c%P%R\t%16-18f, %12-15r"},
  {FPU_FPA_EXT_V1, 0x0e100110, 0x0fff0f98, "fix%c%R\t%12-15r, %0-2f"},
  {FPU_FPA_EXT_V1, 0x0e200110, 0x0fff0fff, "wfs%c\t%12-15r"},
  {FPU_FPA_EXT_V1, 0x0e300110, 0x0fff0fff, "rfs%c\t%12-15r"},
  {FPU_FPA_EXT_V1, 0x0e400110, 0x0fff0fff, "wfc%c\t%12-15r"},
  {FPU_FPA_EXT_V1, 0x0e500110, 0x0fff0fff, "rfc%c\t%12-15r"},
  {FPU_FPA_EXT_V1, 0x0e90f110, 0x0ff8fff0, "cmf%c\t%16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0eb0f110, 0x0ff8fff0, "cnf%c\t%16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ed0f110, 0x0ff8fff0, "cmfe%c\t%16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0ef0f110, 0x0ff8fff0, "cnfe%c\t%16-18f, %0-3f"},
  {FPU_FPA_EXT_V1, 0x0c000100, 0x0e100f00, "stf%c%Q\t%12-14f, %A"},
  {FPU_FPA_EXT_V1, 0x0c100100, 0x0e100f00, "ldf%c%Q\t%12-14f, %A"},
  {FPU_FPA_EXT_V2, 0x0c000200, 0x0e100f00, "sfm%c\t%12-14f, %F, %A"},
  {FPU_FPA_EXT_V2, 0x0c100200, 0x0e100f00, "lfm%c\t%12-14f, %F, %A"},

  /* Floating point coprocessor (VFP) instructions */
  {FPU_VFP_EXT_V1, 0x0eb00bc0, 0x0fff0ff0, "fabsd%c\t%1z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0eb00ac0, 0x0fbf0fd0, "fabss%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0e300b00, 0x0ff00ff0, "faddd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e300a00, 0x0fb00f50, "fadds%c\t%1y, %2y, %1y"},
  {FPU_VFP_EXT_V1, 0x0eb40b40, 0x0fff0f70, "fcmp%7'ed%c\t%1z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0eb40a40, 0x0fbf0f50, "fcmp%7'es%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb50b40, 0x0fff0f70, "fcmp%7'ezd%c\t%1z"},
  {FPU_VFP_EXT_V1xD, 0x0eb50a40, 0x0fbf0f70, "fcmp%7'ezs%c\t%1y"},
  {FPU_VFP_EXT_V1, 0x0eb00b40, 0x0fff0ff0, "fcpyd%c\t%1z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0eb00a40, 0x0fbf0fd0, "fcpys%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb70ac0, 0x0fff0fd0, "fcvtds%c\t%1z, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb70bc0, 0x0fbf0ff0, "fcvtsd%c\t%1y, %0z"},
  {FPU_VFP_EXT_V1, 0x0e800b00, 0x0ff00ff0, "fdivd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e800a00, 0x0fb00f50, "fdivs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0d100b00, 0x0f700f00, "fldd%c\t%1z, %A"},
  {FPU_VFP_EXT_V1xD, 0x0c900b00, 0x0fd00f00, "fldmia%0?xd%c\t%16-19r%21'!, %3z"},
  {FPU_VFP_EXT_V1xD, 0x0d300b00, 0x0ff00f00, "fldmdb%0?xd%c\t%16-19r!, %3z"},
  {FPU_VFP_EXT_V1xD, 0x0d100a00, 0x0f300f00, "flds%c\t%1y, %A"},
  {FPU_VFP_EXT_V1xD, 0x0c900a00, 0x0f900f00, "fldmias%c\t%16-19r%21'!, %3y"},
  {FPU_VFP_EXT_V1xD, 0x0d300a00, 0x0fb00f00, "fldmdbs%c\t%16-19r!, %3y"},
  {FPU_VFP_EXT_V1, 0x0e000b00, 0x0ff00ff0, "fmacd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e000a00, 0x0fb00f50, "fmacs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0e200b10, 0x0ff00fff, "fmdhr%c\t%2z, %12-15r"},
  {FPU_VFP_EXT_V1, 0x0e000b10, 0x0ff00fff, "fmdlr%c\t%2z, %12-15r"},
  {FPU_VFP_EXT_V2, 0x0c400b10, 0x0ff00ff0, "fmdrr%c\t%0z, %12-15r, %16-19r"},
  {FPU_VFP_EXT_V1, 0x0e300b10, 0x0ff00fff, "fmrdh%c\t%12-15r, %2z"},
  {FPU_VFP_EXT_V1, 0x0e100b10, 0x0ff00fff, "fmrdl%c\t%12-15r, %2z"},
  {FPU_VFP_EXT_V1, 0x0c500b10, 0x0ff00ff0, "fmrrd%c\t%12-15r, %16-19r, %0z"},
  {FPU_VFP_EXT_V2, 0x0c500a10, 0x0ff00fd0, "fmrrs%c\t%12-15r, %16-19r, %4y"},
  {FPU_VFP_EXT_V1xD, 0x0e100a10, 0x0ff00f7f, "fmrs%c\t%12-15r, %2y"},
  {FPU_VFP_EXT_V1xD, 0x0ef1fa10, 0x0fffffff, "fmstat%c"},
  {FPU_VFP_EXT_V1xD, 0x0ef00a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpsid"},
  {FPU_VFP_EXT_V1xD, 0x0ef10a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpscr"},
  {FPU_VFP_EXT_V1xD, 0x0ef80a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpexc"},
  {FPU_VFP_EXT_V1xD, 0x0ef90a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpinst\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0efa0a10, 0x0fff0fff, "fmrx%c\t%12-15r, fpinst2\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0ef00a10, 0x0ff00fff, "fmrx%c\t%12-15r, <impl def 0x%16-19x>"},
  {FPU_VFP_EXT_V1, 0x0e100b00, 0x0ff00ff0, "fmscd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e100a00, 0x0fb00f50, "fmscs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1xD, 0x0e000a10, 0x0ff00f7f, "fmsr%c\t%2y, %12-15r"},
  {FPU_VFP_EXT_V2, 0x0c400a10, 0x0ff00fd0, "fmsrr%c\t%12-15r, %16-19r, %4y"},
  {FPU_VFP_EXT_V1, 0x0e200b00, 0x0ff00ff0, "fmuld%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e200a00, 0x0fb00f50, "fmuls%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1xD, 0x0ee00a10, 0x0fff0fff, "fmxr%c\tfpsid, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee10a10, 0x0fff0fff, "fmxr%c\tfpscr, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee80a10, 0x0fff0fff, "fmxr%c\tfpexc, %12-15r"},
  {FPU_VFP_EXT_V1xD, 0x0ee90a10, 0x0fff0fff, "fmxr%c\tfpinst, %12-15r\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0eea0a10, 0x0fff0fff, "fmxr%c\tfpinst2, %12-15r\t@ Impl def"},
  {FPU_VFP_EXT_V1xD, 0x0ee00a10, 0x0ff00fff, "fmxr%c\t<impl def 0x%16-19x>, %12-15r"},
  {FPU_VFP_EXT_V1, 0x0eb10b40, 0x0fff0ff0, "fnegd%c\t%1z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0eb10a40, 0x0fbf0fd0, "fnegs%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0e000b40, 0x0ff00ff0, "fnmacd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e000a40, 0x0fb00f50, "fnmacs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0e100b40, 0x0ff00ff0, "fnmscd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e100a40, 0x0fb00f50, "fnmscs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0e200b40, 0x0ff00ff0, "fnmuld%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e200a40, 0x0fb00f50, "fnmuls%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb80bc0, 0x0fff0fd0, "fsitod%c\t%1z, %0y"},
  {FPU_VFP_EXT_V1xD, 0x0eb80ac0, 0x0fbf0fd0, "fsitos%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb10bc0, 0x0fff0ff0, "fsqrtd%c\t%1z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0eb10ac0, 0x0fbf0fd0, "fsqrts%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0d000b00, 0x0f700f00, "fstd%c\t%1z, %A"},
  {FPU_VFP_EXT_V1xD, 0x0c800b00, 0x0fd00f00, "fstmia%0?xd%c\t%16-19r%21'!, %3z"},
  {FPU_VFP_EXT_V1xD, 0x0d200b00, 0x0ff00f00, "fstmdb%0?xd%c\t%16-19r!, %3z"},
  {FPU_VFP_EXT_V1xD, 0x0d000a00, 0x0f300f00, "fsts%c\t%1y, %A"},
  {FPU_VFP_EXT_V1xD, 0x0c800a00, 0x0f900f00, "fstmias%c\t%16-19r%21'!, %3y"},
  {FPU_VFP_EXT_V1xD, 0x0d200a00, 0x0fb00f00, "fstmdbs%c\t%16-19r!, %3y"},
  {FPU_VFP_EXT_V1, 0x0e300b40, 0x0ff00ff0, "fsubd%c\t%1z, %2z, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0e300a40, 0x0fb00f50, "fsubs%c\t%1y, %2y, %0y"},
  {FPU_VFP_EXT_V1, 0x0ebc0b40, 0x0fbe0f70, "fto%16?sui%7'zd%c\t%1y, %0z"},
  {FPU_VFP_EXT_V1xD, 0x0ebc0a40, 0x0fbe0f50, "fto%16?sui%7'zs%c\t%1y, %0y"},
  {FPU_VFP_EXT_V1, 0x0eb80b40, 0x0fff0fd0, "fuitod%c\t%1z, %0y"},
  {FPU_VFP_EXT_V1xD, 0x0eb80a40, 0x0fbf0fd0, "fuitos%c\t%1y, %0y"},

  /* Cirrus coprocessor instructions.  */
  {ARM_CEXT_MAVERICK, 0x0d100400, 0x0f500f00, "cfldrs%c\tmvf%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c100400, 0x0f500f00, "cfldrs%c\tmvf%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d500400, 0x0f500f00, "cfldrd%c\tmvd%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c500400, 0x0f500f00, "cfldrd%c\tmvd%12-15d, %A"}, 
  {ARM_CEXT_MAVERICK, 0x0d100500, 0x0f500f00, "cfldr32%c\tmvfx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c100500, 0x0f500f00, "cfldr32%c\tmvfx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d500500, 0x0f500f00, "cfldr64%c\tmvdx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c500500, 0x0f500f00, "cfldr64%c\tmvdx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d000400, 0x0f500f00, "cfstrs%c\tmvf%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c000400, 0x0f500f00, "cfstrs%c\tmvf%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d400400, 0x0f500f00, "cfstrd%c\tmvd%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c400400, 0x0f500f00, "cfstrd%c\tmvd%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d000500, 0x0f500f00, "cfstr32%c\tmvfx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c000500, 0x0f500f00, "cfstr32%c\tmvfx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0d400500, 0x0f500f00, "cfstr64%c\tmvdx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0c400500, 0x0f500f00, "cfstr64%c\tmvdx%12-15d, %A"},
  {ARM_CEXT_MAVERICK, 0x0e000450, 0x0ff00ff0, "cfmvsr%c\tmvf%16-19d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e100450, 0x0ff00ff0, "cfmvrs%c\t%12-15r, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000410, 0x0ff00ff0, "cfmvdlr%c\tmvd%16-19d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e100410, 0x0ff00ff0, "cfmvrdl%c\t%12-15r, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000430, 0x0ff00ff0, "cfmvdhr%c\tmvd%16-19d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e100430, 0x0ff00fff, "cfmvrdh%c\t%12-15r, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000510, 0x0ff00fff, "cfmv64lr%c\tmvdx%16-19d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e100510, 0x0ff00fff, "cfmvr64l%c\t%12-15r, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000530, 0x0ff00fff, "cfmv64hr%c\tmvdx%16-19d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e100530, 0x0ff00fff, "cfmvr64h%c\t%12-15r, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e200440, 0x0ff00fff, "cfmval32%c\tmvax%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e100440, 0x0ff00fff, "cfmv32al%c\tmvfx%12-15d, mvax%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e200460, 0x0ff00fff, "cfmvam32%c\tmvax%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e100460, 0x0ff00fff, "cfmv32am%c\tmvfx%12-15d, mvax%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e200480, 0x0ff00fff, "cfmvah32%c\tmvax%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e100480, 0x0ff00fff, "cfmv32ah%c\tmvfx%12-15d, mvax%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e2004a0, 0x0ff00fff, "cfmva32%c\tmvax%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e1004a0, 0x0ff00fff, "cfmv32a%c\tmvfx%12-15d, mvax%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e2004c0, 0x0ff00fff, "cfmva64%c\tmvax%12-15d, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e1004c0, 0x0ff00fff, "cfmv64a%c\tmvdx%12-15d, mvax%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e2004e0, 0x0fff0fff, "cfmvsc32%c\tdspsc, mvdx%12-15d"},
  {ARM_CEXT_MAVERICK, 0x0e1004e0, 0x0fff0fff, "cfmv32sc%c\tmvdx%12-15d, dspsc"},
  {ARM_CEXT_MAVERICK, 0x0e000400, 0x0ff00fff, "cfcpys%c\tmvf%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000420, 0x0ff00fff, "cfcpyd%c\tmvd%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000460, 0x0ff00fff, "cfcvtsd%c\tmvd%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000440, 0x0ff00fff, "cfcvtds%c\tmvf%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000480, 0x0ff00fff, "cfcvt32s%c\tmvf%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e0004a0, 0x0ff00fff, "cfcvt32d%c\tmvd%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e0004c0, 0x0ff00fff, "cfcvt64s%c\tmvf%12-15d, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e0004e0, 0x0ff00fff, "cfcvt64d%c\tmvd%12-15d, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e100580, 0x0ff00fff, "cfcvts32%c\tmvfx%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e1005a0, 0x0ff00fff, "cfcvtd32%c\tmvfx%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e1005c0, 0x0ff00fff, "cftruncs32%c\tmvfx%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e1005e0, 0x0ff00fff, "cftruncd32%c\tmvfx%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e000550, 0x0ff00ff0, "cfrshl32%c\tmvfx%16-19d, mvfx%0-3d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e000570, 0x0ff00ff0, "cfrshl64%c\tmvdx%16-19d, mvdx%0-3d, %12-15r"},
  {ARM_CEXT_MAVERICK, 0x0e000500, 0x0ff00f00, "cfsh32%c\tmvfx%12-15d, mvfx%16-19d, #%I"},
  {ARM_CEXT_MAVERICK, 0x0e200500, 0x0ff00f00, "cfsh64%c\tmvdx%12-15d, mvdx%16-19d, #%I"},
  {ARM_CEXT_MAVERICK, 0x0e100490, 0x0ff00ff0, "cfcmps%c\t%12-15r, mvf%16-19d, mvf%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e1004b0, 0x0ff00ff0, "cfcmpd%c\t%12-15r, mvd%16-19d, mvd%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100590, 0x0ff00ff0, "cfcmp32%c\t%12-15r, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e1005b0, 0x0ff00ff0, "cfcmp64%c\t%12-15r, mvdx%16-19d, mvdx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e300400, 0x0ff00fff, "cfabss%c\tmvf%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300420, 0x0ff00fff, "cfabsd%c\tmvd%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300440, 0x0ff00fff, "cfnegs%c\tmvf%12-15d, mvf%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300460, 0x0ff00fff, "cfnegd%c\tmvd%12-15d, mvd%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300480, 0x0ff00ff0, "cfadds%c\tmvf%12-15d, mvf%16-19d, mvf%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3004a0, 0x0ff00ff0, "cfaddd%c\tmvd%12-15d, mvd%16-19d, mvd%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3004c0, 0x0ff00ff0, "cfsubs%c\tmvf%12-15d, mvf%16-19d, mvf%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3004e0, 0x0ff00ff0, "cfsubd%c\tmvd%12-15d, mvd%16-19d, mvd%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100400, 0x0ff00ff0, "cfmuls%c\tmvf%12-15d, mvf%16-19d, mvf%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100420, 0x0ff00ff0, "cfmuld%c\tmvd%12-15d, mvd%16-19d, mvd%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e300500, 0x0ff00fff, "cfabs32%c\tmvfx%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300520, 0x0ff00fff, "cfabs64%c\tmvdx%12-15d, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300540, 0x0ff00fff, "cfneg32%c\tmvfx%12-15d, mvfx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300560, 0x0ff00fff, "cfneg64%c\tmvdx%12-15d, mvdx%16-19d"},
  {ARM_CEXT_MAVERICK, 0x0e300580, 0x0ff00ff0, "cfadd32%c\tmvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3005a0, 0x0ff00ff0, "cfadd64%c\tmvdx%12-15d, mvdx%16-19d, mvdx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3005c0, 0x0ff00ff0, "cfsub32%c\tmvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e3005e0, 0x0ff00ff0, "cfsub64%c\tmvdx%12-15d, mvdx%16-19d, mvdx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100500, 0x0ff00ff0, "cfmul32%c\tmvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100520, 0x0ff00ff0, "cfmul64%c\tmvdx%12-15d, mvdx%16-19d, mvdx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100540, 0x0ff00ff0, "cfmac32%c\tmvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100560, 0x0ff00ff0, "cfmsc32%c\tmvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e000600, 0x0ff00f00, "cfmadd32%c\tmvax%5-7d, mvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e100600, 0x0ff00f00, "cfmsub32%c\tmvax%5-7d, mvfx%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e200600, 0x0ff00f00, "cfmadda32%c\tmvax%5-7d, mvax%12-15d, mvfx%16-19d, mvfx%0-3d"},
  {ARM_CEXT_MAVERICK, 0x0e300600, 0x0ff00f00, "cfmsuba32%c\tmvax%5-7d, mvax%12-15d, mvfx%16-19d, mvfx%0-3d"},

  /* Generic coprocessor instructions */
  {ARM_EXT_V2, 0x0c400000, 0x0ff00000, "mcrr%c\t%8-11d, %4-7d, %12-15r, %16-19r, cr%0-3d"},
  {ARM_EXT_V2, 0x0c500000, 0x0ff00000, "mrrc%c\t%8-11d, %4-7d, %12-15r, %16-19r, cr%0-3d"},
  {ARM_EXT_V2, 0x0e000000, 0x0f000010, "cdp%c\t%8-11d, %20-23d, cr%12-15d, cr%16-19d, cr%0-3d, {%5-7d}"},
  {ARM_EXT_V2, 0x0e100010, 0x0f100010, "mrc%c\t%8-11d, %21-23d, %12-15r, cr%16-19d, cr%0-3d, {%5-7d}"},
  {ARM_EXT_V2, 0x0e000010, 0x0f100010, "mcr%c\t%8-11d, %21-23d, %12-15r, cr%16-19d, cr%0-3d, {%5-7d}"},
  {ARM_EXT_V2, 0x0c000000, 0x0e100000, "stc%c%22'l\t%8-11d, cr%12-15d, %A"},
  {ARM_EXT_V2, 0x0c100000, 0x0e100000, "ldc%c%22'l\t%8-11d, cr%12-15d, %A"},

  /* The rest.  */
  {ARM_EXT_V1, 0x00000000, 0x00000000, "undefined instruction %0-31x"},
  {0, 0x00000000, 0x00000000, 0}
};

static const struct thumb_opcode thumb_opcodes[] =
{
  /* Thumb instructions.  */

  /* ARM V6.  */
  {ARM_EXT_V6, 0xb660, 0xfff8, "cpsie\t%2'a%1'i%0'f"},
  {ARM_EXT_V6, 0xb670, 0xfff8, "cpsid\t%2'a%1'i%0'f"},
  {ARM_EXT_V6, 0x4600, 0xffc0, "cpy\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xba00, 0xffc0, "rev\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xba40, 0xffc0, "rev16\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xbac0, 0xffc0, "revsh\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xb650, 0xfff7, "setend\t%3?ble"},
  {ARM_EXT_V6, 0xb200, 0xffc0, "sxth\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xb240, 0xffc0, "sxtb\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xb280, 0xffc0, "uxth\t%0-2r, %3-5r"},
  {ARM_EXT_V6, 0xb2c0, 0xffc0, "uxtb\t%0-2r, %3-5r"},

  /* ARM V5 ISA extends Thumb.  */
  {ARM_EXT_V5T, 0xbe00, 0xff00, "bkpt\t%0-7x"},
  /* Note: this is BLX(2).  BLX(1) is done in arm-dis.c/print_insn_thumb()
     as an extension of the special processing there for Thumb BL.
     BL and BLX(1) involve 2 successive 16-bit instructions, which must
     always appear together in the correct order.  So, the empty
     string is put in this table, and the string interpreter takes <empty>
     to mean it has a pair of BL-ish instructions.  */
  {ARM_EXT_V5T, 0x4780, 0xff87, "blx\t%3-6r"},	/* note: 4 bit register number.  */
  /* ARM V4T ISA (Thumb v1).  */
  {ARM_EXT_V4T, 0x46C0, 0xFFFF, "nop\t\t\t(mov r8, r8)"},
  /* Format 5 instructions do not update the PSR.  */
  {ARM_EXT_V4T, 0x1C00, 0xFFC0, "mov\t%0-2r, %3-5r\t\t(add %0-2r, %3-5r, #%6-8d)"},
  /* Format 4.  */
  {ARM_EXT_V4T, 0x4000, 0xFFC0, "and\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4040, 0xFFC0, "eor\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4080, 0xFFC0, "lsl\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x40C0, 0xFFC0, "lsr\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4100, 0xFFC0, "asr\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4140, 0xFFC0, "adc\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4180, 0xFFC0, "sbc\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x41C0, 0xFFC0, "ror\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4200, 0xFFC0, "tst\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4240, 0xFFC0, "neg\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4280, 0xFFC0, "cmp\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x42C0, 0xFFC0, "cmn\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4300, 0xFFC0, "orr\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4340, 0xFFC0, "mul\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x4380, 0xFFC0, "bic\t%0-2r, %3-5r"},
  {ARM_EXT_V4T, 0x43C0, 0xFFC0, "mvn\t%0-2r, %3-5r"},
  /* format 13 */
  {ARM_EXT_V4T, 0xB000, 0xFF80, "add\tsp, #%0-6W"},
  {ARM_EXT_V4T, 0xB080, 0xFF80, "sub\tsp, #%0-6W"},
  /* format 5 */
  {ARM_EXT_V4T, 0x4700, 0xFF80, "bx\t%S"},
  {ARM_EXT_V4T, 0x4400, 0xFF00, "add\t%D, %S"},
  {ARM_EXT_V4T, 0x4500, 0xFF00, "cmp\t%D, %S"},
  {ARM_EXT_V4T, 0x4600, 0xFF00, "mov\t%D, %S"},
  /* format 14 */
  {ARM_EXT_V4T, 0xB400, 0xFE00, "push\t%N"},
  {ARM_EXT_V4T, 0xBC00, 0xFE00, "pop\t%O"},
  /* format 2 */
  {ARM_EXT_V4T, 0x1800, 0xFE00, "add\t%0-2r, %3-5r, %6-8r"},
  {ARM_EXT_V4T, 0x1A00, 0xFE00, "sub\t%0-2r, %3-5r, %6-8r"},
  {ARM_EXT_V4T, 0x1C00, 0xFE00, "add\t%0-2r, %3-5r, #%6-8d"},
  {ARM_EXT_V4T, 0x1E00, 0xFE00, "sub\t%0-2r, %3-5r, #%6-8d"},
  /* format 8 */
  {ARM_EXT_V4T, 0x5200, 0xFE00, "strh\t%0-2r, [%3-5r, %6-8r]"},
  {ARM_EXT_V4T, 0x5A00, 0xFE00, "ldrh\t%0-2r, [%3-5r, %6-8r]"},
  {ARM_EXT_V4T, 0x5600, 0xF600, "ldrs%11?hb\t%0-2r, [%3-5r, %6-8r]"},
  /* format 7 */
  {ARM_EXT_V4T, 0x5000, 0xFA00, "str%10'b\t%0-2r, [%3-5r, %6-8r]"},
  {ARM_EXT_V4T, 0x5800, 0xFA00, "ldr%10'b\t%0-2r, [%3-5r, %6-8r]"},
  /* format 1 */
  {ARM_EXT_V4T, 0x0000, 0xF800, "lsl\t%0-2r, %3-5r, #%6-10d"},
  {ARM_EXT_V4T, 0x0800, 0xF800, "lsr\t%0-2r, %3-5r, #%6-10d"},
  {ARM_EXT_V4T, 0x1000, 0xF800, "asr\t%0-2r, %3-5r, #%6-10d"},
  /* format 3 */
  {ARM_EXT_V4T, 0x2000, 0xF800, "mov\t%8-10r, #%0-7d"},
  {ARM_EXT_V4T, 0x2800, 0xF800, "cmp\t%8-10r, #%0-7d"},
  {ARM_EXT_V4T, 0x3000, 0xF800, "add\t%8-10r, #%0-7d"},
  {ARM_EXT_V4T, 0x3800, 0xF800, "sub\t%8-10r, #%0-7d"},
  /* format 6 */
  {ARM_EXT_V4T, 0x4800, 0xF800, "ldr\t%8-10r, [pc, #%0-7W]\t(%0-7a)"},  /* TODO: Disassemble PC relative "LDR rD,=<symbolic>" */
  /* format 9 */
  {ARM_EXT_V4T, 0x6000, 0xF800, "str\t%0-2r, [%3-5r, #%6-10W]"},
  {ARM_EXT_V4T, 0x6800, 0xF800, "ldr\t%0-2r, [%3-5r, #%6-10W]"},
  {ARM_EXT_V4T, 0x7000, 0xF800, "strb\t%0-2r, [%3-5r, #%6-10d]"},
  {ARM_EXT_V4T, 0x7800, 0xF800, "ldrb\t%0-2r, [%3-5r, #%6-10d]"},
  /* format 10 */
  {ARM_EXT_V4T, 0x8000, 0xF800, "strh\t%0-2r, [%3-5r, #%6-10H]"},
  {ARM_EXT_V4T, 0x8800, 0xF800, "ldrh\t%0-2r, [%3-5r, #%6-10H]"},
  /* format 11 */
  {ARM_EXT_V4T, 0x9000, 0xF800, "str\t%8-10r, [sp, #%0-7W]"},
  {ARM_EXT_V4T, 0x9800, 0xF800, "ldr\t%8-10r, [sp, #%0-7W]"},
  /* format 12 */
  {ARM_EXT_V4T, 0xA000, 0xF800, "add\t%8-10r, pc, #%0-7W\t(adr %8-10r,%0-7a)"},
  {ARM_EXT_V4T, 0xA800, 0xF800, "add\t%8-10r, sp, #%0-7W"},
  /* format 15 */
  {ARM_EXT_V4T, 0xC000, 0xF800, "stmia\t%8-10r!,%M"},
  {ARM_EXT_V4T, 0xC800, 0xF800, "ldmia\t%8-10r!,%M"},
  /* format 18 */
  {ARM_EXT_V4T, 0xE000, 0xF800, "b\t%0-10B"},
  {ARM_EXT_V4T, 0xE800, 0xF800, "undefined"},
  /* format 19 */
  {ARM_EXT_V4T, 0xF000, 0xF800, ""}, /* special processing required in disassembler */
  {ARM_EXT_V4T, 0xF800, 0xF800, "second half of BL instruction %0-15x"},
  /* format 16 */
  {ARM_EXT_V4T, 0xD000, 0xFF00, "beq\t%0-7B"},
  {ARM_EXT_V4T, 0xD100, 0xFF00, "bne\t%0-7B"},
  {ARM_EXT_V4T, 0xD200, 0xFF00, "bcs\t%0-7B"},
  {ARM_EXT_V4T, 0xD300, 0xFF00, "bcc\t%0-7B"},
  {ARM_EXT_V4T, 0xD400, 0xFF00, "bmi\t%0-7B"},
  {ARM_EXT_V4T, 0xD500, 0xFF00, "bpl\t%0-7B"},
  {ARM_EXT_V4T, 0xD600, 0xFF00, "bvs\t%0-7B"},
  {ARM_EXT_V4T, 0xD700, 0xFF00, "bvc\t%0-7B"},
  {ARM_EXT_V4T, 0xD800, 0xFF00, "bhi\t%0-7B"},
  {ARM_EXT_V4T, 0xD900, 0xFF00, "bls\t%0-7B"},
  {ARM_EXT_V4T, 0xDA00, 0xFF00, "bge\t%0-7B"},
  {ARM_EXT_V4T, 0xDB00, 0xFF00, "blt\t%0-7B"},
  {ARM_EXT_V4T, 0xDC00, 0xFF00, "bgt\t%0-7B"},
  {ARM_EXT_V4T, 0xDD00, 0xFF00, "ble\t%0-7B"},
  /* format 17 */
  {ARM_EXT_V4T, 0xDE00, 0xFF00, "bal\t%0-7B"},
  {ARM_EXT_V4T, 0xDF00, 0xFF00, "swi\t%0-7d"},
  /* format 9 */
  {ARM_EXT_V4T, 0x6000, 0xF800, "str\t%0-2r, [%3-5r, #%6-10W]"},
  {ARM_EXT_V4T, 0x6800, 0xF800, "ldr\t%0-2r, [%3-5r, #%6-10W]"},
  {ARM_EXT_V4T, 0x7000, 0xF800, "strb\t%0-2r, [%3-5r, #%6-10d]"},
  {ARM_EXT_V4T, 0x7800, 0xF800, "ldrb\t%0-2r, [%3-5r, #%6-10d]"},
  /* the rest */
  {ARM_EXT_V1, 0x0000, 0x0000, "undefined instruction %0-15x"},
  {0, 0x0000, 0x0000, 0}
};

static char * arm_conditional[] =
{"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
 "hi", "ls", "ge", "lt", "gt", "le", "", "nv"};

typedef struct
{
  const char * name;
  const char * description;
  const char * reg_names[16];
}
arm_regname;

static arm_regname regnames[] =
{
  { "raw" , "Select raw register names",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"}},
  { "gcc",  "Select register names used by GCC",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "sl",  "fp",  "ip",  "sp",  "lr",  "pc" }},
  { "std",  "Select register names used in ARM's ISA documentation",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp",  "lr",  "pc" }},
  { "apcs", "Select register names used in the APCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "v4", "v5", "v6", "sl",  "fp",  "ip",  "sp",  "lr",  "pc" }},
  { "atpcs", "Select register names used in the ATPCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "v4", "v5", "v6", "v7",  "v8",  "IP",  "SP",  "LR",  "PC" }},
  { "special-atpcs", "Select special register names used in the ATPCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "WR", "v5", "SB", "SL",  "FP",  "IP",  "SP",  "LR",  "PC" }},
  { "iwmmxt_regnames", "Select register names used on the Intel Wireless MMX technology coprocessor",
    { "wr0", "wr1", "wr2", "wr3", "wr4", "wr5", "wr6", "wr7", "wr8", "wr9", "wr10", "wr11", "wr12", "wr13", "wr14", "wr15"}},
  { "iwmmxt_Cregnames", "Select control register names used on the Intel Wireless MMX technology coprocessor",
    {"wcid", "wcon", "wcssf", "wcasf", "reserved", "reserved", "reserved", "reserved", "wcgr0", "wcgr1", "wcgr2", "wcgr3", "reserved", "reserved", "reserved", "reserved"}}
};

static char * iwmmxt_wwnames[] =
{"b", "h", "w", "d"};

static char * iwmmxt_wwssnames[] =
{"b", "bus", "b", "bss",
 "h", "hus", "h", "hss",
 "w", "wus", "w", "wss",
 "d", "dus", "d", "dss"
};

/* Default to GCC register name set.  */
static unsigned int regname_selected = 1;

#define NUM_ARM_REGNAMES  NUM_ELEM (regnames)
#define arm_regnames      regnames[regname_selected].reg_names

static bfd_boolean force_thumb = FALSE;

static char * arm_fp_const[] =
{"0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "0.5", "10.0"};

static char * arm_shift[] =
{"lsl", "lsr", "asr", "ror"};

/* Forward declarations.  */
static void arm_decode_shift
  PARAMS ((long, fprintf_ftype, void *));
static int  print_insn_arm
  PARAMS ((bfd_vma, struct disassemble_info *, long));
static int  print_insn_thumb
  PARAMS ((bfd_vma, struct disassemble_info *, long));
static void parse_disassembler_options
  PARAMS ((char *));
static int  print_insn
  PARAMS ((bfd_vma, struct disassemble_info *, bfd_boolean));
static int set_iwmmxt_regnames
  PARAMS ((void));

int get_arm_regname_num_options
  PARAMS ((void));
int set_arm_regname_option
  PARAMS ((int));
int get_arm_regnames
  PARAMS ((int, const char **, const char **, const char ***));

/* Functions.  */
int
get_arm_regname_num_options ()
{
  return NUM_ARM_REGNAMES;
}

int
set_arm_regname_option (option)
     int option;
{
  int old = regname_selected;
  regname_selected = option;
  return old;
}

int
get_arm_regnames (option, setname, setdescription, register_names)
     int option;
     const char **setname;
     const char **setdescription;
     const char ***register_names;
{
  *setname = regnames[option].name;
  *setdescription = regnames[option].description;
  *register_names = regnames[option].reg_names;
  return 16;
}

static void
arm_decode_shift (given, func, stream)
     long given;
     fprintf_ftype func;
     void * stream;
{
  func (stream, "%s", arm_regnames[given & 0xf]);

  if ((given & 0xff0) != 0)
    {
      if ((given & 0x10) == 0)
	{
	  int amount = (given & 0xf80) >> 7;
	  int shift = (given & 0x60) >> 5;

	  if (amount == 0)
	    {
	      if (shift == 3)
		{
		  func (stream, ", rrx");
		  return;
		}

	      amount = 32;
	    }

	  func (stream, ", %s #%d", arm_shift[shift], amount);
	}
      else
	func (stream, ", %s %s", arm_shift[(given & 0x60) >> 5],
	      arm_regnames[(given & 0xf00) >> 8]);
    }
}

static int
set_iwmmxt_regnames ()
{
  const char * setname;
  const char * setdesc;
  const char ** regnames;
  int iwmmxt_regnames = 0;
  int num_regnames = get_arm_regname_num_options ();

  get_arm_regnames (iwmmxt_regnames, &setname,
		    &setdesc, &regnames);
  while ((strcmp ("iwmmxt_regnames", setname))
	 && (iwmmxt_regnames < num_regnames))
    get_arm_regnames (++iwmmxt_regnames, &setname, &setdesc, &regnames);

  return iwmmxt_regnames;
}
			  
/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction (always 4 on ARM). */

static int
print_insn_arm (pc, info, given)
     bfd_vma pc;
     struct disassemble_info *info;
     long given;
{
  const struct arm_opcode *insn;
  void *stream = info->stream;
  fprintf_ftype func   = info->fprintf_func;
  static int iwmmxt_regnames = 0;

  for (insn = arm_opcodes; insn->assembler; insn++)
    {
      if (insn->value == FIRST_IWMMXT_INSN
	  && info->mach != bfd_mach_arm_XScale
	  && info->mach != bfd_mach_arm_iWMMXt)
	insn = insn + IWMMXT_INSN_COUNT;

      if ((given & insn->mask) == insn->value)
	{
	  char * c;

	  for (c = insn->assembler; *c; c++)
	    {
	      if (*c == '%')
		{
		  switch (*++c)
		    {
		    case '%':
		      func (stream, "%%");
		      break;

		    case 'a':
		      if (((given & 0x000f0000) == 0x000f0000)
			  && ((given & 0x02000000) == 0))
			{
			  int offset = given & 0xfff;

			  func (stream, "[pc");

			  if (given & 0x01000000)
			    {
			      if ((given & 0x00800000) == 0)
				offset = - offset;

			      /* Pre-indexed.  */
			      func (stream, ", #%d]", offset);

			      offset += pc + 8;

			      /* Cope with the possibility of write-back
				 being used.  Probably a very dangerous thing
				 for the programmer to do, but who are we to
				 argue ?  */
			      if (given & 0x00200000)
				func (stream, "!");
			    }
			  else
			    {
			      /* Post indexed.  */
			      func (stream, "], #%d", offset);

			      /* ie ignore the offset.  */
			      offset = pc + 8;
			    }

			  func (stream, "\t; ");
			  info->print_address_func (offset, info);
			}
		      else
			{
			  func (stream, "[%s",
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, ", #%s%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
				  func (stream, ", %s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}

			      func (stream, "]%s",
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, "], #%s%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else
				    func (stream, "]");
				}
			      else
				{
				  func (stream, "], %s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}
			    }
			}
		      break;

		    case 's':
                      if ((given & 0x004f0000) == 0x004f0000)
			{
                          /* PC relative with immediate offset.  */
			  int offset = ((given & 0xf00) >> 4) | (given & 0xf);

			  if ((given & 0x00800000) == 0)
			    offset = -offset;

			  func (stream, "[pc, #%d]\t; ", offset);

			  (*info->print_address_func)
			    (offset + pc + 8, info);
			}
		      else
			{
			  func (stream, "[%s",
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
                              /* Pre-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, ", #%s%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
                                  /* Register.  */
				  func (stream, ", %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}

			      func (stream, "]%s",
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
                              /* Post-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, "], #%s%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else
				    func (stream, "]");
				}
			      else
				{
                                  /* Register.  */
				  func (stream, "], %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}
			    }
			}
		      break;

		    case 'b':
		      (*info->print_address_func)
			(BDISP (given) * 4 + pc + 8, info);
		      break;

		    case 'c':
		      func (stream, "%s",
			    arm_conditional [(given >> 28) & 0xf]);
		      break;

		    case 'm':
		      {
			int started = 0;
			int reg;

			func (stream, "{");
			for (reg = 0; reg < 16; reg++)
			  if ((given & (1 << reg)) != 0)
			    {
			      if (started)
				func (stream, ", ");
			      started = 1;
			      func (stream, "%s", arm_regnames[reg]);
			    }
			func (stream, "}");
		      }
		      break;

		    case 'o':
		      if ((given & 0x02000000) != 0)
			{
			  int rotate = (given & 0xf00) >> 7;
			  int immed = (given & 0xff);
			  immed = (((immed << (32 - rotate))
				    | (immed >> rotate)) & 0xffffffff);
			  func (stream, "#%d\t; 0x%x", immed, immed);
			}
		      else
			arm_decode_shift (given, func, stream);
		      break;

		    case 'p':
		      if ((given & 0x0000f000) == 0x0000f000)
			func (stream, "p");
		      break;

		    case 't':
		      if ((given & 0x01200000) == 0x00200000)
			func (stream, "t");
		      break;

		    case 'A':
		      func (stream, "[%s", arm_regnames [(given >> 16) & 0xf]);

		      if ((given & (1 << 24)) != 0)
			{
			  int offset = given & 0xff;

			  if (offset)
			    func (stream, ", #%s%d]%s",
				  ((given & 0x00800000) == 0 ? "-" : ""),
				  offset * 4,
				  ((given & 0x00200000) != 0 ? "!" : ""));
			  else
			    func (stream, "]");
			}
		      else
			{
			  int offset = given & 0xff;

			  func (stream, "]");

			  if (given & (1 << 21))
			    {
			      if (offset)
				func (stream, ", #%s%d",
				      ((given & 0x00800000) == 0 ? "-" : ""),
				      offset * 4);
			    }
			  else
			    func (stream, ", {%d}", offset);
			}
		      break;

		    case 'B':
		      /* Print ARM V5 BLX(1) address: pc+25 bits.  */
		      {
			bfd_vma address;
			bfd_vma offset = 0;

			if (given & 0x00800000)
			  /* Is signed, hi bits should be ones.  */
			  offset = (-1) ^ 0x00ffffff;

			/* Offset is (SignExtend(offset field)<<2).  */
			offset += given & 0x00ffffff;
			offset <<= 2;
			address = offset + pc + 8;

			if (given & 0x01000000)
			  /* H bit allows addressing to 2-byte boundaries.  */
			  address += 2;

		        info->print_address_func (address, info);
		      }
		      break;

		    case 'I':
		      /* Print a Cirrus/DSP shift immediate.  */
		      /* Immediates are 7bit signed ints with bits 0..3 in
			 bits 0..3 of opcode and bits 4..6 in bits 5..7
			 of opcode.  */
		      {
			int imm;

			imm = (given & 0xf) | ((given & 0xe0) >> 1);

			/* Is ``imm'' a negative number?  */
			if (imm & 0x40)
			  imm |= (-1 << 7);

			func (stream, "%d", imm);
		      }

		      break;

		    case 'C':
		      func (stream, "_");
		      if (given & 0x80000)
			func (stream, "f");
		      if (given & 0x40000)
			func (stream, "s");
		      if (given & 0x20000)
			func (stream, "x");
		      if (given & 0x10000)
			func (stream, "c");
		      break;

		    case 'E':
		      {
			long msb = (given >> 16) & 0x1f;
			long lsb = (given >> 7) & 0x1f;
			long width = msb - lsb + 1;

			if (width > 0)
			  func(stream, "#%lu, #%lu", lsb, width);
			else
			  func(stream, "(invalid %lu:%lu)", lsb, msb);
		      }
		      break;

		    case 'F':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "4");
			  break;
			case 0x8000:
			  func (stream, "1");
			  break;
			case 0x00400000:
			  func (stream, "2");
			  break;
			default:
			  func (stream, "3");
			}
		      break;

		    case 'P':
		      switch (given & 0x00080080)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x80:
			  func (stream, "d");
			  break;
			case 0x00080000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, _("<illegal precision>"));
			  break;
			}
		      break;
		    case 'Q':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x8000:
			  func (stream, "d");
			  break;
			case 0x00400000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, "p");
			  break;
			}
		      break;
		    case 'R':
		      switch (given & 0x60)
			{
			case 0:
			  break;
			case 0x20:
			  func (stream, "p");
			  break;
			case 0x40:
			  func (stream, "m");
			  break;
			default:
			  func (stream, "z");
			  break;
			}
		      break;

		    case '0': case '1': case '2': case '3': case '4':
		    case '5': case '6': case '7': case '8': case '9':
		      {
			int bitstart = *c++ - '0';
			int bitend = 0;
			while (*c >= '0' && *c <= '9')
			  bitstart = (bitstart * 10) + *c++ - '0';

			switch (*c)
			  {
			  case '-':
			    c++;

			    while (*c >= '0' && *c <= '9')
			      bitend = (bitend * 10) + *c++ - '0';

			    if (!bitend)
			      abort ();

			    switch (*c)
			      {
			      case 'r':
				{
				  long reg;

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;

				  func (stream, "%s", arm_regnames[reg]);
				}
				break;
			      case 'd':
				{
				  long reg;

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;

				  func (stream, "%d", reg);
				}
				break;
			      case 'W':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%d", reg + 1);
				}
				break;
			      case 'x':
				{
				  long reg;

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;

				  func (stream, "0x%08x", reg);

				  /* Some SWI instructions have special
				     meanings.  */
				  if ((given & 0x0fffffff) == 0x0FF00000)
				    func (stream, "\t; IMB");
				  else if ((given & 0x0fffffff) == 0x0FF00001)
				    func (stream, "\t; IMBRange");
				}
				break;
			      case 'X':
				{
				  long reg;

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;

				  func (stream, "%01x", reg & 0xf);
				}
				break;
			      case 'f':
				{
				  long reg;

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;

				  if (reg > 7)
				    func (stream, "#%s",
					  arm_fp_const[reg & 7]);
				  else
				    func (stream, "f%d", reg);
				}
				break;

			      case 'w':
				{
				  long reg;

				  if (bitstart != bitend)
				    {
				      reg = given >> bitstart;
				      reg &= (2 << (bitend - bitstart)) - 1;
				      if (bitend - bitstart == 1)
					func (stream, "%s", iwmmxt_wwnames[reg]);
				      else
					func (stream, "%s", iwmmxt_wwssnames[reg]);
				    }
				  else
				    {
				      reg = (((given >> 8)  & 0x1) |
					     ((given >> 22) & 0x1));
				      func (stream, "%s", iwmmxt_wwnames[reg]);
				    }
				}
				break;

			      case 'g':
				{
				  long reg;
				  int current_regnames;

				  if (! iwmmxt_regnames)
				    iwmmxt_regnames = set_iwmmxt_regnames ();
				  current_regnames = set_arm_regname_option
				    (iwmmxt_regnames);

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  func (stream, "%s", arm_regnames[reg]);
				  set_arm_regname_option (current_regnames);
				}
				break;

			      case 'G':
				{
				  long reg;
				  int current_regnames;

				  if (! iwmmxt_regnames)
				    iwmmxt_regnames = set_iwmmxt_regnames ();
				  current_regnames = set_arm_regname_option
				    (iwmmxt_regnames + 1);

				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  func (stream, "%s", arm_regnames[reg]);
				  set_arm_regname_option (current_regnames);
				}
				break;

			      default:
				abort ();
			      }
			    break;

			  case 'y':
			  case 'z':
			    {
			      int single = *c == 'y';
			      int regno;

			      switch (bitstart)
				{
				case 4: /* Sm pair */
				  func (stream, "{");
				  /* Fall through.  */
				case 0: /* Sm, Dm */
				  regno = given & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 5) & 1;
				    }
				  break;

				case 1: /* Sd, Dd */
				  regno = (given >> 12) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 22) & 1;
				    }
				  break;

				case 2: /* Sn, Dn */
				  regno = (given >> 16) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 7) & 1;
				    }
				  break;

				case 3: /* List */
				  func (stream, "{");
				  regno = (given >> 12) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 22) & 1;
				    }
				  break;


				default:
				  abort ();
				}

			      func (stream, "%c%d", single ? 's' : 'd', regno);

			      if (bitstart == 3)
				{
				  int count = given & 0xff;

				  if (single == 0)
				    count >>= 1;

				  if (--count)
				    {
				      func (stream, "-%c%d",
					    single ? 's' : 'd',
					    regno + count);
				    }

				  func (stream, "}");
				}
			      else if (bitstart == 4)
				func (stream, ", %c%d}", single ? 's' : 'd',
				      regno + 1);

			      break;
			    }

			  case '`':
			    c++;
			    if ((given & (1 << bitstart)) == 0)
			      func (stream, "%c", *c);
			    break;
			  case '\'':
			    c++;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c);
			    break;
			  case '?':
			    ++c;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c++);
			    else
			      func (stream, "%c", *++c);
			    break;
			  default:
			    abort ();
			  }
			break;

		      case 'L':
			switch (given & 0x00400100)
			  {
			  case 0x00000000: func (stream, "b"); break;
			  case 0x00400000: func (stream, "h"); break;
			  case 0x00000100: func (stream, "w"); break;
			  case 0x00400100: func (stream, "d"); break;
			  default:
			    break;
			  }
			break;

		      case 'Z':
			{
			  int value;
			  /* given (20, 23) | given (0, 3) */
			  value = ((given >> 16) & 0xf0) | (given & 0xf);
			  func (stream, "%d", value);
			}
			break;

		      case 'l':
			/* This is like the 'A' operator, except that if
			   the width field "M" is zero, then the offset is
			   *not* multiplied by four.  */
			{
			  int offset = given & 0xff;
			  int multiplier = (given & 0x00000100) ? 4 : 1;

			  func (stream, "[%s", arm_regnames [(given >> 16) & 0xf]);

			  if (offset)
			    {
			      if ((given & 0x01000000) != 0)
				func (stream, ", #%s%d]%s",
				      ((given & 0x00800000) == 0 ? "-" : ""),
				      offset * multiplier,
				      ((given & 0x00200000) != 0 ? "!" : ""));
			      else
				func (stream, "], #%s%d",
				      ((given & 0x00800000) == 0 ? "-" : ""),
				      offset * multiplier);
			    }
			  else
			    func (stream, "]");
			}
			break;

		      case 'e':
			{
			  int imm;

			  imm = (given & 0xf) | ((given & 0xfff00) >> 4);
			  func (stream, "%d", imm);
			}
			break;

		      default:
			abort ();
		      }
		    }
		}
	      else
		func (stream, "%c", *c);
	    }
	  return 4;
	}
    }
  abort ();
}

/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction. */

static int
print_insn_thumb (pc, info, given)
     bfd_vma pc;
     struct disassemble_info *info;
     long given;
{
  const struct thumb_opcode *insn;
  void *stream = info->stream;
  fprintf_ftype func = info->fprintf_func;

  for (insn = thumb_opcodes; insn->assembler; insn++)
    {
      if ((given & insn->mask) == insn->value)
        {
          char * c = insn->assembler;

          /* Special processing for Thumb 2 instruction BL sequence:  */
          if (!*c) /* Check for empty (not NULL) assembler string.  */
            {
	      long offset;

	      info->bytes_per_chunk = 4;
	      info->bytes_per_line  = 4;

	      offset = BDISP23 (given);
	      offset = offset * 2 + pc + 4;

	      if ((given & 0x10000000) == 0)
		{
		  func (stream, "blx\t");
		  offset &= 0xfffffffc;
		}
	      else
		func (stream, "bl\t");

	      info->print_address_func (offset, info);
              return 4;
            }
          else
            {
	      info->bytes_per_chunk = 2;
	      info->bytes_per_line  = 4;

              given &= 0xffff;

              for (; *c; c++)
                {
                  if (*c == '%')
                    {
                      int domaskpc = 0;
                      int domasklr = 0;

                      switch (*++c)
                        {
                        case '%':
                          func (stream, "%%");
                          break;

                        case 'S':
                          {
                            long reg;

                            reg = (given >> 3) & 0x7;
                            if (given & (1 << 6))
                              reg += 8;

                            func (stream, "%s", arm_regnames[reg]);
                          }
                          break;

                        case 'D':
                          {
                            long reg;

                            reg = given & 0x7;
                            if (given & (1 << 7))
                             reg += 8;

                            func (stream, "%s", arm_regnames[reg]);
                          }
                          break;

                        case 'T':
                          func (stream, "%s",
                                arm_conditional [(given >> 8) & 0xf]);
                          break;

                        case 'N':
                          if (given & (1 << 8))
                            domasklr = 1;
                          /* Fall through.  */
                        case 'O':
                          if (*c == 'O' && (given & (1 << 8)))
                            domaskpc = 1;
                          /* Fall through.  */
                        case 'M':
                          {
                            int started = 0;
                            int reg;

                            func (stream, "{");

                            /* It would be nice if we could spot
                               ranges, and generate the rS-rE format: */
                            for (reg = 0; (reg < 8); reg++)
                              if ((given & (1 << reg)) != 0)
                                {
                                  if (started)
                                    func (stream, ", ");
                                  started = 1;
                                  func (stream, "%s", arm_regnames[reg]);
                                }

                            if (domasklr)
                              {
                                if (started)
                                  func (stream, ", ");
                                started = 1;
                                func (stream, arm_regnames[14] /* "lr" */);
                              }

                            if (domaskpc)
                              {
                                if (started)
                                  func (stream, ", ");
                                func (stream, arm_regnames[15] /* "pc" */);
                              }

                            func (stream, "}");
                          }
                          break;


                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                          {
                            int bitstart = *c++ - '0';
                            int bitend = 0;

                            while (*c >= '0' && *c <= '9')
                              bitstart = (bitstart * 10) + *c++ - '0';

                            switch (*c)
                              {
                              case '-':
                                {
                                  long reg;

                                  c++;
                                  while (*c >= '0' && *c <= '9')
                                    bitend = (bitend * 10) + *c++ - '0';
                                  if (!bitend)
                                    abort ();
                                  reg = given >> bitstart;
                                  reg &= (2 << (bitend - bitstart)) - 1;
                                  switch (*c)
                                    {
                                    case 'r':
                                      func (stream, "%s", arm_regnames[reg]);
                                      break;

                                    case 'd':
                                      func (stream, "%d", reg);
                                      break;

                                    case 'H':
                                      func (stream, "%d", reg << 1);
                                      break;

                                    case 'W':
                                      func (stream, "%d", reg << 2);
                                      break;

                                    case 'a':
				      /* PC-relative address -- the bottom two
					 bits of the address are dropped
					 before the calculation.  */
                                      info->print_address_func
					(((pc + 4) & ~3) + (reg << 2), info);
                                      break;

                                    case 'x':
                                      func (stream, "0x%04x", reg);
                                      break;

                                    case 'I':
                                      reg = ((reg ^ (1 << bitend)) - (1 << bitend));
                                      func (stream, "%d", reg);
                                      break;

                                    case 'B':
                                      reg = ((reg ^ (1 << bitend)) - (1 << bitend));
                                      (*info->print_address_func)
                                        (reg * 2 + pc + 4, info);
                                      break;

                                    default:
                                      abort ();
                                    }
                                }
                                break;

                              case '\'':
                                c++;
                                if ((given & (1 << bitstart)) != 0)
                                  func (stream, "%c", *c);
                                break;

                              case '?':
                                ++c;
                                if ((given & (1 << bitstart)) != 0)
                                  func (stream, "%c", *c++);
                                else
                                  func (stream, "%c", *++c);
                                break;

                              default:
                                 abort ();
                              }
                          }
                          break;

                        default:
                          abort ();
                        }
                    }
                  else
                    func (stream, "%c", *c);
                }
             }
          return 2;
       }
    }

  /* No match.  */
  abort ();
}

/* Disallow mapping symbols ($a, $b, $d, $t etc) from
   being displayed in symbol relative addresses.  */

bfd_boolean
arm_symbol_is_valid (asymbol * sym,
		     struct disassemble_info * info ATTRIBUTE_UNUSED)
{
  const char * name;
  
  if (sym == NULL)
    return FALSE;

  name = bfd_asymbol_name (sym);

  return (name && *name != '$');
}

/* Parse an individual disassembler option.  */

void
parse_arm_disassembler_option (option)
     char * option;
{
  if (option == NULL)
    return;

  if (strneq (option, "reg-names-", 10))
    {
      int i;

      option += 10;

      for (i = NUM_ARM_REGNAMES; i--;)
	if (strneq (option, regnames[i].name, strlen (regnames[i].name)))
	  {
	    regname_selected = i;
	    break;
	  }

      if (i < 0)
	/* XXX - should break 'option' at following delimiter.  */
	fprintf (stderr, _("Unrecognised register name set: %s\n"), option);
    }
  else if (strneq (option, "force-thumb", 11))
    force_thumb = 1;
  else if (strneq (option, "no-force-thumb", 14))
    force_thumb = 0;
  else
    /* XXX - should break 'option' at following delimiter.  */
    fprintf (stderr, _("Unrecognised disassembler option: %s\n"), option);

  return;
}

/* Parse the string of disassembler options, spliting it at whitespaces
   or commas.  (Whitespace separators supported for backwards compatibility).  */

static void
parse_disassembler_options (options)
     char * options;
{
  if (options == NULL)
    return;

  while (*options)
    {
      parse_arm_disassembler_option (options);

      /* Skip forward to next seperator.  */
      while ((*options) && (! ISSPACE (*options)) && (*options != ','))
	++ options;
      /* Skip forward past seperators.  */
      while (ISSPACE (*options) || (*options == ','))
	++ options;      
    }
}

/* NOTE: There are no checks in these routines that
   the relevant number of data bytes exist.  */

static int
print_insn (pc, info, little)
     bfd_vma pc;
     struct disassemble_info * info;
     bfd_boolean little;
{
  unsigned char      b[4];
  long               given;
  int                status;
  int                is_thumb, second_half_valid = 1;

  if (info->disassembler_options)
    {
      parse_disassembler_options (info->disassembler_options);

      /* To avoid repeated parsing of these options, we remove them here.  */
      info->disassembler_options = NULL;
    }

  is_thumb = force_thumb;

  if (!is_thumb && info->symbols != NULL)
    {
      if (bfd_asymbol_flavour (*info->symbols) == bfd_target_coff_flavour)
	{
	  coff_symbol_type * cs;

	  cs = coffsymbol (*info->symbols);
	  is_thumb = (   cs->native->u.syment.n_sclass == C_THUMBEXT
		      || cs->native->u.syment.n_sclass == C_THUMBSTAT
		      || cs->native->u.syment.n_sclass == C_THUMBLABEL
		      || cs->native->u.syment.n_sclass == C_THUMBEXTFUNC
		      || cs->native->u.syment.n_sclass == C_THUMBSTATFUNC);
	}
      else if (bfd_asymbol_flavour (*info->symbols) == bfd_target_elf_flavour)
	{
	  elf_symbol_type *  es;
	  unsigned int       type;

	  es = *(elf_symbol_type **)(info->symbols);
	  type = ELF_ST_TYPE (es->internal_elf_sym.st_info);

	  is_thumb = (type == STT_ARM_TFUNC) || (type == STT_ARM_16BIT);
	}
    }

  info->bytes_per_chunk = 4;
  info->display_endian  = little ? BFD_ENDIAN_LITTLE : BFD_ENDIAN_BIG;

  if (little)
    {
      status = info->read_memory_func (pc, (bfd_byte *) &b[0], 4, info);
      if (status != 0 && is_thumb)
	{
	  info->bytes_per_chunk = 2;
	  second_half_valid = 0;

	  status = info->read_memory_func (pc, (bfd_byte *) b, 2, info);
	  b[3] = b[2] = 0;
	}

      if (status != 0)
	{
	  info->memory_error_func (status, pc, info);
	  return -1;
	}

      given = (b[0]) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
    }
  else
    {
      status = info->read_memory_func
	(WORD_ADDRESS (pc), (bfd_byte *) &b[0], 4, info);
      if (status != 0)
	{
	  info->memory_error_func (status, WORD_ADDRESS (pc), info);
	  return -1;
	}

      if (is_thumb)
	{
	  if (pc & 0x2)
	    {
	      given = (b[2] << 8) | b[3];

	      status = info->read_memory_func
		(WORD_ADDRESS (pc + 4), (bfd_byte *) b, 4, info);
	      if (status != 0)
		second_half_valid = 0;
	      else
		given |= (b[0] << 24) | (b[1] << 16);
	    }
	  else
	    given = (b[0] << 8) | b[1] | (b[2] << 24) | (b[3] << 16);
	}
      else
	given = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3]);
    }

  if (info->flags & INSN_HAS_RELOC)
    /* If the instruction has a reloc associated with it, then
       the offset field in the instruction will actually be the
       addend for the reloc.  (We are using REL type relocs).
       In such cases, we can ignore the pc when computing
       addresses, since the addend is not currently pc-relative.  */
    pc = 0;

  if (is_thumb)
    status = print_insn_thumb (pc, info, given);
  else
    status = print_insn_arm (pc, info, given);

  if (is_thumb && status == 4 && second_half_valid == 0)
    {
      info->memory_error_func (status, WORD_ADDRESS (pc + 4), info);
      return -1;
    }

  return status;
}

int
print_insn_big_arm (pc, info)
     bfd_vma pc;
     struct disassemble_info * info;
{
  return print_insn (pc, info, FALSE);
}

int
print_insn_little_arm (pc, info)
     bfd_vma pc;
     struct disassemble_info * info;
{
  return print_insn (pc, info, TRUE);
}

void
print_arm_disassembler_options (FILE * stream)
{
  int i;

  fprintf (stream, _("\n\
The following ARM specific disassembler options are supported for use with\n\
the -M switch:\n"));

  for (i = NUM_ARM_REGNAMES; i--;)
    fprintf (stream, "  reg-names-%s %*c%s\n",
	     regnames[i].name,
	     (int)(14 - strlen (regnames[i].name)), ' ',
	     regnames[i].description);

  fprintf (stream, "  force-thumb              Assume all insns are Thumb insns\n");
  fprintf (stream, "  no-force-thumb           Examine preceeding label to determine an insn's type\n\n");
}
