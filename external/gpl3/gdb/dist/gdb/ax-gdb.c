/* GDB-specific functions for operating on agent expressions.

   Copyright (C) 1998-2014 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "symfile.h"
#include "gdbtypes.h"
#include "language.h"
#include "value.h"
#include "expression.h"
#include "command.h"
#include "gdbcmd.h"
#include "frame.h"
#include "target.h"
#include "ax.h"
#include "ax-gdb.h"
#include <string.h>
#include "block.h"
#include "regcache.h"
#include "user-regs.h"
#include "dictionary.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "cp-support.h"
#include "arch-utils.h"
#include "cli/cli-utils.h"
#include "linespec.h"

#include "valprint.h"
#include "c-lang.h"

#include "format.h"

/* To make sense of this file, you should read doc/agentexpr.texi.
   Then look at the types and enums in ax-gdb.h.  For the code itself,
   look at gen_expr, towards the bottom; that's the main function that
   looks at the GDB expressions and calls everything else to generate
   code.

   I'm beginning to wonder whether it wouldn't be nicer to internally
   generate trees, with types, and then spit out the bytecode in
   linear form afterwards; we could generate fewer `swap', `ext', and
   `zero_ext' bytecodes that way; it would make good constant folding
   easier, too.  But at the moment, I think we should be willing to
   pay for the simplicity of this code with less-than-optimal bytecode
   strings.

   Remember, "GBD" stands for "Great Britain, Dammit!"  So be careful.  */



/* Prototypes for local functions.  */

/* There's a standard order to the arguments of these functions:
   union exp_element ** --- pointer into expression
   struct agent_expr * --- agent expression buffer to generate code into
   struct axs_value * --- describes value left on top of stack  */

static struct value *const_var_ref (struct symbol *var);
static struct value *const_expr (union exp_element **pc);
static struct value *maybe_const_expr (union exp_element **pc);

static void gen_traced_pop (struct gdbarch *, struct agent_expr *,
			    struct axs_value *);

static void gen_sign_extend (struct agent_expr *, struct type *);
static void gen_extend (struct agent_expr *, struct type *);
static void gen_fetch (struct agent_expr *, struct type *);
static void gen_left_shift (struct agent_expr *, int);


static void gen_frame_args_address (struct gdbarch *, struct agent_expr *);
static void gen_frame_locals_address (struct gdbarch *, struct agent_expr *);
static void gen_offset (struct agent_expr *ax, int offset);
static void gen_sym_offset (struct agent_expr *, struct symbol *);
static void gen_var_ref (struct gdbarch *, struct agent_expr *ax,
			 struct axs_value *value, struct symbol *var);


static void gen_int_literal (struct agent_expr *ax,
			     struct axs_value *value,
			     LONGEST k, struct type *type);

static void gen_usual_unary (struct expression *exp, struct agent_expr *ax,
			     struct axs_value *value);
static int type_wider_than (struct type *type1, struct type *type2);
static struct type *max_type (struct type *type1, struct type *type2);
static void gen_conversion (struct agent_expr *ax,
			    struct type *from, struct type *to);
static int is_nontrivial_conversion (struct type *from, struct type *to);
static void gen_usual_arithmetic (struct expression *exp,
				  struct agent_expr *ax,
				  struct axs_value *value1,
				  struct axs_value *value2);
static void gen_integral_promotions (struct expression *exp,
				     struct agent_expr *ax,
				     struct axs_value *value);
static void gen_cast (struct agent_expr *ax,
		      struct axs_value *value, struct type *type);
static void gen_scale (struct agent_expr *ax,
		       enum agent_op op, struct type *type);
static void gen_ptradd (struct agent_expr *ax, struct axs_value *value,
			struct axs_value *value1, struct axs_value *value2);
static void gen_ptrsub (struct agent_expr *ax, struct axs_value *value,
			struct axs_value *value1, struct axs_value *value2);
static void gen_ptrdiff (struct agent_expr *ax, struct axs_value *value,
			 struct axs_value *value1, struct axs_value *value2,
			 struct type *result_type);
static void gen_binop (struct agent_expr *ax,
		       struct axs_value *value,
		       struct axs_value *value1,
		       struct axs_value *value2,
		       enum agent_op op,
		       enum agent_op op_unsigned, int may_carry, char *name);
static void gen_logical_not (struct agent_expr *ax, struct axs_value *value,
			     struct type *result_type);
static void gen_complement (struct agent_expr *ax, struct axs_value *value);
static void gen_deref (struct agent_expr *, struct axs_value *);
static void gen_address_of (struct agent_expr *, struct axs_value *);
static void gen_bitfield_ref (struct expression *exp, struct agent_expr *ax,
			      struct axs_value *value,
			      struct type *type, int start, int end);
static void gen_primitive_field (struct expression *exp,
				 struct agent_expr *ax,
				 struct axs_value *value,
				 int offset, int fieldno, struct type *type);
static int gen_struct_ref_recursive (struct expression *exp,
				     struct agent_expr *ax,
				     struct axs_value *value,
				     char *field, int offset,
				     struct type *type);
static void gen_struct_ref (struct expression *exp, struct agent_expr *ax,
			    struct axs_value *value,
			    char *field,
			    char *operator_name, char *operand_name);
static void gen_static_field (struct gdbarch *gdbarch,
			      struct agent_expr *ax, struct axs_value *value,
			      struct type *type, int fieldno);
static void gen_repeat (struct expression *exp, union exp_element **pc,
			struct agent_expr *ax, struct axs_value *value);
static void gen_sizeof (struct expression *exp, union exp_element **pc,
			struct agent_expr *ax, struct axs_value *value,
			struct type *size_type);
static void gen_expr_binop_rest (struct expression *exp,
				 enum exp_opcode op, union exp_element **pc,
				 struct agent_expr *ax,
				 struct axs_value *value,
				 struct axs_value *value1,
				 struct axs_value *value2);

static void agent_command (char *exp, int from_tty);


/* Detecting constant expressions.  */

/* If the variable reference at *PC is a constant, return its value.
   Otherwise, return zero.

   Hey, Wally!  How can a variable reference be a constant?

   Well, Beav, this function really handles the OP_VAR_VALUE operator,
   not specifically variable references.  GDB uses OP_VAR_VALUE to
   refer to any kind of symbolic reference: function names, enum
   elements, and goto labels are all handled through the OP_VAR_VALUE
   operator, even though they're constants.  It makes sense given the
   situation.

   Gee, Wally, don'cha wonder sometimes if data representations that
   subvert commonly accepted definitions of terms in favor of heavily
   context-specific interpretations are really just a tool of the
   programming hegemony to preserve their power and exclude the
   proletariat?  */

static struct value *
const_var_ref (struct symbol *var)
{
  struct type *type = SYMBOL_TYPE (var);

  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:
      return value_from_longest (type, (LONGEST) SYMBOL_VALUE (var));

    case LOC_LABEL:
      return value_from_pointer (type, (CORE_ADDR) SYMBOL_VALUE_ADDRESS (var));

    default:
      return 0;
    }
}


/* If the expression starting at *PC has a constant value, return it.
   Otherwise, return zero.  If we return a value, then *PC will be
   advanced to the end of it.  If we return zero, *PC could be
   anywhere.  */
static struct value *
const_expr (union exp_element **pc)
{
  enum exp_opcode op = (*pc)->opcode;
  struct value *v1;

  switch (op)
    {
    case OP_LONG:
      {
	struct type *type = (*pc)[1].type;
	LONGEST k = (*pc)[2].longconst;

	(*pc) += 4;
	return value_from_longest (type, k);
      }

    case OP_VAR_VALUE:
      {
	struct value *v = const_var_ref ((*pc)[2].symbol);

	(*pc) += 4;
	return v;
      }

      /* We could add more operators in here.  */

    case UNOP_NEG:
      (*pc)++;
      v1 = const_expr (pc);
      if (v1)
	return value_neg (v1);
      else
	return 0;

    default:
      return 0;
    }
}


/* Like const_expr, but guarantee also that *PC is undisturbed if the
   expression is not constant.  */
static struct value *
maybe_const_expr (union exp_element **pc)
{
  union exp_element *tentative_pc = *pc;
  struct value *v = const_expr (&tentative_pc);

  /* If we got a value, then update the real PC.  */
  if (v)
    *pc = tentative_pc;

  return v;
}


/* Generating bytecode from GDB expressions: general assumptions */

/* Here are a few general assumptions made throughout the code; if you
   want to make a change that contradicts one of these, then you'd
   better scan things pretty thoroughly.

   - We assume that all values occupy one stack element.  For example,
   sometimes we'll swap to get at the left argument to a binary
   operator.  If we decide that void values should occupy no stack
   elements, or that synthetic arrays (whose size is determined at
   run time, created by the `@' operator) should occupy two stack
   elements (address and length), then this will cause trouble.

   - We assume the stack elements are infinitely wide, and that we
   don't have to worry what happens if the user requests an
   operation that is wider than the actual interpreter's stack.
   That is, it's up to the interpreter to handle directly all the
   integer widths the user has access to.  (Woe betide the language
   with bignums!)

   - We don't support side effects.  Thus, we don't have to worry about
   GCC's generalized lvalues, function calls, etc.

   - We don't support floating point.  Many places where we switch on
   some type don't bother to include cases for floating point; there
   may be even more subtle ways this assumption exists.  For
   example, the arguments to % must be integers.

   - We assume all subexpressions have a static, unchanging type.  If
   we tried to support convenience variables, this would be a
   problem.

   - All values on the stack should always be fully zero- or
   sign-extended.

   (I wasn't sure whether to choose this or its opposite --- that
   only addresses are assumed extended --- but it turns out that
   neither convention completely eliminates spurious extend
   operations (if everything is always extended, then you have to
   extend after add, because it could overflow; if nothing is
   extended, then you end up producing extends whenever you change
   sizes), and this is simpler.)  */


/* Scan for all static fields in the given class, including any base
   classes, and generate tracing bytecodes for each.  */

static void
gen_trace_static_fields (struct gdbarch *gdbarch,
			 struct agent_expr *ax,
			 struct type *type)
{
  int i, nbases = TYPE_N_BASECLASSES (type);
  struct axs_value value;

  CHECK_TYPEDEF (type);

  for (i = TYPE_NFIELDS (type) - 1; i >= nbases; i--)
    {
      if (field_is_static (&TYPE_FIELD (type, i)))
	{
	  gen_static_field (gdbarch, ax, &value, type, i);
	  if (value.optimized_out)
	    continue;
	  switch (value.kind)
	    {
	    case axs_lvalue_memory:
	      {
	        /* Initialize the TYPE_LENGTH if it is a typedef.  */
	        check_typedef (value.type);
		ax_const_l (ax, TYPE_LENGTH (value.type));
		ax_simple (ax, aop_trace);
	      }
	      break;

	    case axs_lvalue_register:
	      /* We don't actually need the register's value to be pushed,
		 just note that we need it to be collected.  */
	      ax_reg_mask (ax, value.u.reg);

	    default:
	      break;
	    }
	}
    }

  /* Now scan through base classes recursively.  */
  for (i = 0; i < nbases; i++)
    {
      struct type *basetype = check_typedef (TYPE_BASECLASS (type, i));

      gen_trace_static_fields (gdbarch, ax, basetype);
    }
}

