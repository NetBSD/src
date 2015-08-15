/* Convert a DWARF location expression to C

   Copyright (C) 2014-2015 Free Software Foundation, Inc.

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

#include "defs.h"
#include "dwarf2.h"
#include "dwarf2expr.h"
#include "dwarf2loc.h"
#include "ui-file.h"
#include "utils.h"
#include "compile-internal.h"
#include "compile.h"
#include "block.h"
#include "dwarf2-frame.h"
#include "gdb_vecs.h"
#include "value.h"



/* Information about a given instruction.  */

struct insn_info
{
  /* Stack depth at entry.  */

  unsigned int depth;

  /* Whether this instruction has been visited.  */

  unsigned int visited : 1;

  /* Whether this instruction needs a label.  */

  unsigned int label : 1;

  /* Whether this instruction is DW_OP_GNU_push_tls_address.  This is
     a hack until we can add a feature to glibc to let us properly
     generate code for TLS.  */

  unsigned int is_tls : 1;
};

/* A helper function for compute_stack_depth that does the work.  This
   examines the DWARF expression starting from START and computes
   stack effects.

   NEED_TEMPVAR is an out parameter which is set if this expression
   needs a special temporary variable to be emitted (see the code
   generator).
   INFO is an array of insn_info objects, indexed by offset from the
   start of the DWARF expression.
   TO_DO is a list of bytecodes which must be examined; it may be
   added to by this function.
   BYTE_ORDER and ADDR_SIZE describe this bytecode in the obvious way.
   OP_PTR and OP_END are the bounds of the DWARF expression.  */

static void
compute_stack_depth_worker (int start, int *need_tempvar,
			    struct insn_info *info,
			    VEC (int) **to_do,
			    enum bfd_endian byte_order, unsigned int addr_size,
			    const gdb_byte *op_ptr, const gdb_byte *op_end)
{
  const gdb_byte * const base = op_ptr;
  int stack_depth;

  op_ptr += start;
  gdb_assert (info[start].visited);
  stack_depth = info[start].depth;

  while (op_ptr < op_end)
    {
      enum dwarf_location_atom op = *op_ptr;
      uint64_t reg;
      int64_t offset;
      int ndx = op_ptr - base;

#define SET_CHECK_DEPTH(WHERE)				\
      if (info[WHERE].visited)				\
	{						\
	  if (info[WHERE].depth != stack_depth)		\
	    error (_("inconsistent stack depths"));	\
	}						\
      else						\
	{						\
	  /* Stack depth not set, so set it.  */	\
	  info[WHERE].visited = 1;			\
	  info[WHERE].depth = stack_depth;		\
	}

      SET_CHECK_DEPTH (ndx);

      ++op_ptr;

      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  ++stack_depth;
	  break;

	case DW_OP_addr:
	  op_ptr += addr_size;
	  ++stack_depth;
	  break;

	case DW_OP_const1u:
	case DW_OP_const1s:
	  op_ptr += 1;
	  ++stack_depth;
	  break;
	case DW_OP_const2u:
	case DW_OP_const2s:
	  op_ptr += 2;
	  ++stack_depth;
	  break;
	case DW_OP_const4u:
	case DW_OP_const4s:
	  op_ptr += 4;
	  ++stack_depth;
	  break;
	case DW_OP_const8u:
	case DW_OP_const8s:
	  op_ptr += 8;
	  ++stack_depth;
	  break;
	case DW_OP_constu:
	case DW_OP_consts:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  ++stack_depth;
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  ++stack_depth;
	  break;

	case DW_OP_regx:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  ++stack_depth;
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  ++stack_depth;
	  break;
	case DW_OP_bregx:
	  {
	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    ++stack_depth;
	  }
	  break;
	case DW_OP_fbreg:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  ++stack_depth;
	  break;

	case DW_OP_dup:
	  ++stack_depth;
	  break;

	case DW_OP_drop:
	  --stack_depth;
	  break;

	case DW_OP_pick:
	  ++op_ptr;
	  ++stack_depth;
	  break;

	case DW_OP_rot:
	case DW_OP_swap:
	  *need_tempvar = 1;
	  break;

	case DW_OP_over:
	  ++stack_depth;
	  break;

	case DW_OP_abs:
	case DW_OP_neg:
	case DW_OP_not:
	case DW_OP_deref:
	  break;

	case DW_OP_deref_size:
	  ++op_ptr;
	  break;

	case DW_OP_plus_uconst:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  break;

	case DW_OP_div:
	case DW_OP_shra:
	case DW_OP_and:
	case DW_OP_minus:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_or:
	case DW_OP_plus:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_xor:
	case DW_OP_le:
	case DW_OP_ge:
	case DW_OP_eq:
	case DW_OP_lt:
	case DW_OP_gt:
	case DW_OP_ne:
	  --stack_depth;
	  break;

	case DW_OP_call_frame_cfa:
	  ++stack_depth;
	  break;

	case DW_OP_GNU_push_tls_address:
	  info[ndx].is_tls = 1;
	  break;

	case DW_OP_skip:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  offset = op_ptr + offset - base;
	  /* If the destination has not been seen yet, add it to the
	     to-do list.  */
	  if (!info[offset].visited)
	    VEC_safe_push (int, *to_do, offset);
	  SET_CHECK_DEPTH (offset);
	  info[offset].label = 1;
	  /* We're done with this line of code.  */
	  return;

	case DW_OP_bra:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  offset = op_ptr + offset - base;
	  --stack_depth;
	  /* If the destination has not been seen yet, add it to the
	     to-do list.  */
	  if (!info[offset].visited)
	    VEC_safe_push (int, *to_do, offset);
	  SET_CHECK_DEPTH (offset);
	  info[offset].label = 1;
	  break;

	case DW_OP_nop:
	  break;

	default:
	  error (_("unhandled DWARF op: %s"), get_DW_OP_name (op));
	}
    }

  gdb_assert (op_ptr == op_end);

