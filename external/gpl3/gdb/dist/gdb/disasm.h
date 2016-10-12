/* Disassemble support for GDB.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef DISASM_H
#define DISASM_H

#include "dis-asm.h"

#define DISASSEMBLY_SOURCE_DEPRECATED (0x1 << 0)
#define DISASSEMBLY_RAW_INSN	(0x1 << 1)
#define DISASSEMBLY_OMIT_FNAME	(0x1 << 2)
#define DISASSEMBLY_FILENAME	(0x1 << 3)
#define DISASSEMBLY_OMIT_PC	(0x1 << 4)
#define DISASSEMBLY_SOURCE	(0x1 << 5)
#define DISASSEMBLY_SPECULATIVE	(0x1 << 6)

struct gdbarch;
struct ui_out;
struct ui_file;

/* An instruction to be disassembled.  */

struct disasm_insn
{
  /* The address of the memory containing the instruction.  */
  CORE_ADDR addr;

  /* An optional instruction number.  If non-zero, it is printed first.  */
  unsigned int number;

  /* True if the instruction was executed speculatively.  */
  unsigned int is_speculative:1;
};

/* Prints the instruction INSN into UIOUT and returns the length of the
   printed instruction in bytes.  */

extern int gdb_pretty_print_insn (struct gdbarch *gdbarch, struct ui_out *uiout,
				  struct disassemble_info * di,
				  const struct disasm_insn *insn, int flags,
				  struct ui_file *stb);

/* Return a filled in disassemble_info object for use by gdb.  */

extern struct disassemble_info gdb_disassemble_info (struct gdbarch *gdbarch,
						     struct ui_file *file);

extern void gdb_disassembly (struct gdbarch *gdbarch, struct ui_out *uiout,
			     char *file_string, int flags, int how_many,
			     CORE_ADDR low, CORE_ADDR high);

/* Print the instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns the length of the instruction, in bytes,
   and, if requested, the number of branch delay slot instructions.  */

extern int gdb_print_insn (struct gdbarch *gdbarch, CORE_ADDR memaddr,
			   struct ui_file *stream, int *branch_delay_insns);

/* Return the length in bytes of the instruction at address MEMADDR in
   debugged memory.  */

extern int gdb_insn_length (struct gdbarch *gdbarch, CORE_ADDR memaddr);

/* Return the length in bytes of INSN, originally at MEMADDR.  MAX_LEN
   is the size of the buffer containing INSN.  */

extern int gdb_buffered_insn_length (struct gdbarch *gdbarch,
				     const gdb_byte *insn, int max_len,
				     CORE_ADDR memaddr);

#endif