/* Trace the lvalue on the stack, if it needs it.  In either case, pop
   the value.  Useful on the left side of a comma, and at the end of
   an expression being used for tracing.  */
static void
gen_traced_pop (struct gdbarch *gdbarch,
		struct agent_expr *ax, struct axs_value *value)
{
  int string_trace = 0;
  if (ax->trace_string
      && TYPE_CODE (value->type) == TYPE_CODE_PTR
      && c_textual_element_type (check_typedef (TYPE_TARGET_TYPE (value->type)),
				 's'))
    string_trace = 1;

  if (ax->tracing)
    switch (value->kind)
      {
      case axs_rvalue:
	if (string_trace)
	  {
	    ax_const_l (ax, ax->trace_string);
	    ax_simple (ax, aop_tracenz);
	  }
	else
	  /* We don't trace rvalues, just the lvalues necessary to
	     produce them.  So just dispose of this value.  */
	  ax_simple (ax, aop_pop);
	break;

      case axs_lvalue_memory:
	{
	  if (string_trace)
	    ax_simple (ax, aop_dup);

	  /* Initialize the TYPE_LENGTH if it is a typedef.  */
	  check_typedef (value->type);

	  /* There's no point in trying to use a trace_quick bytecode
	     here, since "trace_quick SIZE pop" is three bytes, whereas
	     "const8 SIZE trace" is also three bytes, does the same
	     thing, and the simplest code which generates that will also
	     work correctly for objects with large sizes.  */
	  ax_const_l (ax, TYPE_LENGTH (value->type));
	  ax_simple (ax, aop_trace);

	  if (string_trace)
	    {
	      ax_simple (ax, aop_ref32);
	      ax_const_l (ax, ax->trace_string);
	      ax_simple (ax, aop_tracenz);
	    }
	}
	break;

      case axs_lvalue_register:
	/* We don't actually need the register's value to be on the
	   stack, and the target will get heartburn if the register is
	   larger than will fit in a stack, so just mark it for
	   collection and be done with it.  */
	ax_reg_mask (ax, value->u.reg);
       
	/* But if the register points to a string, assume the value
	   will fit on the stack and push it anyway.  */
	if (string_trace)
	  {
	    ax_reg (ax, value->u.reg);
	    ax_const_l (ax, ax->trace_string);
	    ax_simple (ax, aop_tracenz);
	  }
	break;
      }
  else
    /* If we're not tracing, just pop the value.  */
    ax_simple (ax, aop_pop);

  /* To trace C++ classes with static fields stored elsewhere.  */
  if (ax->tracing
      && (TYPE_CODE (value->type) == TYPE_CODE_STRUCT
	  || TYPE_CODE (value->type) == TYPE_CODE_UNION))
    gen_trace_static_fields (gdbarch, ax, value->type);
}



/* Generating bytecode from GDB expressions: helper functions */

/* Assume that the lower bits of the top of the stack is a value of
   type TYPE, and the upper bits are zero.  Sign-extend if necessary.  */
static void
gen_sign_extend (struct agent_expr *ax, struct type *type)
{
  /* Do we need to sign-extend this?  */
  if (!TYPE_UNSIGNED (type))
    ax_ext (ax, TYPE_LENGTH (type) * TARGET_CHAR_BIT);
}


/* Assume the lower bits of the top of the stack hold a value of type
   TYPE, and the upper bits are garbage.  Sign-extend or truncate as
   needed.  */
static void
gen_extend (struct agent_expr *ax, struct type *type)
{
  int bits = TYPE_LENGTH (type) * TARGET_CHAR_BIT;

  /* I just had to.  */
  ((TYPE_UNSIGNED (type) ? ax_zero_ext : ax_ext) (ax, bits));
}


/* Assume that the top of the stack contains a value of type "pointer
   to TYPE"; generate code to fetch its value.  Note that TYPE is the
   target type, not the pointer type.  */
static void
gen_fetch (struct agent_expr *ax, struct type *type)
{
  if (ax->tracing)
    {
      /* Record the area of memory we're about to fetch.  */
      ax_trace_quick (ax, TYPE_LENGTH (type));
    }

  if (TYPE_CODE (type) == TYPE_CODE_RANGE)
    type = TYPE_TARGET_TYPE (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
      /* It's a scalar value, so we know how to dereference it.  How
         many bytes long is it?  */
      switch (TYPE_LENGTH (type))
	{
	case 8 / TARGET_CHAR_BIT:
	  ax_simple (ax, aop_ref8);
	  break;
	case 16 / TARGET_CHAR_BIT:
	  ax_simple (ax, aop_ref16);
	  break;
	case 32 / TARGET_CHAR_BIT:
	  ax_simple (ax, aop_ref32);
	  break;
	case 64 / TARGET_CHAR_BIT:
	  ax_simple (ax, aop_ref64);
	  break;

	  /* Either our caller shouldn't have asked us to dereference
	     that pointer (other code's fault), or we're not
	     implementing something we should be (this code's fault).
	     In any case, it's a bug the user shouldn't see.  */
	default:
	  internal_error (__FILE__, __LINE__,
			  _("gen_fetch: strange size"));
	}

      gen_sign_extend (ax, type);
      break;

    default:
      /* Our caller requested us to dereference a pointer from an unsupported
	 type.  Error out and give callers a chance to handle the failure
	 gracefully.  */
      error (_("gen_fetch: Unsupported type code `%s'."),
	     TYPE_NAME (type));
    }
}


/* Generate code to left shift the top of the stack by DISTANCE bits, or
   right shift it by -DISTANCE bits if DISTANCE < 0.  This generates
   unsigned (logical) right shifts.  */
static void
gen_left_shift (struct agent_expr *ax, int distance)
{
  if (distance > 0)
    {
      ax_const_l (ax, distance);
      ax_simple (ax, aop_lsh);
    }
  else if (distance < 0)
    {
      ax_const_l (ax, -distance);
      ax_simple (ax, aop_rsh_unsigned);
    }
}



/* Generating bytecode from GDB expressions: symbol references */

/* Generate code to push the base address of the argument portion of
   the top stack frame.  */
static void
gen_frame_args_address (struct gdbarch *gdbarch, struct agent_expr *ax)
{
  int frame_reg;
  LONGEST frame_offset;

  gdbarch_virtual_frame_pointer (gdbarch,
				 ax->scope, &frame_reg, &frame_offset);
  ax_reg (ax, frame_reg);
  gen_offset (ax, frame_offset);
}


/* Generate code to push the base address of the locals portion of the
   top stack frame.  */
static void
gen_frame_locals_address (struct gdbarch *gdbarch, struct agent_expr *ax)
{
  int frame_reg;
  LONGEST frame_offset;

  gdbarch_virtual_frame_pointer (gdbarch,
				 ax->scope, &frame_reg, &frame_offset);
  ax_reg (ax, frame_reg);
  gen_offset (ax, frame_offset);
}


/* Generate code to add OFFSET to the top of the stack.  Try to
   generate short and readable code.  We use this for getting to
   variables on the stack, and structure members.  If we were
   programming in ML, it would be clearer why these are the same
   thing.  */
static void
gen_offset (struct agent_expr *ax, int offset)
{
  /* It would suffice to simply push the offset and add it, but this
     makes it easier to read positive and negative offsets in the
     bytecode.  */
  if (offset > 0)
    {
      ax_const_l (ax, offset);
      ax_simple (ax, aop_add);
    }
  else if (offset < 0)
    {
      ax_const_l (ax, -offset);
      ax_simple (ax, aop_sub);
    }
}


/* In many cases, a symbol's value is the offset from some other
   address (stack frame, base register, etc.)  Generate code to add
   VAR's value to the top of the stack.  */
static void
gen_sym_offset (struct agent_expr *ax, struct symbol *var)
{
  gen_offset (ax, SYMBOL_VALUE (var));
}


/* Generate code for a variable reference to AX.  The variable is the
   symbol VAR.  Set VALUE to describe the result.  */

static void
gen_var_ref (struct gdbarch *gdbarch, struct agent_expr *ax,
	     struct axs_value *value, struct symbol *var)
{
  /* Dereference any typedefs.  */
  value->type = check_typedef (SYMBOL_TYPE (var));
  value->optimized_out = 0;

  if (SYMBOL_COMPUTED_OPS (var) != NULL)
    {
      SYMBOL_COMPUTED_OPS (var)->tracepoint_var_ref (var, gdbarch, ax, value);
      return;
    }

  /* I'm imitating the code in read_var_value.  */
  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:		/* A constant, like an enum value.  */
      ax_const_l (ax, (LONGEST) SYMBOL_VALUE (var));
      value->kind = axs_rvalue;
      break;

    case LOC_LABEL:		/* A goto label, being used as a value.  */
      ax_const_l (ax, (LONGEST) SYMBOL_VALUE_ADDRESS (var));
      value->kind = axs_rvalue;
      break;

    case LOC_CONST_BYTES:
      internal_error (__FILE__, __LINE__,
		      _("gen_var_ref: LOC_CONST_BYTES "
			"symbols are not supported"));

      /* Variable at a fixed location in memory.  Easy.  */
    case LOC_STATIC:
      /* Push the address of the variable.  */
      ax_const_l (ax, SYMBOL_VALUE_ADDRESS (var));
      value->kind = axs_lvalue_memory;
      break;

    case LOC_ARG:		/* var lives in argument area of frame */
      gen_frame_args_address (gdbarch, ax);
      gen_sym_offset (ax, var);
      value->kind = axs_lvalue_memory;
      break;

    case LOC_REF_ARG:		/* As above, but the frame slot really
				   holds the address of the variable.  */
      gen_frame_args_address (gdbarch, ax);
      gen_sym_offset (ax, var);
      /* Don't assume any particular pointer size.  */
      gen_fetch (ax, builtin_type (gdbarch)->builtin_data_ptr);
      value->kind = axs_lvalue_memory;
      break;

    case LOC_LOCAL:		/* var lives in locals area of frame */
      gen_frame_locals_address (gdbarch, ax);
      gen_sym_offset (ax, var);
      value->kind = axs_lvalue_memory;
      break;

    case LOC_TYPEDEF:
      error (_("Cannot compute value of typedef `%s'."),
	     SYMBOL_PRINT_NAME (var));
      break;

    case LOC_BLOCK:
      ax_const_l (ax, BLOCK_START (SYMBOL_BLOCK_VALUE (var)));
      value->kind = axs_rvalue;
      break;

    case LOC_REGISTER:
      /* Don't generate any code at all; in the process of treating
         this as an lvalue or rvalue, the caller will generate the
         right code.  */
      value->kind = axs_lvalue_register;
      value->u.reg = SYMBOL_REGISTER_OPS (var)->register_number (var, gdbarch);
      break;

      /* A lot like LOC_REF_ARG, but the pointer lives directly in a
         register, not on the stack.  Simpler than LOC_REGISTER
         because it's just like any other case where the thing
	 has a real address.  */
    case LOC_REGPARM_ADDR:
      ax_reg (ax, SYMBOL_REGISTER_OPS (var)->register_number (var, gdbarch));
      value->kind = axs_lvalue_memory;
      break;