#undef SET_CHECK_DEPTH
}

/* Compute the maximum needed stack depth of a DWARF expression, and
   some other information as well.

   BYTE_ORDER and ADDR_SIZE describe this bytecode in the obvious way.
   NEED_TEMPVAR is an out parameter which is set if this expression
   needs a special temporary variable to be emitted (see the code
   generator).
   IS_TLS is an out parameter which is set if this expression refers
   to a TLS variable.
   OP_PTR and OP_END are the bounds of the DWARF expression.
   INITIAL_DEPTH is the initial depth of the DWARF expression stack.
   INFO is an array of insn_info objects, indexed by offset from the
   start of the DWARF expression.

   This returns the maximum stack depth.  */

static int
compute_stack_depth (enum bfd_endian byte_order, unsigned int addr_size,
		     int *need_tempvar, int *is_tls,
		     const gdb_byte *op_ptr, const gdb_byte *op_end,
		     int initial_depth,
		     struct insn_info **info)
{
  unsigned char *set;
  struct cleanup *outer_cleanup, *cleanup;
  VEC (int) *to_do = NULL;
  int stack_depth, i;

  *info = XCNEWVEC (struct insn_info, op_end - op_ptr);
  outer_cleanup = make_cleanup (xfree, *info);

  cleanup = make_cleanup (VEC_cleanup (int), &to_do);

  VEC_safe_push (int, to_do, 0);
  (*info)[0].depth = initial_depth;
  (*info)[0].visited = 1;

  while (!VEC_empty (int, to_do))
    {
      int ndx = VEC_pop (int, to_do);

      compute_stack_depth_worker (ndx, need_tempvar, *info, &to_do,
				  byte_order, addr_size,
				  op_ptr, op_end);
    }

  stack_depth = 0;
  *is_tls = 0;
  for (i = 0; i < op_end - op_ptr; ++i)
    {
      if ((*info)[i].depth > stack_depth)
	stack_depth = (*info)[i].depth;
      if ((*info)[i].is_tls)
	*is_tls = 1;
    }

