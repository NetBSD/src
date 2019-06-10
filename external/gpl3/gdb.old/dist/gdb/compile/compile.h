/* Header file for Compile and inject module.

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

#ifndef GDB_COMPILE_H
#define GDB_COMPILE_H

struct ui_file;
struct gdbarch;
struct dwarf2_per_cu_data;
struct symbol;
struct dynamic_prop;

/* Public function that is called from compile_control case in the
   expression command.  GDB returns either a CMD, or a CMD_STRING, but
   never both.  */

extern void eval_compile_command (struct command_line *cmd,
				  const char *cmd_string,
				  enum compile_i_scope_types scope,
				  void *scope_data);

/* Compile a DWARF location expression to C, suitable for use by the
   compiler.

   STREAM is the stream where the code should be written.

   RESULT_NAME is the name of a variable in the resulting C code.  The
   result of the expression will be assigned to this variable.

   SYM is the symbol corresponding to this expression.
   PC is the location at which the expression is being evaluated.
   ARCH is the architecture to use.

   REGISTERS_USED is an out parameter which is updated to note which
   registers were needed by this expression.

   ADDR_SIZE is the DWARF address size to use.

   OPT_PTR and OP_END are the bounds of the DWARF expression.

   PER_CU is the per-CU object used for looking up various other
   things.  */

extern void compile_dwarf_expr_to_c (string_file &stream,
				     const char *result_name,
				     struct symbol *sym,
				     CORE_ADDR pc,
				     struct gdbarch *arch,
				     unsigned char *registers_used,
				     unsigned int addr_size,
				     const gdb_byte *op_ptr,
				     const gdb_byte *op_end,
				     struct dwarf2_per_cu_data *per_cu);

/* Compile a DWARF bounds expression to C, suitable for use by the
   compiler.

   STREAM is the stream where the code should be written.

   RESULT_NAME is the name of a variable in the resulting C code.  The
   result of the expression will be assigned to this variable.

   PROP is the dynamic property for which we're compiling.

   SYM is the symbol corresponding to this expression.
   PC is the location at which the expression is being evaluated.
   ARCH is the architecture to use.

   REGISTERS_USED is an out parameter which is updated to note which
   registers were needed by this expression.

   ADDR_SIZE is the DWARF address size to use.

   OPT_PTR and OP_END are the bounds of the DWARF expression.

   PER_CU is the per-CU object used for looking up various other
   things.  */

extern void compile_dwarf_bounds_to_c (string_file &stream,
				       const char *result_name,
				       const struct dynamic_prop *prop,
				       struct symbol *sym, CORE_ADDR pc,
				       struct gdbarch *arch,
				       unsigned char *registers_used,
				       unsigned int addr_size,
				       const gdb_byte *op_ptr,
				       const gdb_byte *op_end,
				       struct dwarf2_per_cu_data *per_cu);

extern void compile_print_value (struct value *val, void *data_voidp);

#endif /* GDB_COMPILE_H */