    case LOC_UNRESOLVED:
      {
	struct minimal_symbol *msym
	  = lookup_minimal_symbol (SYMBOL_LINKAGE_NAME (var), NULL, NULL);

	if (!msym)
	  error (_("Couldn't resolve symbol `%s'."), SYMBOL_PRINT_NAME (var));

	/* Push the address of the variable.  */
	ax_const_l (ax, SYMBOL_VALUE_ADDRESS (msym));
	value->kind = axs_lvalue_memory;
      }
      break;

    case LOC_COMPUTED:
      gdb_assert_not_reached (_("LOC_COMPUTED variable missing a method"));

    case LOC_OPTIMIZED_OUT:
      /* Flag this, but don't say anything; leave it up to callers to
	 warn the user.  */
      value->optimized_out = 1;
      break;

    default:
      error (_("Cannot find value of botched symbol `%s'."),
	     SYMBOL_PRINT_NAME (var));
      break;
    }
}



/* Generating bytecode from GDB expressions: literals */

static void
gen_int_literal (struct agent_expr *ax, struct axs_value *value, LONGEST k,
		 struct type *type)
{
  ax_const_l (ax, k);
  value->kind = axs_rvalue;
  value->type = check_typedef (type);
}



/* Generating bytecode from GDB expressions: unary conversions, casts */

/* Take what's on the top of the stack (as described by VALUE), and
   try to make an rvalue out of it.  Signal an error if we can't do
   that.  */
void
require_rvalue (struct agent_expr *ax, struct axs_value *value)
{
  /* Only deal with scalars, structs and such may be too large
     to fit in a stack entry.  */
  value->type = check_typedef (value->type);
  if (TYPE_CODE (value->type) == TYPE_CODE_ARRAY
      || TYPE_CODE (value->type) == TYPE_CODE_STRUCT
      || TYPE_CODE (value->type) == TYPE_CODE_UNION
      || TYPE_CODE (value->type) == TYPE_CODE_FUNC)
    error (_("Value not scalar: cannot be an rvalue."));

  switch (value->kind)
    {
    case axs_rvalue:
      /* It's already an rvalue.  */
      break;

    case axs_lvalue_memory:
      /* The top of stack is the address of the object.  Dereference.  */
      gen_fetch (ax, value->type);
      break;

    case axs_lvalue_register:
      /* There's nothing on the stack, but value->u.reg is the
         register number containing the value.

         When we add floating-point support, this is going to have to
         change.  What about SPARC register pairs, for example?  */
      ax_reg (ax, value->u.reg);
      gen_extend (ax, value->type);
      break;
    }

  value->kind = axs_rvalue;
}


/* Assume the top of the stack is described by VALUE, and perform the
   usual unary conversions.  This is motivated by ANSI 6.2.2, but of
   course GDB expressions are not ANSI; they're the mishmash union of
   a bunch of languages.  Rah.

   NOTE!  This function promises to produce an rvalue only when the
   incoming value is of an appropriate type.  In other words, the
   consumer of the value this function produces may assume the value
   is an rvalue only after checking its type.

   The immediate issue is that if the user tries to use a structure or
   union as an operand of, say, the `+' operator, we don't want to try
   to convert that structure to an rvalue; require_rvalue will bomb on
   structs and unions.  Rather, we want to simply pass the struct
   lvalue through unchanged, and let `+' raise an error.  */

static void
gen_usual_unary (struct expression *exp, struct agent_expr *ax,
		 struct axs_value *value)
{
  /* We don't have to generate any code for the usual integral
     conversions, since values are always represented as full-width on
     the stack.  Should we tweak the type?  */

  /* Some types require special handling.  */
  switch (TYPE_CODE (value->type))
    {
      /* Functions get converted to a pointer to the function.  */
    case TYPE_CODE_FUNC:
      value->type = lookup_pointer_type (value->type);
      value->kind = axs_rvalue;	/* Should always be true, but just in case.  */
      break;

      /* Arrays get converted to a pointer to their first element, and
         are no longer an lvalue.  */
    case TYPE_CODE_ARRAY:
      {
	struct type *elements = TYPE_TARGET_TYPE (value->type);

	value->type = lookup_pointer_type (elements);
	value->kind = axs_rvalue;
	/* We don't need to generate any code; the address of the array
	   is also the address of its first element.  */
      }
      break;

      /* Don't try to convert structures and unions to rvalues.  Let the
         consumer signal an error.  */
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return;
    }

  /* If the value is an lvalue, dereference it.  */
  require_rvalue (ax, value);
}


/* Return non-zero iff the type TYPE1 is considered "wider" than the
   type TYPE2, according to the rules described in gen_usual_arithmetic.  */
static int
type_wider_than (struct type *type1, struct type *type2)
{
  return (TYPE_LENGTH (type1) > TYPE_LENGTH (type2)
	  || (TYPE_LENGTH (type1) == TYPE_LENGTH (type2)
	      && TYPE_UNSIGNED (type1)
	      && !TYPE_UNSIGNED (type2)));
}


/* Return the "wider" of the two types TYPE1 and TYPE2.  */
static struct type *
max_type (struct type *type1, struct type *type2)
{
  return type_wider_than (type1, type2) ? type1 : type2;
}


/* Generate code to convert a scalar value of type FROM to type TO.  */
static void
gen_conversion (struct agent_expr *ax, struct type *from, struct type *to)
{
  /* Perhaps there is a more graceful way to state these rules.  */

  /* If we're converting to a narrower type, then we need to clear out
     the upper bits.  */
  if (TYPE_LENGTH (to) < TYPE_LENGTH (from))
    gen_extend (ax, from);

  /* If the two values have equal width, but different signednesses,
     then we need to extend.  */
  else if (TYPE_LENGTH (to) == TYPE_LENGTH (from))
    {
      if (TYPE_UNSIGNED (from) != TYPE_UNSIGNED (to))
	gen_extend (ax, to);
    }

  /* If we're converting to a wider type, and becoming unsigned, then
     we need to zero out any possible sign bits.  */
  else if (TYPE_LENGTH (to) > TYPE_LENGTH (from))
    {
      if (TYPE_UNSIGNED (to))
	gen_extend (ax, to);
    }
}


/* Return non-zero iff the type FROM will require any bytecodes to be
   emitted to be converted to the type TO.  */
static int
is_nontrivial_conversion (struct type *from, struct type *to)
{
  struct agent_expr *ax = new_agent_expr (NULL, 0);
  int nontrivial;

  /* Actually generate the code, and see if anything came out.  At the
     moment, it would be trivial to replicate the code in
     gen_conversion here, but in the future, when we're supporting
     floating point and the like, it may not be.  Doing things this
     way allows this function to be independent of the logic in
     gen_conversion.  */
  gen_conversion (ax, from, to);
  nontrivial = ax->len > 0;
  free_agent_expr (ax);
  return nontrivial;
}


/* Generate code to perform the "usual arithmetic conversions" (ANSI C
   6.2.1.5) for the two operands of an arithmetic operator.  This
   effectively finds a "least upper bound" type for the two arguments,
   and promotes each argument to that type.  *VALUE1 and *VALUE2
   describe the values as they are passed in, and as they are left.  */
static void
gen_usual_arithmetic (struct expression *exp, struct agent_expr *ax,
		      struct axs_value *value1, struct axs_value *value2)
{
  /* Do the usual binary conversions.  */
  if (TYPE_CODE (value1->type) == TYPE_CODE_INT
      && TYPE_CODE (value2->type) == TYPE_CODE_INT)
    {
      /* The ANSI integral promotions seem to work this way: Order the
         integer types by size, and then by signedness: an n-bit
         unsigned type is considered "wider" than an n-bit signed
         type.  Promote to the "wider" of the two types, and always
         promote at least to int.  */
      struct type *target = max_type (builtin_type (exp->gdbarch)->builtin_int,
				      max_type (value1->type, value2->type));

      /* Deal with value2, on the top of the stack.  */
      gen_conversion (ax, value2->type, target);

      /* Deal with value1, not on the top of the stack.  Don't
         generate the `swap' instructions if we're not actually going
         to do anything.  */
      if (is_nontrivial_conversion (value1->type, target))
	{
	  ax_simple (ax, aop_swap);
	  gen_conversion (ax, value1->type, target);
	  ax_simple (ax, aop_swap);
	}

      value1->type = value2->type = check_typedef (target);
    }
}


/* Generate code to perform the integral promotions (ANSI 6.2.1.1) on
   the value on the top of the stack, as described by VALUE.  Assume
   the value has integral type.  */
static void
gen_integral_promotions (struct expression *exp, struct agent_expr *ax,
			 struct axs_value *value)
{
  const struct builtin_type *builtin = builtin_type (exp->gdbarch);

  if (!type_wider_than (value->type, builtin->builtin_int))
    {
      gen_conversion (ax, value->type, builtin->builtin_int);
      value->type = builtin->builtin_int;
    }
  else if (!type_wider_than (value->type, builtin->builtin_unsigned_int))
    {
      gen_conversion (ax, value->type, builtin->builtin_unsigned_int);
      value->type = builtin->builtin_unsigned_int;
    }
}


/* Generate code for a cast to TYPE.  */
static void
gen_cast (struct agent_expr *ax, struct axs_value *value, struct type *type)
{
  /* GCC does allow casts to yield lvalues, so this should be fixed
     before merging these changes into the trunk.  */
  require_rvalue (ax, value);
  /* Dereference typedefs.  */
  type = check_typedef (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      /* It's implementation-defined, and I'll bet this is what GCC
         does.  */
      break;

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FUNC:
      error (_("Invalid type cast: intended type must be scalar."));

    case TYPE_CODE_ENUM:
    case TYPE_CODE_BOOL:
      /* We don't have to worry about the size of the value, because
         all our integral values are fully sign-extended, and when
         casting pointers we can do anything we like.  Is there any
         way for us to know what GCC actually does with a cast like
         this?  */
      break;

    case TYPE_CODE_INT:
      gen_conversion (ax, value->type, type);
      break;

    case TYPE_CODE_VOID:
      /* We could pop the value, and rely on everyone else to check
         the type and notice that this value doesn't occupy a stack
         slot.  But for now, leave the value on the stack, and
         preserve the "value == stack element" assumption.  */
      break;

    default:
      error (_("Casts to requested type are not yet implemented."));
    }

  value->type = type;
}



/* Generating bytecode from GDB expressions: arithmetic */

/* Scale the integer on the top of the stack by the size of the target
   of the pointer type TYPE.  */
static void
gen_scale (struct agent_expr *ax, enum agent_op op, struct type *type)
{
  struct type *element = TYPE_TARGET_TYPE (type);

  if (TYPE_LENGTH (element) != 1)
    {
      ax_const_l (ax, TYPE_LENGTH (element));
      ax_simple (ax, op);
    }
}


/* Generate code for pointer arithmetic PTR + INT.  */
static void
gen_ptradd (struct agent_expr *ax, struct axs_value *value,
	    struct axs_value *value1, struct axs_value *value2)
{
  gdb_assert (pointer_type (value1->type));
  gdb_assert (TYPE_CODE (value2->type) == TYPE_CODE_INT);

  gen_scale (ax, aop_mul, value1->type);
  ax_simple (ax, aop_add);
  gen_extend (ax, value1->type);	/* Catch overflow.  */
  value->type = value1->type;
  value->kind = axs_rvalue;
}


