/* GDB-specific functions for operating on agent expressions
   Copyright (C) 1998-2015 Free Software Foundation, Inc.

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

#ifndef AX_GDB_H
#define AX_GDB_H

struct expression;
union exp_element;

/* Types and enums */

/* GDB stores expressions in the form of a flattened tree (struct
   expression), so we just walk that tree and generate agent bytecodes
   as we go along.

   GDB's normal evaluation uses struct value, which contains the
   expression's value as well as its address or the register it came
   from.  The `+' operator uses the value, whereas the unary `&'
   operator will use the address portion.  The `=' operator will use
   the address or register number of its left hand side.

   The issues are different when generating agent bytecode.  Given a
   variable reference expression, we should not necessarily generate
   code to fetch its value, because the next operator may be `=' or
   unary `&'.  Instead, when we recurse on a subexpression, we
   indicate whether we want that expression to produce an lvalue or an
   rvalue.  If we requested an lvalue, then the recursive call tells
   us whether it generated code to compute an address on the stack, or
   whether the lvalue lives in a register.

   The `axs' prefix here means `agent expression, static', because
   this is all static analysis of the expression, i.e. analysis which
   doesn't depend on the contents of memory and registers.  */


/* Different kinds of agent expression static values.  */
enum axs_lvalue_kind
  {
    /* We generated code to compute the subexpression's value.
       Constants and arithmetic operators yield this.  */
    axs_rvalue,

    /* We generated code to yield the subexpression's value's address on
       the top of the stack.  If the caller needs an rvalue, it should
       call require_rvalue to produce the rvalue from this address.  */
    axs_lvalue_memory,

    /* We didn't generate any code, and the stack is undisturbed,
       because the subexpression's value lives in a register; u.reg is
       the register number.  If the caller needs an rvalue, it should
       call require_rvalue to produce the rvalue from this register
       number.  */
    axs_lvalue_register
  };

/* Structure describing what we got from a subexpression.  Think of
   this as parallel to value.h's enum lval_type, except that we're
   describing a value which will exist when the expression is
   evaluated in the future, not a value we have in our hand.  */
struct axs_value
  {
    enum axs_lvalue_kind kind;	/* see above */

    /* The type of the subexpression.  Even if lvalue == axs_lvalue_memory,
       this is the type of the value itself; the value on the stack is a
       "pointer to" an object of this type.  */
    struct type *type;

    /* If nonzero, this is a variable which does not actually exist in
       the program.  */
    char optimized_out;

    union
      {
	/* if kind == axs_lvalue_register, this is the register number */
	int reg;
      }
    u;
  };


/* Translating GDB expressions into agent expressions.  */

/* Given a GDB expression EXPR, return bytecode to trace its value.
   The result will use the `trace' and `trace_quick' bytecodes to
   record the value of all memory touched by the expression, and leave
   no values on the stack.  The caller can then use the ax_reqs
   function to discover which registers the expression uses.  */
extern struct agent_expr *gen_trace_for_expr (CORE_ADDR, struct expression *,
					      int);

extern struct agent_expr *gen_trace_for_var (CORE_ADDR, struct gdbarch *,
					     struct symbol *, int);

extern struct agent_expr *gen_trace_for_return_address (CORE_ADDR,
							struct gdbarch *,
							int);

extern struct agent_expr *gen_eval_for_expr (CORE_ADDR, struct expression *);

extern void gen_expr (struct expression *exp, union exp_element **pc,
		      struct agent_expr *ax, struct axs_value *value);

extern void require_rvalue (struct agent_expr *ax, struct axs_value *value);

struct format_piece;
extern struct agent_expr *gen_printf (CORE_ADDR, struct gdbarch *,
				      CORE_ADDR, LONGEST, const char *, int,
				      struct format_piece *,
				      int, struct expression **);

#endif /* AX_GDB_H */
