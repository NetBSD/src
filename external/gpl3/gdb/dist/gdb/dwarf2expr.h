/* DWARF 2 Expression Evaluator.

   Copyright (C) 2001, 2002, 2003, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

   Contributed by Daniel Berlin <dan@dberlin.org>.

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

#if !defined (DWARF2EXPR_H)
#define DWARF2EXPR_H

/* The location of a value.  */
enum dwarf_value_location
{
  /* The piece is in memory.
     The value on the dwarf stack is its address.  */
  DWARF_VALUE_MEMORY,

  /* The piece is in a register.
     The value on the dwarf stack is the register number.  */
  DWARF_VALUE_REGISTER,

  /* The piece is on the dwarf stack.  */
  DWARF_VALUE_STACK,

  /* The piece is a literal.  */
  DWARF_VALUE_LITERAL,

  /* The piece was optimized out.  */
  DWARF_VALUE_OPTIMIZED_OUT,

  /* The piece is an implicit pointer.  */
  DWARF_VALUE_IMPLICIT_POINTER
};

/* The dwarf expression stack.  */

struct dwarf_stack_value
{
  ULONGEST value;

  /* Non-zero if the piece is in memory and is known to be
     on the program's stack.  It is always ok to set this to zero.
     This is used, for example, to optimize memory access from the target.
     It can vastly speed up backtraces on long latency connections when
     "set stack-cache on".  */
  int in_stack_memory;
};

/* The expression evaluator works with a dwarf_expr_context, describing
   its current state and its callbacks.  */
struct dwarf_expr_context
{
  /* The stack of values, allocated with xmalloc.  */
  struct dwarf_stack_value *stack;

  /* The number of values currently pushed on the stack, and the
     number of elements allocated to the stack.  */
  int stack_len, stack_allocated;

  /* Target architecture to use for address operations.  */
  struct gdbarch *gdbarch;

  /* Target address size in bytes.  */
  int addr_size;

  /* Offset used to relocate DW_OP_addr argument.  */
  CORE_ADDR offset;

  /* An opaque argument provided by the caller, which will be passed
     to all of the callback functions.  */
  void *baton;

  /* Return the value of register number REGNUM.  */
  CORE_ADDR (*read_reg) (void *baton, int regnum);

  /* Read LENGTH bytes at ADDR into BUF.  */
  void (*read_mem) (void *baton, gdb_byte *buf, CORE_ADDR addr, size_t length);

  /* Return the location expression for the frame base attribute, in
     START and LENGTH.  The result must be live until the current
     expression evaluation is complete.  */
  void (*get_frame_base) (void *baton, const gdb_byte **start, size_t *length);

  /* Return the CFA for the frame.  */
  CORE_ADDR (*get_frame_cfa) (void *baton);

  /* Return the PC for the frame.  */
  CORE_ADDR (*get_frame_pc) (void *baton);

  /* Return the thread-local storage address for
     DW_OP_GNU_push_tls_address.  */
  CORE_ADDR (*get_tls_address) (void *baton, CORE_ADDR offset);

  /* Execute DW_AT_location expression for the DWARF expression subroutine in
     the DIE at DIE_OFFSET in the CU from CTX.  Do not touch STACK while it
     being passed to and returned from the called DWARF subroutine.  */
  void (*dwarf_call) (struct dwarf_expr_context *ctx, size_t die_offset);

#if 0
  /* Not yet implemented.  */

  /* Return the `object address' for DW_OP_push_object_address.  */
  CORE_ADDR (*get_object_address) (void *baton);
#endif

  /* The current depth of dwarf expression recursion, via DW_OP_call*,
     DW_OP_fbreg, DW_OP_push_object_address, etc., and the maximum
     depth we'll tolerate before raising an error.  */
  int recursion_depth, max_recursion_depth;

  /* Location of the value.  */
  enum dwarf_value_location location;

  /* For DWARF_VALUE_LITERAL, the current literal value's length and
     data.  For DWARF_VALUE_IMPLICIT_POINTER, LEN is the offset of the
     target DIE.  */
  ULONGEST len;
  const gdb_byte *data;