/* Generate code for pointer arithmetic PTR - INT.  */
static void
gen_ptrsub (struct agent_expr *ax, struct axs_value *value,
	    struct axs_value *value1, struct axs_value *value2)
{
  gdb_assert (pointer_type (value1->type));
  gdb_assert (TYPE_CODE (value2->type) == TYPE_CODE_INT);

  gen_scale (ax, aop_mul, value1->type);
  ax_simple (ax, aop_sub);
  gen_extend (ax, value1->type);	/* Catch overflow.  */
  value->type = value1->type;
  value->kind = axs_rvalue;
}


/* Generate code for pointer arithmetic PTR - PTR.  */
static void
gen_ptrdiff (struct agent_expr *ax, struct axs_value *value,
	     struct axs_value *value1, struct axs_value *value2,
	     struct type *result_type)
{
  gdb_assert (pointer_type (value1->type));
  gdb_assert (pointer_type (value2->type));

  if (TYPE_LENGTH (TYPE_TARGET_TYPE (value1->type))
      != TYPE_LENGTH (TYPE_TARGET_TYPE (value2->type)))
    error (_("\
First argument of `-' is a pointer, but second argument is neither\n\
an integer nor a pointer of the same type."));

  ax_simple (ax, aop_sub);
  gen_scale (ax, aop_div_unsigned, value1->type);
  value->type = result_type;
  value->kind = axs_rvalue;
}

static void
gen_equal (struct agent_expr *ax, struct axs_value *value,
	   struct axs_value *value1, struct axs_value *value2,
	   struct type *result_type)
{
  if (pointer_type (value1->type) || pointer_type (value2->type))
    ax_simple (ax, aop_equal);
  else
    gen_binop (ax, value, value1, value2,
	       aop_equal, aop_equal, 0, "equal");
  value->type = result_type;
  value->kind = axs_rvalue;
}

static void
gen_less (struct agent_expr *ax, struct axs_value *value,
	  struct axs_value *value1, struct axs_value *value2,
	  struct type *result_type)
{
  if (pointer_type (value1->type) || pointer_type (value2->type))
    ax_simple (ax, aop_less_unsigned);
  else
    gen_binop (ax, value, value1, value2,
	       aop_less_signed, aop_less_unsigned, 0, "less than");
  value->type = result_type;
  value->kind = axs_rvalue;
}

/* Generate code for a binary operator that doesn't do pointer magic.
   We set VALUE to describe the result value; we assume VALUE1 and
   VALUE2 describe the two operands, and that they've undergone the
   usual binary conversions.  MAY_CARRY should be non-zero iff the
   result needs to be extended.  NAME is the English name of the
   operator, used in error messages */
static void
gen_binop (struct agent_expr *ax, struct axs_value *value,
	   struct axs_value *value1, struct axs_value *value2,
	   enum agent_op op, enum agent_op op_unsigned,
	   int may_carry, char *name)
{
  /* We only handle INT op INT.  */
  if ((TYPE_CODE (value1->type) != TYPE_CODE_INT)
      || (TYPE_CODE (value2->type) != TYPE_CODE_INT))
    error (_("Invalid combination of types in %s."), name);

  ax_simple (ax,
	     TYPE_UNSIGNED (value1->type) ? op_unsigned : op);
  if (may_carry)
    gen_extend (ax, value1->type);	/* catch overflow */
  value->type = value1->type;
  value->kind = axs_rvalue;
}


static void
gen_logical_not (struct agent_expr *ax, struct axs_value *value,
		 struct type *result_type)
{
  if (TYPE_CODE (value->type) != TYPE_CODE_INT
      && TYPE_CODE (value->type) != TYPE_CODE_PTR)
    error (_("Invalid type of operand to `!'."));

  ax_simple (ax, aop_log_not);
  value->type = result_type;
}


static void
gen_complement (struct agent_expr *ax, struct axs_value *value)
{
  if (TYPE_CODE (value->type) != TYPE_CODE_INT)
    error (_("Invalid type of operand to `~'."));

  ax_simple (ax, aop_bit_not);
  gen_extend (ax, value->type);
}



/* Generating bytecode from GDB expressions: * & . -> @ sizeof */

/* Dereference the value on the top of the stack.  */
static void
gen_deref (struct agent_expr *ax, struct axs_value *value)
{
  /* The caller should check the type, because several operators use
     this, and we don't know what error message to generate.  */
  if (!pointer_type (value->type))
    internal_error (__FILE__, __LINE__,
		    _("gen_deref: expected a pointer"));

  /* We've got an rvalue now, which is a pointer.  We want to yield an
     lvalue, whose address is exactly that pointer.  So we don't
     actually emit any code; we just change the type from "Pointer to
     T" to "T", and mark the value as an lvalue in memory.  Leave it
     to the consumer to actually dereference it.  */
  value->type = check_typedef (TYPE_TARGET_TYPE (value->type));
  if (TYPE_CODE (value->type) == TYPE_CODE_VOID)
    error (_("Attempt to dereference a generic pointer."));
  value->kind = ((TYPE_CODE (value->type) == TYPE_CODE_FUNC)
		 ? axs_rvalue : axs_lvalue_memory);
}


/* Produce the address of the lvalue on the top of the stack.  */
static void
gen_address_of (struct agent_expr *ax, struct axs_value *value)
{
  /* Special case for taking the address of a function.  The ANSI
     standard describes this as a special case, too, so this
     arrangement is not without motivation.  */
  if (TYPE_CODE (value->type) == TYPE_CODE_FUNC)
    /* The value's already an rvalue on the stack, so we just need to
       change the type.  */
    value->type = lookup_pointer_type (value->type);
  else
    switch (value->kind)
      {
      case axs_rvalue:
	error (_("Operand of `&' is an rvalue, which has no address."));

      case axs_lvalue_register:
	error (_("Operand of `&' is in a register, and has no address."));

      case axs_lvalue_memory:
	value->kind = axs_rvalue;
	value->type = lookup_pointer_type (value->type);
	break;
      }
}

/* Generate code to push the value of a bitfield of a structure whose
   address is on the top of the stack.  START and END give the
   starting and one-past-ending *bit* numbers of the field within the
   structure.  */
static void
gen_bitfield_ref (struct expression *exp, struct agent_expr *ax,
		  struct axs_value *value, struct type *type,
		  int start, int end)
{
  /* Note that ops[i] fetches 8 << i bits.  */
  static enum agent_op ops[]
    = {aop_ref8, aop_ref16, aop_ref32, aop_ref64};
  static int num_ops = (sizeof (ops) / sizeof (ops[0]));

  /* We don't want to touch any byte that the bitfield doesn't
     actually occupy; we shouldn't make any accesses we're not
     explicitly permitted to.  We rely here on the fact that the
     bytecode `ref' operators work on unaligned addresses.

     It takes some fancy footwork to get the stack to work the way
     we'd like.  Say we're retrieving a bitfield that requires three
     fetches.  Initially, the stack just contains the address:
     addr
     For the first fetch, we duplicate the address
     addr addr
     then add the byte offset, do the fetch, and shift and mask as
     needed, yielding a fragment of the value, properly aligned for
     the final bitwise or:
     addr frag1
     then we swap, and repeat the process:
     frag1 addr                    --- address on top
     frag1 addr addr               --- duplicate it
     frag1 addr frag2              --- get second fragment
     frag1 frag2 addr              --- swap again
     frag1 frag2 frag3             --- get third fragment
     Notice that, since the third fragment is the last one, we don't
     bother duplicating the address this time.  Now we have all the
     fragments on the stack, and we can simply `or' them together,
     yielding the final value of the bitfield.  */

  /* The first and one-after-last bits in the field, but rounded down
     and up to byte boundaries.  */
  int bound_start = (start / TARGET_CHAR_BIT) * TARGET_CHAR_BIT;
  int bound_end = (((end + TARGET_CHAR_BIT - 1)
		    / TARGET_CHAR_BIT)
		   * TARGET_CHAR_BIT);

  /* current bit offset within the structure */
  int offset;

  /* The index in ops of the opcode we're considering.  */
  int op;

  /* The number of fragments we generated in the process.  Probably
     equal to the number of `one' bits in bytesize, but who cares?  */
  int fragment_count;

  /* Dereference any typedefs.  */
  type = check_typedef (type);

  /* Can we fetch the number of bits requested at all?  */
  if ((end - start) > ((1 << num_ops) * 8))
    internal_error (__FILE__, __LINE__,
		    _("gen_bitfield_ref: bitfield too wide"));

  /* Note that we know here that we only need to try each opcode once.
     That may not be true on machines with weird byte sizes.  */
  offset = bound_start;
  fragment_count = 0;
  for (op = num_ops - 1; op >= 0; op--)
    {
      /* number of bits that ops[op] would fetch */
      int op_size = 8 << op;

      /* The stack at this point, from bottom to top, contains zero or
         more fragments, then the address.  */

      /* Does this fetch fit within the bitfield?  */
      if (offset + op_size <= bound_end)
	{
	  /* Is this the last fragment?  */
	  int last_frag = (offset + op_size == bound_end);

	  if (!last_frag)
	    ax_simple (ax, aop_dup);	/* keep a copy of the address */

	  /* Add the offset.  */
	  gen_offset (ax, offset / TARGET_CHAR_BIT);

	  if (ax->tracing)
	    {
	      /* Record the area of memory we're about to fetch.  */
	      ax_trace_quick (ax, op_size / TARGET_CHAR_BIT);
	    }

	  /* Perform the fetch.  */
	  ax_simple (ax, ops[op]);

	  /* Shift the bits we have to their proper position.
	     gen_left_shift will generate right shifts when the operand
	     is negative.

	     A big-endian field diagram to ponder:
	     byte 0  byte 1  byte 2  byte 3  byte 4  byte 5  byte 6  byte 7
	     +------++------++------++------++------++------++------++------+
	     xxxxAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBCCCCCxxxxxxxxxxx
	     ^               ^               ^    ^
	     bit number      16              32              48   53
	     These are bit numbers as supplied by GDB.  Note that the
	     bit numbers run from right to left once you've fetched the
	     value!

	     A little-endian field diagram to ponder:
	     byte 7  byte 6  byte 5  byte 4  byte 3  byte 2  byte 1  byte 0
	     +------++------++------++------++------++------++------++------+
	     xxxxxxxxxxxAAAAABBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCxxxx
	     ^               ^               ^           ^   ^
	     bit number     48              32              16          4   0

	     In both cases, the most significant end is on the left
	     (i.e. normal numeric writing order), which means that you
	     don't go crazy thinking about `left' and `right' shifts.

	     We don't have to worry about masking yet:
	     - If they contain garbage off the least significant end, then we
	     must be looking at the low end of the field, and the right
	     shift will wipe them out.
	     - If they contain garbage off the most significant end, then we
	     must be looking at the most significant end of the word, and
	     the sign/zero extension will wipe them out.
	     - If we're in the interior of the word, then there is no garbage
	     on either end, because the ref operators zero-extend.  */
	  if (gdbarch_byte_order (exp->gdbarch) == BFD_ENDIAN_BIG)
	    gen_left_shift (ax, end - (offset + op_size));
	  else
	    gen_left_shift (ax, offset - start);

	  if (!last_frag)
	    /* Bring the copy of the address up to the top.  */
	    ax_simple (ax, aop_swap);

	  offset += op_size;
	  fragment_count++;
	}
    }

  /* Generate enough bitwise `or' operations to combine all the
     fragments we left on the stack.  */
  while (fragment_count-- > 1)
    ax_simple (ax, aop_bit_or);

  /* Sign- or zero-extend the value as appropriate.  */
  ((TYPE_UNSIGNED (type) ? ax_zero_ext : ax_ext) (ax, end - start));

  /* This is *not* an lvalue.  Ugh.  */
  value->kind = axs_rvalue;
  value->type = type;
}