  do_cleanups (cleanup);
  discard_cleanups (outer_cleanup);
  return stack_depth + 1;
}



#define GCC_UINTPTR "__gdb_uintptr"
#define GCC_INTPTR "__gdb_intptr"

/* Emit code to push a constant.  */

static void
push (int indent, struct ui_file *stream, ULONGEST l)
{
  fprintfi_filtered (indent, stream, "__gdb_stack[++__gdb_tos] = %s;\n",
		     hex_string (l));
}

/* Emit code to push an arbitrary expression.  This works like
   printf.  */

static void
pushf (int indent, struct ui_file *stream, const char *format, ...)
{
  va_list args;

  fprintfi_filtered (indent, stream, "__gdb_stack[__gdb_tos + 1] = ");
  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
  fprintf_filtered (stream, ";\n");

  fprintfi_filtered (indent, stream, "++__gdb_tos;\n");
}

/* Emit code for a unary expression -- one which operates in-place on
   the top-of-stack.  This works like printf.  */

static void
unary (int indent, struct ui_file *stream, const char *format, ...)
{
  va_list args;

  fprintfi_filtered (indent, stream, "__gdb_stack[__gdb_tos] = ");
  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
  fprintf_filtered (stream, ";\n");
}

/* Emit code for a unary expression -- one which uses the top two
   stack items, popping the topmost one.  This works like printf.  */

static void
binary (int indent, struct ui_file *stream, const char *format, ...)
{
  va_list args;

  fprintfi_filtered (indent, stream, "__gdb_stack[__gdb_tos - 1] = ");
  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
  fprintf_filtered (stream, ";\n");
  fprintfi_filtered (indent, stream, "--__gdb_tos;\n");
}

/* Print the name of a label given its "SCOPE", an arbitrary integer
   used for uniqueness, and its TARGET, the bytecode offset
   corresponding to the label's point of definition.  */

static void
print_label (struct ui_file *stream, unsigned int scope, int target)
{
  fprintf_filtered (stream, "__label_%u_%s",
		    scope, pulongest (target));
}

/* Emit code that pushes a register's address on the stack.
   REGISTERS_USED is an out parameter which is updated to note which
   register was needed by this expression.  */

static void
pushf_register_address (int indent, struct ui_file *stream,
			unsigned char *registers_used,
			struct gdbarch *gdbarch, int regnum)
{
  char *regname = compile_register_name_mangled (gdbarch, regnum);
  struct cleanup *cleanups = make_cleanup (xfree, regname);

  registers_used[regnum] = 1;
  pushf (indent, stream, "&" COMPILE_I_SIMPLE_REGISTER_ARG_NAME	 "->%s",
	 regname);

  do_cleanups (cleanups);
}

/* Emit code that pushes a register's value on the stack.
   REGISTERS_USED is an out parameter which is updated to note which
   register was needed by this expression.  OFFSET is added to the
   register's value before it is pushed.  */

static void
pushf_register (int indent, struct ui_file *stream,
		unsigned char *registers_used,
		struct gdbarch *gdbarch, int regnum, uint64_t offset)
{
  char *regname = compile_register_name_mangled (gdbarch, regnum);
  struct cleanup *cleanups = make_cleanup (xfree, regname);

  registers_used[regnum] = 1;
  if (offset == 0)
    pushf (indent, stream, COMPILE_I_SIMPLE_REGISTER_ARG_NAME "->%s",
	   regname);
  else
    pushf (indent, stream, COMPILE_I_SIMPLE_REGISTER_ARG_NAME "->%s + %s",
	   regname, hex_string (offset));

  do_cleanups (cleanups);
}