  /* Initialization status of variable: Non-zero if variable has been
     initialized; zero otherwise.  */
  int initialized;

  /* An array of pieces.  PIECES points to its first element;
     NUM_PIECES is its length.

     Each time DW_OP_piece is executed, we add a new element to the
     end of this array, recording the current top of the stack, the
     current location, and the size given as the operand to
     DW_OP_piece.  We then pop the top value from the stack, reset the
     location, and resume evaluation.

     The Dwarf spec doesn't say whether DW_OP_piece pops the top value
     from the stack.  We do, ensuring that clients of this interface
     expecting to see a value left on the top of the stack (say, code
     evaluating frame base expressions or CFA's specified with
     DW_CFA_def_cfa_expression) will get an error if the expression
     actually marks all the values it computes as pieces.

     If an expression never uses DW_OP_piece, num_pieces will be zero.
     (It would be nice to present these cases as expressions yielding
     a single piece, so that callers need not distinguish between the
     no-DW_OP_piece and one-DW_OP_piece cases.  But expressions with
     no DW_OP_piece operations have no value to place in a piece's
     'size' field; the size comes from the surrounding data.  So the
     two cases need to be handled separately.)  */
  int num_pieces;
  struct dwarf_expr_piece *pieces;
};


/* A piece of an object, as recorded by DW_OP_piece or DW_OP_bit_piece.  */
struct dwarf_expr_piece
{
  enum dwarf_value_location location;

  union
  {
    struct
    {
      /* This piece's address, for DWARF_VALUE_MEMORY pieces.  */
      CORE_ADDR addr;
      /* Non-zero if the piece is known to be in memory and on
	 the program's stack.  */
      int in_stack_memory;
    } mem;

    /* The piece's register number or literal value, for
       DWARF_VALUE_REGISTER or DWARF_VALUE_STACK pieces.  */
    ULONGEST value;

    struct
    {
      /* A pointer to the data making up this piece,
	 for DWARF_VALUE_LITERAL pieces.  */
      const gdb_byte *data;
      /* The length of the available data.  */
      ULONGEST length;
    } literal;

    /* Used for DWARF_VALUE_IMPLICIT_POINTER.  */
    struct
    {
      /* The referent DIE from DW_OP_GNU_implicit_pointer.  */
      ULONGEST die;
      /* The byte offset into the resulting data.  */
      LONGEST offset;
    } ptr;
  } v;

  /* The length of the piece, in bits.  */
  ULONGEST size;
  /* The piece offset, in bits.  */
  ULONGEST offset;
};

struct dwarf_expr_context *new_dwarf_expr_context (void);
void free_dwarf_expr_context (struct dwarf_expr_context *ctx);
struct cleanup *
    make_cleanup_free_dwarf_expr_context (struct dwarf_expr_context *ctx);

void dwarf_expr_push (struct dwarf_expr_context *ctx, ULONGEST value,
		      int in_stack_memory);
void dwarf_expr_pop (struct dwarf_expr_context *ctx);
void dwarf_expr_eval (struct dwarf_expr_context *ctx, const gdb_byte *addr,
		      size_t len);
ULONGEST dwarf_expr_fetch (struct dwarf_expr_context *ctx, int n);
CORE_ADDR dwarf_expr_fetch_address (struct dwarf_expr_context *ctx, int n);
int dwarf_expr_fetch_in_stack_memory (struct dwarf_expr_context *ctx, int n);


const gdb_byte *read_uleb128 (const gdb_byte *buf, const gdb_byte *buf_end,
			      ULONGEST * r);
const gdb_byte *read_sleb128 (const gdb_byte *buf, const gdb_byte *buf_end,
			      LONGEST * r);

const char *dwarf_stack_op_name (unsigned int);

void dwarf_expr_require_composition (const gdb_byte *, const gdb_byte *,
				     const char *);

#endif /* dwarf2expr.h */