/* Generate bytecodes for field number FIELDNO of type TYPE.  OFFSET
   is an accumulated offset (in bytes), will be nonzero for objects
   embedded in other objects, like C++ base classes.  Behavior should
   generally follow value_primitive_field.  */

static void
gen_primitive_field (struct expression *exp,
		     struct agent_expr *ax, struct axs_value *value,
		     int offset, int fieldno, struct type *type)
{
  /* Is this a bitfield?  */
  if (TYPE_FIELD_PACKED (type, fieldno))
    gen_bitfield_ref (exp, ax, value, TYPE_FIELD_TYPE (type, fieldno),
		      (offset * TARGET_CHAR_BIT
		       + TYPE_FIELD_BITPOS (type, fieldno)),
		      (offset * TARGET_CHAR_BIT
		       + TYPE_FIELD_BITPOS (type, fieldno)
		       + TYPE_FIELD_BITSIZE (type, fieldno)));
  else
    {
      gen_offset (ax, offset
		  + TYPE_FIELD_BITPOS (type, fieldno) / TARGET_CHAR_BIT);
      value->kind = axs_lvalue_memory;
      value->type = TYPE_FIELD_TYPE (type, fieldno);
    }
}

/* Search for the given field in either the given type or one of its
   base classes.  Return 1 if found, 0 if not.  */

static int
gen_struct_ref_recursive (struct expression *exp, struct agent_expr *ax,
			  struct axs_value *value,
			  char *field, int offset, struct type *type)
{
  int i, rslt;
  int nbases = TYPE_N_BASECLASSES (type);

  CHECK_TYPEDEF (type);

  for (i = TYPE_NFIELDS (type) - 1; i >= nbases; i--)
    {
      const char *this_name = TYPE_FIELD_NAME (type, i);

      if (this_name)
	{
	  if (strcmp (field, this_name) == 0)
	    {
	      /* Note that bytecodes for the struct's base (aka
		 "this") will have been generated already, which will
		 be unnecessary but not harmful if the static field is
		 being handled as a global.  */
	      if (field_is_static (&TYPE_FIELD (type, i)))
		{
		  gen_static_field (exp->gdbarch, ax, value, type, i);
		  if (value->optimized_out)
		    error (_("static field `%s' has been "
			     "optimized out, cannot use"),
			   field);
		  return 1;
		}

	      gen_primitive_field (exp, ax, value, offset, i, type);
	      return 1;
	    }
#if 0 /* is this right? */
	  if (this_name[0] == '\0')
	    internal_error (__FILE__, __LINE__,
			    _("find_field: anonymous unions not supported"));
#endif
	}
    }

  /* Now scan through base classes recursively.  */
  for (i = 0; i < nbases; i++)
    {
      struct type *basetype = check_typedef (TYPE_BASECLASS (type, i));

      rslt = gen_struct_ref_recursive (exp, ax, value, field,
				       offset + TYPE_BASECLASS_BITPOS (type, i)
				       / TARGET_CHAR_BIT,
				       basetype);
      if (rslt)
	return 1;
    }

  /* Not found anywhere, flag so caller can complain.  */
  return 0;
}

/* Generate code to reference the member named FIELD of a structure or
   union.  The top of the stack, as described by VALUE, should have
   type (pointer to a)* struct/union.  OPERATOR_NAME is the name of
   the operator being compiled, and OPERAND_NAME is the kind of thing
   it operates on; we use them in error messages.  */
static void
gen_struct_ref (struct expression *exp, struct agent_expr *ax,
		struct axs_value *value, char *field,
		char *operator_name, char *operand_name)
{
  struct type *type;
  int found;

  /* Follow pointers until we reach a non-pointer.  These aren't the C
     semantics, but they're what the normal GDB evaluator does, so we
     should at least be consistent.  */
  while (pointer_type (value->type))
    {
      require_rvalue (ax, value);
      gen_deref (ax, value);
    }
  type = check_typedef (value->type);

  /* This must yield a structure or a union.  */
  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_UNION)
    error (_("The left operand of `%s' is not a %s."),
	   operator_name, operand_name);

  /* And it must be in memory; we don't deal with structure rvalues,
     or structures living in registers.  */
  if (value->kind != axs_lvalue_memory)
    error (_("Structure does not live in memory."));

  /* Search through fields and base classes recursively.  */
  found = gen_struct_ref_recursive (exp, ax, value, field, 0, type);
  
  if (!found)
    error (_("Couldn't find member named `%s' in struct/union/class `%s'"),
	   field, TYPE_TAG_NAME (type));
}

static int
gen_namespace_elt (struct expression *exp,
		   struct agent_expr *ax, struct axs_value *value,
		   const struct type *curtype, char *name);
static int
gen_maybe_namespace_elt (struct expression *exp,
			 struct agent_expr *ax, struct axs_value *value,
			 const struct type *curtype, char *name);

static void
gen_static_field (struct gdbarch *gdbarch,
		  struct agent_expr *ax, struct axs_value *value,
		  struct type *type, int fieldno)
{
  if (TYPE_FIELD_LOC_KIND (type, fieldno) == FIELD_LOC_KIND_PHYSADDR)
    {
      ax_const_l (ax, TYPE_FIELD_STATIC_PHYSADDR (type, fieldno));
      value->kind = axs_lvalue_memory;
      value->type = TYPE_FIELD_TYPE (type, fieldno);
      value->optimized_out = 0;
    }
  else
    {
      const char *phys_name = TYPE_FIELD_STATIC_PHYSNAME (type, fieldno);
      struct symbol *sym = lookup_symbol (phys_name, 0, VAR_DOMAIN, 0);

      if (sym)
	{
	  gen_var_ref (gdbarch, ax, value, sym);
  
	  /* Don't error if the value was optimized out, we may be
	     scanning all static fields and just want to pass over this
	     and continue with the rest.  */
	}
      else
	{
	  /* Silently assume this was optimized out; class printing
	     will let the user know why the data is missing.  */
	  value->optimized_out = 1;
	}
    }
}

static int
gen_struct_elt_for_reference (struct expression *exp,
			      struct agent_expr *ax, struct axs_value *value,
			      struct type *type, char *fieldname)
{
  struct type *t = type;
  int i;

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    internal_error (__FILE__, __LINE__,
		    _("non-aggregate type to gen_struct_elt_for_reference"));

  for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
    {
      const char *t_field_name = TYPE_FIELD_NAME (t, i);

      if (t_field_name && strcmp (t_field_name, fieldname) == 0)
	{
	  if (field_is_static (&TYPE_FIELD (t, i)))
	    {
	      gen_static_field (exp->gdbarch, ax, value, t, i);
	      if (value->optimized_out)
		error (_("static field `%s' has been "
			 "optimized out, cannot use"),
		       fieldname);
	      return 1;
	    }
	  if (TYPE_FIELD_PACKED (t, i))
	    error (_("pointers to bitfield members not allowed"));

	  /* FIXME we need a way to do "want_address" equivalent */	  

	  error (_("Cannot reference non-static field \"%s\""), fieldname);
	}
    }

  /* FIXME add other scoped-reference cases here */

  /* Do a last-ditch lookup.  */
  return gen_maybe_namespace_elt (exp, ax, value, type, fieldname);
}

/* C++: Return the member NAME of the namespace given by the type
   CURTYPE.  */

static int
gen_namespace_elt (struct expression *exp,
		   struct agent_expr *ax, struct axs_value *value,
		   const struct type *curtype, char *name)
{
  int found = gen_maybe_namespace_elt (exp, ax, value, curtype, name);

  if (!found)
    error (_("No symbol \"%s\" in namespace \"%s\"."), 
	   name, TYPE_TAG_NAME (curtype));

  return found;
}

/* A helper function used by value_namespace_elt and
   value_struct_elt_for_reference.  It looks up NAME inside the
   context CURTYPE; this works if CURTYPE is a namespace or if CURTYPE
   is a class and NAME refers to a type in CURTYPE itself (as opposed
   to, say, some base class of CURTYPE).  */

static int
gen_maybe_namespace_elt (struct expression *exp,
			 struct agent_expr *ax, struct axs_value *value,
			 const struct type *curtype, char *name)
{
  const char *namespace_name = TYPE_TAG_NAME (curtype);
  struct symbol *sym;

  sym = cp_lookup_symbol_namespace (namespace_name, name,
				    block_for_pc (ax->scope),
				    VAR_DOMAIN);

  if (sym == NULL)
    return 0;

  gen_var_ref (exp->gdbarch, ax, value, sym);

  if (value->optimized_out)
    error (_("`%s' has been optimized out, cannot use"),
	   SYMBOL_PRINT_NAME (sym));

  return 1;
}


static int
gen_aggregate_elt_ref (struct expression *exp,
		       struct agent_expr *ax, struct axs_value *value,
		       struct type *type, char *field,
		       char *operator_name, char *operand_name)
{
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return gen_struct_elt_for_reference (exp, ax, value, type, field);
      break;
    case TYPE_CODE_NAMESPACE:
      return gen_namespace_elt (exp, ax, value, type, field);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("non-aggregate type in gen_aggregate_elt_ref"));
    }

  return 0;
}

/* Generate code for GDB's magical `repeat' operator.
   LVALUE @ INT creates an array INT elements long, and whose elements
   have the same type as LVALUE, located in memory so that LVALUE is
   its first element.  For example, argv[0]@argc gives you the array
   of command-line arguments.

   Unfortunately, because we have to know the types before we actually
   have a value for the expression, we can't implement this perfectly
   without changing the type system, having values that occupy two
   stack slots, doing weird things with sizeof, etc.  So we require
   the right operand to be a constant expression.  */
static void
gen_repeat (struct expression *exp, union exp_element **pc,
	    struct agent_expr *ax, struct axs_value *value)
{
  struct axs_value value1;

  /* We don't want to turn this into an rvalue, so no conversions
     here.  */
  gen_expr (exp, pc, ax, &value1);
  if (value1.kind != axs_lvalue_memory)
    error (_("Left operand of `@' must be an object in memory."));

  /* Evaluate the length; it had better be a constant.  */
  {
    struct value *v = const_expr (pc);
    int length;

    if (!v)
      error (_("Right operand of `@' must be a "
	       "constant, in agent expressions."));
    if (TYPE_CODE (value_type (v)) != TYPE_CODE_INT)
      error (_("Right operand of `@' must be an integer."));
    length = value_as_long (v);
    if (length <= 0)
      error (_("Right operand of `@' must be positive."));

    /* The top of the stack is already the address of the object, so
       all we need to do is frob the type of the lvalue.  */
    {
      /* FIXME-type-allocation: need a way to free this type when we are
         done with it.  */
      struct type *array
	= lookup_array_range_type (value1.type, 0, length - 1);

      value->kind = axs_lvalue_memory;
      value->type = array;
    }
  }
}