/* Compile a DWARF expression to C code.

   INDENT is the indentation level to use.
   STREAM is the stream where the code should be written.

   TYPE_NAME names the type of the result of the DWARF expression.
   For locations this is "void *" but for array bounds it will be an
   integer type.

   RESULT_NAME is the name of a variable in the resulting C code.  The
   result of the expression will be assigned to this variable.

   SYM is the symbol corresponding to this expression.
   PC is the location at which the expression is being evaluated.
   ARCH is the architecture to use.

   REGISTERS_USED is an out parameter which is updated to note which
   registers were needed by this expression.

   ADDR_SIZE is the DWARF address size to use.

   OPT_PTR and OP_END are the bounds of the DWARF expression.

   If non-NULL, INITIAL points to an initial value to write to the
   stack.  If NULL, no initial value is written.

   PER_CU is the per-CU object used for looking up various other
   things.  */

static void
do_compile_dwarf_expr_to_c (int indent, struct ui_file *stream,
			    const char *type_name,
			    const char *result_name,
			    struct symbol *sym, CORE_ADDR pc,
			    struct gdbarch *arch,
			    unsigned char *registers_used,
			    unsigned int addr_size,
			    const gdb_byte *op_ptr, const gdb_byte *op_end,
			    CORE_ADDR *initial,
			    struct dwarf2_per_cu_data *per_cu)
{
  /* We keep a counter so that labels and other objects we create have
     unique names.  */
  static unsigned int scope;

  enum bfd_endian byte_order = gdbarch_byte_order (arch);
  const gdb_byte * const base = op_ptr;
  int need_tempvar = 0;
  int is_tls = 0;
  struct cleanup *cleanup;
  struct insn_info *info;
  int stack_depth;

  ++scope;

  fprintfi_filtered (indent, stream, "%s%s;\n", type_name, result_name);
  fprintfi_filtered (indent, stream, "{\n");
  indent += 2;

  stack_depth = compute_stack_depth (byte_order, addr_size,
				     &need_tempvar, &is_tls,
				     op_ptr, op_end, initial != NULL,
				     &info);
  cleanup = make_cleanup (xfree, info);

  /* This is a hack until we can add a feature to glibc to let us
     properly generate code for TLS.  You might think we could emit
     the address in the ordinary course of translating
     DW_OP_GNU_push_tls_address, but since the operand appears on the
     stack, it is relatively hard to find, and the idea of calling
     target_translate_tls_address with OFFSET==0 and then adding the
     offset by hand seemed too hackish.  */
  if (is_tls)
    {
      struct frame_info *frame = get_selected_frame (NULL);
      struct value *val;

      if (frame == NULL)
	error (_("Symbol \"%s\" cannot be used because "
		 "there is no selected frame"),
	       SYMBOL_PRINT_NAME (sym));

      val = read_var_value (sym, frame);
      if (VALUE_LVAL (val) != lval_memory)
	error (_("Symbol \"%s\" cannot be used for compilation evaluation "
		 "as its address has not been found."),
	       SYMBOL_PRINT_NAME (sym));

      warning (_("Symbol \"%s\" is thread-local and currently can only "
		 "be referenced from the current thread in "
		 "compiled code."),
	       SYMBOL_PRINT_NAME (sym));

      fprintfi_filtered (indent, stream, "%s = %s;\n",
			 result_name,
			 core_addr_to_string (value_address (val)));
      fprintfi_filtered (indent - 2, stream, "}\n");
      do_cleanups (cleanup);
      return;
    }

  fprintfi_filtered (indent, stream, GCC_UINTPTR " __gdb_stack[%d];\n",
		     stack_depth);

  if (need_tempvar)
    fprintfi_filtered (indent, stream, GCC_UINTPTR " __gdb_tmp;\n");
  fprintfi_filtered (indent, stream, "int __gdb_tos = -1;\n");

  if (initial != NULL)
    pushf (indent, stream, core_addr_to_string (*initial));

