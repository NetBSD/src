/* Disassembler definitions for ARM.

   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004 Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


struct arm_opcode
{
  unsigned long arch;		/* Architecture defining this insn.  */
  unsigned long value, mask;	/* Recognise insn if (op&mask)==value.  */
  char *assembler;		/* How to disassemble this insn.  */
};

struct thumb_opcode
{
  unsigned long arch;		/* Architecture defining this insn.  */
  unsigned short value, mask;	/* Recognise insn if (op&mask)==value.  */
  char * assembler;		/* How to disassemble this insn.  */
};


#define BDISP(x) ((((x) & 0xffffff) ^ 0x800000) - 0x800000) /* 26 bit */

#define BDISP23(x) ((((((x) & 0x07ff) << 11) | (((x) & 0x07ff0000) >> 16)) \
                     ^ 0x200000) - 0x200000) /* 23bit */