/* Emit code for the `sizeof' operator.
   *PC should point at the start of the operand expression; we advance it
   to the first instruction after the operand.  */
static void
gen_sizeof (struct expression *exp, union exp_element **pc,
	    struct agent_expr *ax, struct axs_value *value,
	    struct type *size_type)
{
  /* We don't care about the value of the operand expression; we only
     care about its type.  However, in the current arrangement, the
     only way to find an expression's type is to generate code for it.
     So we generate code for the operand, and then throw it away,
     replacing it with code that simply pushes its size.  */
  int start = ax->len;

  gen_expr (exp, pc, ax, value);

  /* Throw away the code we just generated.  */
  ax->len = start;

  ax_const_l (ax, TYPE_LENGTH (value->type));
  value->kind = axs_rvalue;
  value->type = size_type;
}


/* Generating bytecode from GDB expressions: general recursive thingy  */

/* XXX: i18n */
/* A gen_expr function written by a Gen-X'er guy.
   Append code for the subexpression of EXPR starting at *POS_P to AX.  */
void
gen_expr (struct expression *exp, union exp_element **pc,
	  struct agent_expr *ax, struct axs_value *value)
{
  /* Used to hold the descriptions of operand expressions.  */
  struct axs_value value1, value2, value3;
  enum exp_opcode op = (*pc)[0].opcode, op2;
  int if1, go1, if2, go2, end;
  struct type *int_type = builtin_type (exp->gdbarch)->builtin_int;

  /* If we're looking at a constant expression, just push its value.  */
  {
    struct value *v = maybe_const_expr (pc);

    if (v)
      {
	ax_const_l (ax, value_as_long (v));
	value->kind = axs_rvalue;
	value->type = check_typedef (value_type (v));
	return;
      }
  }

  /* Otherwise, go ahead and generate code for it.  */
  switch (op)
    {
      /* Binary arithmetic operators.  */
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_LSH:
    case BINOP_RSH:
    case BINOP_SUBSCRIPT:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:
      (*pc)++;
      gen_expr (exp, pc, ax, &value1);
      gen_usual_unary (exp, ax, &value1);
      gen_expr_binop_rest (exp, op, pc, ax, value, &value1, &value2);
      break;

    case BINOP_LOGICAL_AND:
      (*pc)++;
      /* Generate the obvious sequence of tests and jumps.  */
      gen_expr (exp, pc, ax, &value1);
      gen_usual_unary (exp, ax, &value1);
      if1 = ax_goto (ax, aop_if_goto);
      go1 = ax_goto (ax, aop_goto);
      ax_label (ax, if1, ax->len);
      gen_expr (exp, pc, ax, &value2);
      gen_usual_unary (exp, ax, &value2);
      if2 = ax_goto (ax, aop_if_goto);
      go2 = ax_goto (ax, aop_goto);
      ax_label (ax, if2, ax->len);
      ax_const_l (ax, 1);
      end = ax_goto (ax, aop_goto);
      ax_label (ax, go1, ax->len);
      ax_label (ax, go2, ax->len);
      ax_const_l (ax, 0);
      ax_label (ax, end, ax->len);
      value->kind = axs_rvalue;
      value->type = int_type;
      break;

    case BINOP_LOGICAL_OR:
      (*pc)++;
      /* Generate the obvious sequence of tests and jumps.  */
      gen_expr (exp, pc, ax, &value1);
      gen_usual_unary (exp, ax, &value1);
      if1 = ax_goto (ax, aop_if_goto);
      gen_expr (exp, pc, ax, &value2);
      gen_usual_unary (exp, ax, &value2);
      if2 = ax_goto (ax, aop_if_goto);
      ax_const_l (ax, 0);
      end = ax_goto (ax, aop_goto);
      ax_label (ax, if1, ax->len);
      ax_label (ax, if2, ax->len);
      ax_const_l (ax, 1);
      ax_label (ax, end, ax->len);
      value->kind = axs_rvalue;
      value->type = int_type;
      break;

    case TERNOP_COND:
      (*pc)++;
      gen_expr (exp, pc, ax, &value1);
      gen_usual_unary (exp, ax, &value1);
      /* For (A ? B : C), it's easiest to generate subexpression
	 bytecodes in order, but if_goto jumps on true, so we invert
	 the sense of A.  Then we can do B by dropping through, and
	 jump to do C.  */
      gen_logical_not (ax, &value1, int_type);
      if1 = ax_goto (ax, aop_if_goto);
      gen_expr (exp, pc, ax, &value2);
      gen_usual_unary (exp, ax, &value2);
      end = ax_goto (ax, aop_goto);
      ax_label (ax, if1, ax->len);
      gen_expr (exp, pc, ax, &value3);
      gen_usual_unary (exp, ax, &value3);
      ax_label (ax, end, ax->len);
      /* This is arbitary - what if B and C are incompatible types? */
      value->type = value2.type;
      value->kind = value2.kind;
      break;

    case BINOP_ASSIGN:
      (*pc)++;
      if ((*pc)[0].opcode == OP_INTERNALVAR)
	{
	  char *name = internalvar_name ((*pc)[1].internalvar);
	  struct trace_state_variable *tsv;

	  (*pc) += 3;
	  gen_expr (exp, pc, ax, value);
	  tsv = find_trace_state_variable (name);
	  if (tsv)
	    {
	      ax_tsv (ax, aop_setv, tsv->number);
	      if (ax->tracing)
		ax_tsv (ax, aop_tracev, tsv->number);
	    }
	  else
	    error (_("$%s is not a trace state variable, "
		     "may not assign to it"), name);
	}
      else
	error (_("May only assign to trace state variables"));
      break;

    case BINOP_ASSIGN_MODIFY:
      (*pc)++;
      op2 = (*pc)[0].opcode;
      (*pc)++;
      (*pc)++;
      if ((*pc)[0].opcode == OP_INTERNALVAR)
	{
	  char *name = internalvar_name ((*pc)[1].internalvar);
	  struct trace_state_variable *tsv;

	  (*pc) += 3;
	  tsv = find_trace_state_variable (name);
	  if (tsv)
	    {
	      /* The tsv will be the left half of the binary operation.  */
	      ax_tsv (ax, aop_getv, tsv->number);
	      if (ax->tracing)
		ax_tsv (ax, aop_tracev, tsv->number);
	      /* Trace state variables are always 64-bit integers.  */
	      value1.kind = axs_rvalue;
	      value1.type = builtin_type (exp->gdbarch)->builtin_long_long;
	      /* Now do right half of expression.  */
	      gen_expr_binop_rest (exp, op2, pc, ax, value, &value1, &value2);
	      /* We have a result of the binary op, set the tsv.  */
	      ax_tsv (ax, aop_setv, tsv->number);
	      if (ax->tracing)
		ax_tsv (ax, aop_tracev, tsv->number);
	    }
	  else
	    error (_("$%s is not a trace state variable, "
		     "may not assign to it"), name);
	}
      else
	error (_("May only assign to trace state variables"));
      break;

      /* Note that we need to be a little subtle about generating code
         for comma.  In C, we can do some optimizations here because
         we know the left operand is only being evaluated for effect.
         However, if the tracing kludge is in effect, then we always
         need to evaluate the left hand side fully, so that all the
         variables it mentions get traced.  */
    case BINOP_COMMA:
      (*pc)++;
      gen_expr (exp, pc, ax, &value1);
      /* Don't just dispose of the left operand.  We might be tracing,
         in which case we want to emit code to trace it if it's an
         lvalue.  */
      gen_traced_pop (exp->gdbarch, ax, &value1);
      gen_expr (exp, pc, ax, value);
      /* It's the consumer's responsibility to trace the right operand.  */
      break;

    case OP_LONG:		/* some integer constant */
      {
	struct type *type = (*pc)[1].type;
	LONGEST k = (*pc)[2].longconst;

	(*pc) += 4;
	gen_int_literal (ax, value, k, type);
      }
      break;

    case OP_VAR_VALUE:
      gen_var_ref (exp->gdbarch, ax, value, (*pc)[2].symbol);

      if (value->optimized_out)
	error (_("`%s' has been optimized out, cannot use"),
	       SYMBOL_PRINT_NAME ((*pc)[2].symbol));

      (*pc) += 4;
      break;

    case OP_REGISTER:
      {
	const char *name = &(*pc)[2].string;
	int reg;

	(*pc) += 4 + BYTES_TO_EXP_ELEM ((*pc)[1].longconst + 1);
	reg = user_reg_map_name_to_regnum (exp->gdbarch, name, strlen (name));
	if (reg == -1)
	  internal_error (__FILE__, __LINE__,
			  _("Register $%s not available"), name);
	/* No support for tracing user registers yet.  */
	if (reg >= gdbarch_num_regs (exp->gdbarch)
	    + gdbarch_num_pseudo_regs (exp->gdbarch))
	  error (_("'%s' is a user-register; "
		   "GDB cannot yet trace user-register contents."),
		 name);
	value->kind = axs_lvalue_register;
	value->u.reg = reg;
	value->type = register_type (exp->gdbarch, reg);
      }
      break;

    case OP_INTERNALVAR:
      {
	struct internalvar *var = (*pc)[1].internalvar;
	const char *name = internalvar_name (var);
	struct trace_state_variable *tsv;

	(*pc) += 3;
	tsv = find_trace_state_variable (name);
	if (tsv)
	  {
	    ax_tsv (ax, aop_getv, tsv->number);
	    if (ax->tracing)
	      ax_tsv (ax, aop_tracev, tsv->number);
	    /* Trace state variables are always 64-bit integers.  */
	    value->kind = axs_rvalue;
	    value->type = builtin_type (exp->gdbarch)->builtin_long_long;
	  }
	else if (! compile_internalvar_to_ax (var, ax, value))
	  error (_("$%s is not a trace state variable; GDB agent "
		   "expressions cannot use convenience variables."), name);
      }
      break;

      /* Weirdo operator: see comments for gen_repeat for details.  */
    case BINOP_REPEAT:
      /* Note that gen_repeat handles its own argument evaluation.  */
      (*pc)++;
      gen_repeat (exp, pc, ax, value);
      break;

    case UNOP_CAST:
      {
	struct type *type = (*pc)[1].type;

	(*pc) += 3;
	gen_expr (exp, pc, ax, value);
	gen_cast (ax, value, type);
      }
      break;

    case UNOP_CAST_TYPE:
      {
	int offset;
	struct value *val;
	struct type *type;

	++*pc;
	offset = *pc - exp->elts;
	val = evaluate_subexp (NULL, exp, &offset, EVAL_AVOID_SIDE_EFFECTS);
	type = value_type (val);
	*pc = &exp->elts[offset];

	gen_expr (exp, pc, ax, value);
	gen_cast (ax, value, type);
      }
      break;

    case UNOP_MEMVAL:
      {
	struct type *type = check_typedef ((*pc)[1].type);

	(*pc) += 3;
	gen_expr (exp, pc, ax, value);

	/* If we have an axs_rvalue or an axs_lvalue_memory, then we
	   already have the right value on the stack.  For
	   axs_lvalue_register, we must convert.  */
	if (value->kind == axs_lvalue_register)
	  require_rvalue (ax, value);

	value->type = type;
	value->kind = axs_lvalue_memory;
      }
      break;

    case UNOP_MEMVAL_TYPE:
      {
	int offset;
	struct value *val;
	struct type *type;

	++*pc;
	offset = *pc - exp->elts;
	val = evaluate_subexp (NULL, exp, &offset, EVAL_AVOID_SIDE_EFFECTS);
	type = value_type (val);
	*pc = &exp->elts[offset];

	gen_expr (exp, pc, ax, value);

	/* If we have an axs_rvalue or an axs_lvalue_memory, then we
	   already have the right value on the stack.  For
	   axs_lvalue_register, we must convert.  */
	if (value->kind == axs_lvalue_register)
	  require_rvalue (ax, value);

	value->type = type;
	value->kind = axs_lvalue_memory;
      }
      break;

    case UNOP_PLUS:
      (*pc)++;
      /* + FOO is equivalent to 0 + FOO, which can be optimized.  */
      gen_expr (exp, pc, ax, value);
      gen_usual_unary (exp, ax, value);
      break;
      
    case UNOP_NEG:
      (*pc)++;
      /* -FOO is equivalent to 0 - FOO.  */
      gen_int_literal (ax, &value1, 0,
		       builtin_type (exp->gdbarch)->builtin_int);
      gen_usual_unary (exp, ax, &value1);	/* shouldn't do much */
      gen_expr (exp, pc, ax, &value2);
      gen_usual_unary (exp, ax, &value2);
      gen_usual_arithmetic (exp, ax, &value1, &value2);
      gen_binop (ax, value, &value1, &value2, aop_sub, aop_sub, 1, "negation");
      break;

    case UNOP_LOGICAL_NOT:
      (*pc)++;
      gen_expr (exp, pc, ax, value);
      gen_usual_unary (exp, ax, value);
      gen_logical_not (ax, value, int_type);
      break;

    case UNOP_COMPLEMENT:
      (*pc)++;
      gen_expr (exp, pc, ax, value);
      gen_usual_unary (exp, ax, value);
      gen_integral_promotions (exp, ax, value);
      gen_complement (ax, value);
      break;

    case UNOP_IND:
      (*pc)++;
      gen_expr (exp, pc, ax, value);
      gen_usual_unary (exp, ax, value);
      if (!pointer_type (value->type))
	error (_("Argument of unary `*' is not a pointer."));
      gen_deref (ax, value);
      break;

    case UNOP_ADDR:
      (*pc)++;
      gen_expr (exp, pc, ax, value);
      gen_address_of (ax, value);
      break;

    case UNOP_SIZEOF:
      (*pc)++;
      /* Notice that gen_sizeof handles its own operand, unlike most
         of the other unary operator functions.  This is because we
         have to throw away the code we generate.  */
      gen_sizeof (exp, pc, ax, value,
		  builtin_type (exp->gdbarch)->builtin_int);
      break;

    case STRUCTOP_STRUCT:
    case STRUCTOP_PTR:
      {
	int length = (*pc)[1].longconst;
	char *name = &(*pc)[2].string;

	(*pc) += 4 + BYTES_TO_EXP_ELEM (length + 1);
	gen_expr (exp, pc, ax, value);
	if (op == STRUCTOP_STRUCT)
	  gen_struct_ref (exp, ax, value, name, ".", "structure or union");
	else if (op == STRUCTOP_PTR)
	  gen_struct_ref (exp, ax, value, name, "->",
			  "pointer to a structure or union");
	else
	  /* If this `if' chain doesn't handle it, then the case list
	     shouldn't mention it, and we shouldn't be here.  */
	  internal_error (__FILE__, __LINE__,
			  _("gen_expr: unhandled struct case"));
      }
      break;

    case OP_THIS:
      {
	struct symbol *sym, *func;
	struct block *b;
	const struct language_defn *lang;

	b = block_for_pc (ax->scope);
	func = block_linkage_function (b);
	lang = language_def (SYMBOL_LANGUAGE (func));

	sym = lookup_language_this (lang, b);
	if (!sym)
	  error (_("no `%s' found"), lang->la_name_of_this);

	gen_var_ref (exp->gdbarch, ax, value, sym);

	if (value->optimized_out)
	  error (_("`%s' has been optimized out, cannot use"),
		 SYMBOL_PRINT_NAME (sym));

	(*pc) += 2;
      }
      break;

    case OP_SCOPE:
      {
	struct type *type = (*pc)[1].type;
	int length = longest_to_int ((*pc)[2].longconst);
	char *name = &(*pc)[3].string;
	int found;

	found = gen_aggregate_elt_ref (exp, ax, value, type, name,
				       "?", "??");
	if (!found)
	  error (_("There is no field named %s"), name);
	(*pc) += 5 + BYTES_TO_EXP_ELEM (length + 1);
      }
      break;

    case OP_TYPE:
    case OP_TYPEOF:
    case OP_DECLTYPE:
      error (_("Attempt to use a type name as an expression."));

    default:
      error (_("Unsupported operator %s (%d) in expression."),
	     op_name (exp, op), op);
    }
}

