/* Disassemble support for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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

#define DISASSEMBLY_SOURCE	(0x1 << 0)
#define DISASSEMBLY_RAW_INSN	(0x1 << 1)
#define DISASSEMBLY_OMIT_FNAME	(0x1 << 2)
#define DISASSEMBLY_FILENAME	(0x1 << 3)
#define DISASSEMBLY_OMIT_PC	(0x1 << 4)

struct ui_out;
struct ui_file;

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