  while (op_ptr < op_end)
    {
      enum dwarf_location_atom op = *op_ptr;
      uint64_t uoffset, reg;
      int64_t offset;

      print_spaces (indent - 2, stream);
      if (info[op_ptr - base].label)
	{
	  print_label (stream, scope, op_ptr - base);
	  fprintf_filtered (stream, ":;");
	}
      fprintf_filtered (stream, "/* %s */\n", get_DW_OP_name (op));

      /* This is handy for debugging the generated code:
      fprintf_filtered (stream, "if (__gdb_tos != %d) abort ();\n",
			(int) info[op_ptr - base].depth - 1);
      */

      ++op_ptr;

      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  push (indent, stream, op - DW_OP_lit0);
	  break;

	case DW_OP_addr:
	  op_ptr += addr_size;
	  /* Some versions of GCC emit DW_OP_addr before
	     DW_OP_GNU_push_tls_address.  In this case the value is an
	     index, not an address.  We don't support things like
	     branching between the address and the TLS op.  */
	  if (op_ptr >= op_end || *op_ptr != DW_OP_GNU_push_tls_address)
	    uoffset += dwarf2_per_cu_text_offset (per_cu);
	  push (indent, stream, uoffset);
	  break;

	case DW_OP_const1u:
	  push (indent, stream,
		extract_unsigned_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;
	case DW_OP_const1s:
	  push (indent, stream,
		extract_signed_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;
	case DW_OP_const2u:
	  push (indent, stream,
		extract_unsigned_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;
	case DW_OP_const2s:
	  push (indent, stream,
		extract_signed_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;
	case DW_OP_const4u:
	  push (indent, stream,
		extract_unsigned_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;
	case DW_OP_const4s:
	  push (indent, stream,
		extract_signed_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;
	case DW_OP_const8u:
	  push (indent, stream,
		extract_unsigned_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;
	case DW_OP_const8s:
	  push (indent, stream,
		extract_signed_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;
	case DW_OP_constu:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &uoffset);
	  push (indent, stream, uoffset);
	  break;
	case DW_OP_consts:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  push (indent, stream, offset);
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  pushf_register_address (indent, stream, registers_used, arch,
				  dwarf2_reg_to_regnum_or_error (arch,
							      op - DW_OP_reg0));
	  break;

	case DW_OP_regx:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  pushf_register_address (indent, stream, registers_used, arch,
				  dwarf2_reg_to_regnum_or_error (arch, reg));
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  pushf_register (indent, stream, registers_used, arch,
			  dwarf2_reg_to_regnum_or_error (arch,
							 op - DW_OP_breg0),
			  offset);
	  break;
	case DW_OP_bregx:
	  {
	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    pushf_register (indent, stream, registers_used, arch,
			    dwarf2_reg_to_regnum_or_error (arch, reg), offset);
	  }
	  break;
	case DW_OP_fbreg:
	  {
	    const gdb_byte *datastart;
	    size_t datalen;
	    const struct block *b;
	    struct symbol *framefunc;
	    char fb_name[50];

	    b = block_for_pc (pc);

	    if (!b)
	      error (_("No block found for address"));

	    framefunc = block_linkage_function (b);

	    if (!framefunc)
	      error (_("No function found for block"));

	    func_get_frame_base_dwarf_block (framefunc, pc,
					     &datastart, &datalen);

	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);

	    /* Generate a unique-enough name, in case the frame base
	       is computed multiple times in this expression.  */
	    xsnprintf (fb_name, sizeof (fb_name), "__frame_base_%ld",
		       (long) (op_ptr - base));

	    do_compile_dwarf_expr_to_c (indent, stream,
					"void *", fb_name,
					sym, pc,
					arch, registers_used, addr_size,
					datastart, datastart + datalen,
					NULL, per_cu);

	    pushf (indent, stream, "%s + %s", fb_name, hex_string (offset));
	  }
	  break;

	case DW_OP_dup:
	  pushf (indent, stream, "__gdb_stack[__gdb_tos]");
	  break;

	case DW_OP_drop:
	  fprintfi_filtered (indent, stream, "--__gdb_tos;\n");
	  break;

	case DW_OP_pick:
	  offset = *op_ptr++;
	  pushf (indent, stream, "__gdb_stack[__gdb_tos - %d]", offset);
	  break;

	case DW_OP_swap:
	  fprintfi_filtered (indent, stream,
			     "__gdb_tmp = __gdb_stack[__gdb_tos - 1];\n");
	  fprintfi_filtered (indent, stream,
			     "__gdb_stack[__gdb_tos - 1] = "
			     "__gdb_stack[__gdb_tos];\n");
	  fprintfi_filtered (indent, stream, ("__gdb_stack[__gdb_tos] = "
					      "__gdb_tmp;\n"));
	  break;

	case DW_OP_over:
	  pushf (indent, stream, "__gdb_stack[__gdb_tos - 1]");
	  break;

	case DW_OP_rot:
	  fprintfi_filtered (indent, stream, ("__gdb_tmp = "
					      "__gdb_stack[__gdb_tos];\n"));
	  fprintfi_filtered (indent, stream,
			     "__gdb_stack[__gdb_tos] = "
			     "__gdb_stack[__gdb_tos - 1];\n");
	  fprintfi_filtered (indent, stream,
			     "__gdb_stack[__gdb_tos - 1] = "
			     "__gdb_stack[__gdb_tos -2];\n");
	  fprintfi_filtered (indent, stream, "__gdb_stack[__gdb_tos - 2] = "
			     "__gdb_tmp;\n");
	  break;

	case DW_OP_deref:
	case DW_OP_deref_size:
	  {
	    int size;
	    const char *mode;

	    if (op == DW_OP_deref_size)
	      size = *op_ptr++;
	    else
	      size = addr_size;

	    mode = c_get_mode_for_size (size);
	    if (mode == NULL)
	      error (_("Unsupported size %d in %s"),
		     size, get_DW_OP_name (op));

	    /* Cast to a pointer of the desired type, then
	       dereference.  */
	    fprintfi_filtered (indent, stream,
			       "__gdb_stack[__gdb_tos] = "
			       "*((__gdb_int_%s *) "
			       "__gdb_stack[__gdb_tos]);\n",
			       mode);
	  }
	  break;

	case DW_OP_abs:
	  unary (indent, stream,
		 "((" GCC_INTPTR ") __gdb_stack[__gdb_tos]) < 0 ? "
		 "-__gdb_stack[__gdb_tos] : __gdb_stack[__gdb_tos]");
	  break;

	case DW_OP_neg:
	  unary (indent, stream, "-__gdb_stack[__gdb_tos]");
	  break;

	case DW_OP_not:
	  unary (indent, stream, "~__gdb_stack[__gdb_tos]");
	  break;

	case DW_OP_plus_uconst:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  unary (indent, stream, "__gdb_stack[__gdb_tos] + %s",
		 hex_string (reg));
	  break;

	case DW_OP_div:
	  binary (indent, stream, ("((" GCC_INTPTR
				   ") __gdb_stack[__gdb_tos-1]) / (("
				   GCC_INTPTR ") __gdb_stack[__gdb_tos])"));
	  break;

	case DW_OP_shra:
	  binary (indent, stream,
		  "((" GCC_INTPTR ") __gdb_stack[__gdb_tos-1]) >> "
		  "__gdb_stack[__gdb_tos]");
	  break;

#define BINARY(OP)							\
	  binary (indent, stream, ("__gdb_stack[__gdb_tos-1] " #OP	\
				   " __gdb_stack[__gdb_tos]"));	\
	  break

	case DW_OP_and:
	  BINARY (&);
	case DW_OP_minus:
	  BINARY (-);
	case DW_OP_mod:
	  BINARY (%);
	case DW_OP_mul:
	  BINARY (*);
	case DW_OP_or:
	  BINARY (|);
	case DW_OP_plus:
	  BINARY (+);
	case DW_OP_shl:
	  BINARY (<<);
	case DW_OP_shr:
	  BINARY (>>);
	case DW_OP_xor:
	  BINARY (^);
#undef BINARY

#define COMPARE(OP)							\
	  binary (indent, stream,					\
		  "(((" GCC_INTPTR ") __gdb_stack[__gdb_tos-1]) " #OP	\
		  " ((" GCC_INTPTR					\
		  ") __gdb_stack[__gdb_tos]))");			\
	  break

	case DW_OP_le:
	  COMPARE (<=);
	case DW_OP_ge:
	  COMPARE (>=);
	case DW_OP_eq:
	  COMPARE (==);
	case DW_OP_lt:
	  COMPARE (<);
	case DW_OP_gt:
	  COMPARE (>);
	case DW_OP_ne:
	  COMPARE (!=);
#undef COMPARE

	case DW_OP_call_frame_cfa:
	  {
	    int regnum;
	    CORE_ADDR text_offset;
	    LONGEST off;
	    const gdb_byte *cfa_start, *cfa_end;

	    if (dwarf2_fetch_cfa_info (arch, pc, per_cu,
				       &regnum, &off,
				       &text_offset, &cfa_start, &cfa_end))
	      {
		/* Register.  */
		pushf_register (indent, stream, registers_used, arch, regnum,
				off);
	      }
	    else
	      {
		/* Another expression.  */
		char cfa_name[50];

		/* Generate a unique-enough name, in case the CFA is
		   computed multiple times in this expression.  */
		xsnprintf (cfa_name, sizeof (cfa_name),
			   "__cfa_%ld", (long) (op_ptr - base));

		do_compile_dwarf_expr_to_c (indent, stream,
					    "void *", cfa_name,
					    sym, pc, arch, registers_used,
					    addr_size,
					    cfa_start, cfa_end,
					    &text_offset, per_cu);
		pushf (indent, stream, cfa_name);
	      }
	  }

	  break;

	case DW_OP_skip:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  fprintfi_filtered (indent, stream, "goto ");
	  print_label (stream, scope, op_ptr + offset - base);
	  fprintf_filtered (stream, ";\n");
	  break;

	case DW_OP_bra:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  fprintfi_filtered (indent, stream,
			     "if ((( " GCC_INTPTR
			     ") __gdb_stack[__gdb_tos--]) != 0) goto ");
	  print_label (stream, scope, op_ptr + offset - base);
	  fprintf_filtered (stream, ";\n");
	  break;

	case DW_OP_nop:
	  break;

	default:
	  error (_("unhandled DWARF op: %s"), get_DW_OP_name (op));
	}
    }

  fprintfi_filtered (indent, stream, "%s = (%s) __gdb_stack[__gdb_tos];\n",
		     result_name, type_name);
  fprintfi_filtered (indent - 2, stream, "}\n");

  do_cleanups (cleanup);
}

/* See compile.h.  */

void
compile_dwarf_expr_to_c (struct ui_file *stream, const char *result_name,
			 struct symbol *sym, CORE_ADDR pc,
			 struct gdbarch *arch, unsigned char *registers_used,
			 unsigned int addr_size,
			 const gdb_byte *op_ptr, const gdb_byte *op_end,
			 struct dwarf2_per_cu_data *per_cu)
{
  do_compile_dwarf_expr_to_c (2, stream, "void *", result_name, sym, pc,
			      arch, registers_used, addr_size, op_ptr, op_end,
			      NULL, per_cu);
}

/* See compile.h.  */

void
compile_dwarf_bounds_to_c (struct ui_file *stream,
			   const char *result_name,
			   const struct dynamic_prop *prop,
			   struct symbol *sym, CORE_ADDR pc,
			   struct gdbarch *arch, unsigned char *registers_used,
			   unsigned int addr_size,
			   const gdb_byte *op_ptr, const gdb_byte *op_end,
			   struct dwarf2_per_cu_data *per_cu)
{
  do_compile_dwarf_expr_to_c (2, stream, "unsigned long ", result_name,
			      sym, pc, arch, registers_used,
			      addr_size, op_ptr, op_end, NULL, per_cu);
}