/* This handles the middle-to-right-side of code generation for binary
   expressions, which is shared between regular binary operations and
   assign-modify (+= and friends) expressions.  */

static void
gen_expr_binop_rest (struct expression *exp,
		     enum exp_opcode op, union exp_element **pc,
		     struct agent_expr *ax, struct axs_value *value,
		     struct axs_value *value1, struct axs_value *value2)
{
  struct type *int_type = builtin_type (exp->gdbarch)->builtin_int;

  gen_expr (exp, pc, ax, value2);
  gen_usual_unary (exp, ax, value2);
  gen_usual_arithmetic (exp, ax, value1, value2);
  switch (op)
    {
    case BINOP_ADD:
      if (TYPE_CODE (value1->type) == TYPE_CODE_INT
	  && pointer_type (value2->type))
	{
	  /* Swap the values and proceed normally.  */
	  ax_simple (ax, aop_swap);
	  gen_ptradd (ax, value, value2, value1);
	}
      else if (pointer_type (value1->type)
	       && TYPE_CODE (value2->type) == TYPE_CODE_INT)
	gen_ptradd (ax, value, value1, value2);
      else
	gen_binop (ax, value, value1, value2,
		   aop_add, aop_add, 1, "addition");
      break;
    case BINOP_SUB:
      if (pointer_type (value1->type)
	  && TYPE_CODE (value2->type) == TYPE_CODE_INT)
	gen_ptrsub (ax,value, value1, value2);
      else if (pointer_type (value1->type)
	       && pointer_type (value2->type))
	/* FIXME --- result type should be ptrdiff_t */
	gen_ptrdiff (ax, value, value1, value2,
		     builtin_type (exp->gdbarch)->builtin_long);
      else
	gen_binop (ax, value, value1, value2,
		   aop_sub, aop_sub, 1, "subtraction");
      break;
    case BINOP_MUL:
      gen_binop (ax, value, value1, value2,
		 aop_mul, aop_mul, 1, "multiplication");
      break;
    case BINOP_DIV:
      gen_binop (ax, value, value1, value2,
		 aop_div_signed, aop_div_unsigned, 1, "division");
      break;
    case BINOP_REM:
      gen_binop (ax, value, value1, value2,
		 aop_rem_signed, aop_rem_unsigned, 1, "remainder");
      break;
    case BINOP_LSH:
      gen_binop (ax, value, value1, value2,
		 aop_lsh, aop_lsh, 1, "left shift");
      break;
    case BINOP_RSH:
      gen_binop (ax, value, value1, value2,
		 aop_rsh_signed, aop_rsh_unsigned, 1, "right shift");
      break;
    case BINOP_SUBSCRIPT:
      {
	struct type *type;

	if (binop_types_user_defined_p (op, value1->type, value2->type))
	  {
	    error (_("cannot subscript requested type: "
		     "cannot call user defined functions"));
	  }
	else
	  {
	    /* If the user attempts to subscript something that is not
	       an array or pointer type (like a plain int variable for
	       example), then report this as an error.  */
	    type = check_typedef (value1->type);
	    if (TYPE_CODE (type) != TYPE_CODE_ARRAY
		&& TYPE_CODE (type) != TYPE_CODE_PTR)
	      {
		if (TYPE_NAME (type))
		  error (_("cannot subscript something of type `%s'"),
			 TYPE_NAME (type));
		else
		  error (_("cannot subscript requested type"));
	      }
	  }

	if (!is_integral_type (value2->type))
	  error (_("Argument to arithmetic operation "
		   "not a number or boolean."));

	gen_ptradd (ax, value, value1, value2);
	gen_deref (ax, value);
	break;
      }
    case BINOP_BITWISE_AND:
      gen_binop (ax, value, value1, value2,
		 aop_bit_and, aop_bit_and, 0, "bitwise and");
      break;

    case BINOP_BITWISE_IOR:
      gen_binop (ax, value, value1, value2,
		 aop_bit_or, aop_bit_or, 0, "bitwise or");
      break;
      
    case BINOP_BITWISE_XOR:
      gen_binop (ax, value, value1, value2,
		 aop_bit_xor, aop_bit_xor, 0, "bitwise exclusive-or");
      break;

    case BINOP_EQUAL:
      gen_equal (ax, value, value1, value2, int_type);
      break;

    case BINOP_NOTEQUAL:
      gen_equal (ax, value, value1, value2, int_type);
      gen_logical_not (ax, value, int_type);
      break;

    case BINOP_LESS:
      gen_less (ax, value, value1, value2, int_type);
      break;

    case BINOP_GTR:
      ax_simple (ax, aop_swap);
      gen_less (ax, value, value1, value2, int_type);
      break;

    case BINOP_LEQ:
      ax_simple (ax, aop_swap);
      gen_less (ax, value, value1, value2, int_type);
      gen_logical_not (ax, value, int_type);
      break;

    case BINOP_GEQ:
      gen_less (ax, value, value1, value2, int_type);
      gen_logical_not (ax, value, int_type);
      break;

    default:
      /* We should only list operators in the outer case statement
	 that we actually handle in the inner case statement.  */
      internal_error (__FILE__, __LINE__,
		      _("gen_expr: op case sets don't match"));
    }
}


/* Given a single variable and a scope, generate bytecodes to trace
   its value.  This is for use in situations where we have only a
   variable's name, and no parsed expression; for instance, when the
   name comes from a list of local variables of a function.  */

struct agent_expr *
gen_trace_for_var (CORE_ADDR scope, struct gdbarch *gdbarch,
		   struct symbol *var, int trace_string)
{
  struct cleanup *old_chain = 0;
  struct agent_expr *ax = new_agent_expr (gdbarch, scope);
  struct axs_value value;

  old_chain = make_cleanup_free_agent_expr (ax);

  ax->tracing = 1;
  ax->trace_string = trace_string;
  gen_var_ref (gdbarch, ax, &value, var);

  /* If there is no actual variable to trace, flag it by returning
     an empty agent expression.  */
  if (value.optimized_out)
    {
      do_cleanups (old_chain);
      return NULL;
    }

  /* Make sure we record the final object, and get rid of it.  */
  gen_traced_pop (gdbarch, ax, &value);

  /* Oh, and terminate.  */
  ax_simple (ax, aop_end);

  /* We have successfully built the agent expr, so cancel the cleanup
     request.  If we add more cleanups that we always want done, this
     will have to get more complicated.  */
  discard_cleanups (old_chain);
  return ax;
}

/* Generating bytecode from GDB expressions: driver */

/* Given a GDB expression EXPR, return bytecode to trace its value.
   The result will use the `trace' and `trace_quick' bytecodes to
   record the value of all memory touched by the expression.  The
   caller can then use the ax_reqs function to discover which
   registers it relies upon.  */
struct agent_expr *
gen_trace_for_expr (CORE_ADDR scope, struct expression *expr,
		    int trace_string)
{
  struct cleanup *old_chain = 0;
  struct agent_expr *ax = new_agent_expr (expr->gdbarch, scope);
  union exp_element *pc;
  struct axs_value value;

  old_chain = make_cleanup_free_agent_expr (ax);

  pc = expr->elts;
  ax->tracing = 1;
  ax->trace_string = trace_string;
  value.optimized_out = 0;
  gen_expr (expr, &pc, ax, &value);

  /* Make sure we record the final object, and get rid of it.  */
  gen_traced_pop (expr->gdbarch, ax, &value);

  /* Oh, and terminate.  */
  ax_simple (ax, aop_end);

  /* We have successfully built the agent expr, so cancel the cleanup
     request.  If we add more cleanups that we always want done, this
     will have to get more complicated.  */
  discard_cleanups (old_chain);
  return ax;
}

/* Given a GDB expression EXPR, return a bytecode sequence that will
   evaluate and return a result.  The bytecodes will do a direct
   evaluation, using the current data on the target, rather than
   recording blocks of memory and registers for later use, as
   gen_trace_for_expr does.  The generated bytecode sequence leaves
   the result of expression evaluation on the top of the stack.  */

struct agent_expr *
gen_eval_for_expr (CORE_ADDR scope, struct expression *expr)
{
  struct cleanup *old_chain = 0;
  struct agent_expr *ax = new_agent_expr (expr->gdbarch, scope);
  union exp_element *pc;
  struct axs_value value;

  old_chain = make_cleanup_free_agent_expr (ax);

  pc = expr->elts;
  ax->tracing = 0;
  value.optimized_out = 0;
  gen_expr (expr, &pc, ax, &value);

  require_rvalue (ax, &value);

  /* Oh, and terminate.  */
  ax_simple (ax, aop_end);

  /* We have successfully built the agent expr, so cancel the cleanup
     request.  If we add more cleanups that we always want done, this
     will have to get more complicated.  */
  discard_cleanups (old_chain);
  return ax;
}

struct agent_expr *
gen_trace_for_return_address (CORE_ADDR scope, struct gdbarch *gdbarch,
			      int trace_string)
{
  struct cleanup *old_chain = 0;
  struct agent_expr *ax = new_agent_expr (gdbarch, scope);
  struct axs_value value;

  old_chain = make_cleanup_free_agent_expr (ax);

  ax->tracing = 1;
  ax->trace_string = trace_string;

  gdbarch_gen_return_address (gdbarch, ax, &value, scope);

  /* Make sure we record the final object, and get rid of it.  */
  gen_traced_pop (gdbarch, ax, &value);

  /* Oh, and terminate.  */
  ax_simple (ax, aop_end);

  /* We have successfully built the agent expr, so cancel the cleanup
     request.  If we add more cleanups that we always want done, this
     will have to get more complicated.  */
  discard_cleanups (old_chain);
  return ax;
}

/* Given a collection of printf-style arguments, generate code to
   evaluate the arguments and pass everything to a special
   bytecode.  */

struct agent_expr *
gen_printf (CORE_ADDR scope, struct gdbarch *gdbarch,
	    CORE_ADDR function, LONGEST channel,
	    const char *format, int fmtlen,
	    struct format_piece *frags,
	    int nargs, struct expression **exprs)
{
  struct cleanup *old_chain = 0;
  struct agent_expr *ax = new_agent_expr (gdbarch, scope);
  union exp_element *pc;
  struct axs_value value;
  int tem;

  old_chain = make_cleanup_free_agent_expr (ax);

  /* We're computing values, not doing side effects.  */
  ax->tracing = 0;

  /* Evaluate and push the args on the stack in reverse order,
     for simplicity of collecting them on the target side.  */
  for (tem = nargs - 1; tem >= 0; --tem)
    {
      pc = exprs[tem]->elts;
      value.optimized_out = 0;
      gen_expr (exprs[tem], &pc, ax, &value);
      require_rvalue (ax, &value);
    }

  /* Push function and channel.  */
  ax_const_l (ax, channel);
  ax_const_l (ax, function);

  /* Issue the printf bytecode proper.  */
  ax_simple (ax, aop_printf);
  ax_simple (ax, nargs);
  ax_string (ax, format, fmtlen);

  /* And terminate.  */
  ax_simple (ax, aop_end);

  /* We have successfully built the agent expr, so cancel the cleanup
     request.  If we add more cleanups that we always want done, this
     will have to get more complicated.  */
  discard_cleanups (old_chain);

  return ax;
}

static void
agent_eval_command_one (const char *exp, int eval, CORE_ADDR pc)
{
  struct cleanup *old_chain = 0;
  struct expression *expr;
  struct agent_expr *agent;
  const char *arg;
  int trace_string = 0;

  if (!eval)
    {
      if (*exp == '/')
        exp = decode_agent_options (exp, &trace_string);
    }

  arg = exp;
  if (!eval && strcmp (arg, "$_ret") == 0)
    {
      agent = gen_trace_for_return_address (pc, get_current_arch (),
					    trace_string);
      old_chain = make_cleanup_free_agent_expr (agent);
    }
  else
    {
      expr = parse_exp_1 (&arg, pc, block_for_pc (pc), 0);
      old_chain = make_cleanup (free_current_contents, &expr);
      if (eval)
	{
	  gdb_assert (trace_string == 0);
	  agent = gen_eval_for_expr (pc, expr);
	}
      else
	agent = gen_trace_for_expr (pc, expr, trace_string);
      make_cleanup_free_agent_expr (agent);
    }

  ax_reqs (agent);
  ax_print (gdb_stdout, agent);

  /* It would be nice to call ax_reqs here to gather some general info
     about the expression, and then print out the result.  */

  do_cleanups (old_chain);
  dont_repeat ();
}

static void
agent_command_1 (char *exp, int eval)
{
  /* We don't deal with overlay debugging at the moment.  We need to
     think more carefully about this.  If you copy this code into
     another command, change the error message; the user shouldn't
     have to know anything about agent expressions.  */
  if (overlay_debugging)
    error (_("GDB can't do agent expression translation with overlays."));

  if (exp == 0)
    error_no_arg (_("expression to translate"));

  if (check_for_argument (&exp, "-at", sizeof ("-at") - 1))
    {
      struct linespec_result canonical;
      int ix;
      struct linespec_sals *iter;
      struct cleanup *old_chain;

      exp = skip_spaces (exp);
      init_linespec_result (&canonical);
      decode_line_full (&exp, DECODE_LINE_FUNFIRSTLINE,
			(struct symtab *) NULL, 0, &canonical,
			NULL, NULL);
      old_chain = make_cleanup_destroy_linespec_result (&canonical);
      exp = skip_spaces (exp);
      if (exp[0] == ',')
        {
	  exp++;
	  exp = skip_spaces (exp);
	}
      for (ix = 0; VEC_iterate (linespec_sals, canonical.sals, ix, iter); ++ix)
        {
	  int i;

	  for (i = 0; i < iter->sals.nelts; i++)
	    agent_eval_command_one (exp, eval, iter->sals.sals[i].pc);
        }
      do_cleanups (old_chain);
    }
  else
    agent_eval_command_one (exp, eval, get_frame_pc (get_current_frame ()));

  dont_repeat ();
}

static void
agent_command (char *exp, int from_tty)
{
  agent_command_1 (exp, 0);
}

/* Parse the given expression, compile it into an agent expression
   that does direct evaluation, and display the resulting
   expression.  */

static void
agent_eval_command (char *exp, int from_tty)
{
  agent_command_1 (exp, 1);
}

/* Parse the given expression, compile it into an agent expression
   that does a printf, and display the resulting expression.  */

static void
maint_agent_printf_command (char *exp, int from_tty)
{
  struct cleanup *old_chain = 0;
  struct expression *expr;
  struct expression *argvec[100];
  struct agent_expr *agent;
  struct frame_info *fi = get_current_frame ();	/* need current scope */
  const char *cmdrest;
  const char *format_start, *format_end;
  struct format_piece *fpieces;
  int nargs;

  /* We don't deal with overlay debugging at the moment.  We need to
     think more carefully about this.  If you copy this code into
     another command, change the error message; the user shouldn't
     have to know anything about agent expressions.  */
  if (overlay_debugging)
    error (_("GDB can't do agent expression translation with overlays."));

  if (exp == 0)
    error_no_arg (_("expression to translate"));

  cmdrest = exp;

  cmdrest = skip_spaces_const (cmdrest);

  if (*cmdrest++ != '"')
    error (_("Must start with a format string."));

  format_start = cmdrest;

  fpieces = parse_format_string (&cmdrest);

  old_chain = make_cleanup (free_format_pieces_cleanup, &fpieces);

  format_end = cmdrest;

  if (*cmdrest++ != '"')
    error (_("Bad format string, non-terminated '\"'."));
  
  cmdrest = skip_spaces_const (cmdrest);

  if (*cmdrest != ',' && *cmdrest != 0)
    error (_("Invalid argument syntax"));

  if (*cmdrest == ',')
    cmdrest++;
  cmdrest = skip_spaces_const (cmdrest);

  nargs = 0;
  while (*cmdrest != '\0')
    {
      const char *cmd1;

      cmd1 = cmdrest;
      expr = parse_exp_1 (&cmd1, 0, (struct block *) 0, 1);
      argvec[nargs] = expr;
      ++nargs;
      cmdrest = cmd1;
      if (*cmdrest == ',')
	++cmdrest;
      /* else complain? */
    }


  agent = gen_printf (get_frame_pc (fi), get_current_arch (), 0, 0,
		      format_start, format_end - format_start,
		      fpieces, nargs, argvec);
  make_cleanup_free_agent_expr (agent);
  ax_reqs (agent);
  ax_print (gdb_stdout, agent);

  /* It would be nice to call ax_reqs here to gather some general info
     about the expression, and then print out the result.  */

  do_cleanups (old_chain);
  dont_repeat ();
}


/* Initialization code.  */

void _initialize_ax_gdb (void);
void
_initialize_ax_gdb (void)
{
  add_cmd ("agent", class_maintenance, agent_command,
	   _("\
Translate an expression into remote agent bytecode for tracing.\n\
Usage: maint agent [-at location,] EXPRESSION\n\
If -at is given, generate remote agent bytecode for this location.\n\
If not, generate remote agent bytecode for current frame pc address."),
	   &maintenancelist);

  add_cmd ("agent-eval", class_maintenance, agent_eval_command,
	   _("\
Translate an expression into remote agent bytecode for evaluation.\n\
Usage: maint agent-eval [-at location,] EXPRESSION\n\
If -at is given, generate remote agent bytecode for this location.\n\
If not, generate remote agent bytecode for current frame pc address."),
	   &maintenancelist);

  add_cmd ("agent-printf", class_maintenance, maint_agent_printf_command,
	   _("Translate an expression into remote "
	     "agent bytecode for evaluation and display the bytecodes."),
	   &maintenancelist);
}
