/* Evaluate expressions for GDB.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include <string.h>
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "expression.h"
#include "target.h"
#include "frame.h"
#include "language.h"		/* For CAST_IS_CONVERSION.  */
#include "f-lang.h"		/* For array bound stuff.  */
#include "cp-abi.h"
#include "infcall.h"
#include "objc-lang.h"
#include "block.h"
#include "parser-defs.h"
#include "cp-support.h"
#include "ui-out.h"
#include "exceptions.h"
#include "regcache.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdb_obstack.h"
#include "objfiles.h"
#include "python/python.h"

#include "gdb_assert.h"

#include <ctype.h>

/* This is defined in valops.c */
extern int overload_resolution;

/* Prototypes for local functions.  */

static struct value *evaluate_subexp_for_sizeof (struct expression *, int *);

static struct value *evaluate_subexp_for_address (struct expression *,
						  int *, enum noside);

static struct value *evaluate_struct_tuple (struct value *,
					    struct expression *, int *,
					    enum noside, int);

static LONGEST init_array_element (struct value *, struct value *,
				   struct expression *, int *, enum noside,
				   LONGEST, LONGEST);

struct value *
evaluate_subexp (struct type *expect_type, struct expression *exp,
		 int *pos, enum noside noside)
{
  return (*exp->language_defn->la_exp_desc->evaluate_exp) 
    (expect_type, exp, pos, noside);
}

/* Parse the string EXP as a C expression, evaluate it,
   and return the result as a number.  */

CORE_ADDR
parse_and_eval_address (const char *exp)
{
  struct expression *expr = parse_expression (exp);
  CORE_ADDR addr;
  struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

  addr = value_as_address (evaluate_expression (expr));
  do_cleanups (old_chain);
  return addr;
}

/* Like parse_and_eval_address, but treats the value of the expression
   as an integer, not an address, returns a LONGEST, not a CORE_ADDR.  */
LONGEST
parse_and_eval_long (const char *exp)
{
  struct expression *expr = parse_expression (exp);
  LONGEST retval;
  struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

  retval = value_as_long (evaluate_expression (expr));
  do_cleanups (old_chain);
  return (retval);
}

struct value *
parse_and_eval (const char *exp)
{
  struct expression *expr = parse_expression (exp);
  struct value *val;
  struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

  val = evaluate_expression (expr);
  do_cleanups (old_chain);
  return val;
}

/* Parse up to a comma (or to a closeparen)
   in the string EXPP as an expression, evaluate it, and return the value.
   EXPP is advanced to point to the comma.  */

struct value *
parse_to_comma_and_eval (const char **expp)
{
  struct expression *expr = parse_exp_1 (expp, 0, (struct block *) 0, 1);
  struct value *val;
  struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

  val = evaluate_expression (expr);
  do_cleanups (old_chain);
  return val;
}

/* Evaluate an expression in internal prefix form
   such as is constructed by parse.y.

   See expression.h for info on the format of an expression.  */

struct value *
evaluate_expression (struct expression *exp)
{
  int pc = 0;

  return evaluate_subexp (NULL_TYPE, exp, &pc, EVAL_NORMAL);
}

/* Evaluate an expression, avoiding all memory references
   and getting a value whose type alone is correct.  */

struct value *
evaluate_type (struct expression *exp)
{
  int pc = 0;

  return evaluate_subexp (NULL_TYPE, exp, &pc, EVAL_AVOID_SIDE_EFFECTS);
}

/* Evaluate a subexpression, avoiding all memory references and
   getting a value whose type alone is correct.  */

struct value *
evaluate_subexpression_type (struct expression *exp, int subexp)
{
  return evaluate_subexp (NULL_TYPE, exp, &subexp, EVAL_AVOID_SIDE_EFFECTS);
}

/* Find the current value of a watchpoint on EXP.  Return the value in
   *VALP and *RESULTP and the chain of intermediate and final values
   in *VAL_CHAIN.  RESULTP and VAL_CHAIN may be NULL if the caller does
   not need them.

   If PRESERVE_ERRORS is true, then exceptions are passed through.
   Otherwise, if PRESERVE_ERRORS is false, then if a memory error
   occurs while evaluating the expression, *RESULTP will be set to
   NULL.  *RESULTP may be a lazy value, if the result could not be
   read from memory.  It is used to determine whether a value is
   user-specified (we should watch the whole value) or intermediate
   (we should watch only the bit used to locate the final value).

   If the final value, or any intermediate value, could not be read
   from memory, *VALP will be set to NULL.  *VAL_CHAIN will still be
   set to any referenced values.  *VALP will never be a lazy value.
   This is the value which we store in struct breakpoint.

   If VAL_CHAIN is non-NULL, *VAL_CHAIN will be released from the
   value chain.  The caller must free the values individually.  If
   VAL_CHAIN is NULL, all generated values will be left on the value
   chain.  */

void
fetch_subexp_value (struct expression *exp, int *pc, struct value **valp,
		    struct value **resultp, struct value **val_chain,
		    int preserve_errors)
{
  struct value *mark, *new_mark, *result;
  volatile struct gdb_exception ex;

  *valp = NULL;
  if (resultp)
    *resultp = NULL;
  if (val_chain)
    *val_chain = NULL;

  /* Evaluate the expression.  */
  mark = value_mark ();
  result = NULL;

  TRY_CATCH (ex, RETURN_MASK_ALL)
    {
      result = evaluate_subexp (NULL_TYPE, exp, pc, EVAL_NORMAL);
    }
  if (ex.reason < 0)
    {
      /* Ignore memory errors if we want watchpoints pointing at
	 inaccessible memory to still be created; otherwise, throw the
	 error to some higher catcher.  */
      switch (ex.error)
	{
	case MEMORY_ERROR:
	  if (!preserve_errors)
	    break;
	default:
	  throw_exception (ex);
	  break;
	}
    }

  new_mark = value_mark ();
  if (mark == new_mark)
    return;
  if (resultp)
    *resultp = result;

  /* Make sure it's not lazy, so that after the target stops again we
     have a non-lazy previous value to compare with.  */
  if (result != NULL)
    {
      if (!value_lazy (result))
	*valp = result;
      else
	{
	  volatile struct gdb_exception except;

	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      value_fetch_lazy (result);
	      *valp = result;
	    }
	}
    }

  if (val_chain)
    {
      /* Return the chain of intermediate values.  We use this to
	 decide which addresses to watch.  */
      *val_chain = new_mark;
      value_release_to_mark (mark);
    }
}

/* Extract a field operation from an expression.  If the subexpression
   of EXP starting at *SUBEXP is not a structure dereference
   operation, return NULL.  Otherwise, return the name of the
   dereferenced field, and advance *SUBEXP to point to the
   subexpression of the left-hand-side of the dereference.  This is
   used when completing field names.  */

char *
extract_field_op (struct expression *exp, int *subexp)
{
  int tem;
  char *result;

  if (exp->elts[*subexp].opcode != STRUCTOP_STRUCT
      && exp->elts[*subexp].opcode != STRUCTOP_PTR)
    return NULL;
  tem = longest_to_int (exp->elts[*subexp + 1].longconst);
  result = &exp->elts[*subexp + 2].string;
  (*subexp) += 1 + 3 + BYTES_TO_EXP_ELEM (tem + 1);
  return result;
}

/* This function evaluates brace-initializers (in C/C++) for
   structure types.  */

static struct value *
evaluate_struct_tuple (struct value *struct_val,
		       struct expression *exp,
		       int *pos, enum noside noside, int nargs)
{
  struct type *struct_type = check_typedef (value_type (struct_val));
  struct type *field_type;
  int fieldno = -1;

  while (--nargs >= 0)
    {
      struct value *val = NULL;
      int bitpos, bitsize;
      bfd_byte *addr;

      fieldno++;
      /* Skip static fields.  */
      while (fieldno < TYPE_NFIELDS (struct_type)
	     && field_is_static (&TYPE_FIELD (struct_type,
					      fieldno)))
	fieldno++;
      if (fieldno >= TYPE_NFIELDS (struct_type))
	error (_("too many initializers"));
      field_type = TYPE_FIELD_TYPE (struct_type, fieldno);
      if (TYPE_CODE (field_type) == TYPE_CODE_UNION
	  && TYPE_FIELD_NAME (struct_type, fieldno)[0] == '0')
	error (_("don't know which variant you want to set"));

      /* Here, struct_type is the type of the inner struct,
	 while substruct_type is the type of the inner struct.
	 These are the same for normal structures, but a variant struct
	 contains anonymous union fields that contain substruct fields.
	 The value fieldno is the index of the top-level (normal or
	 anonymous union) field in struct_field, while the value
	 subfieldno is the index of the actual real (named inner) field
	 in substruct_type.  */

      field_type = TYPE_FIELD_TYPE (struct_type, fieldno);
      if (val == 0)
	val = evaluate_subexp (field_type, exp, pos, noside);

      /* Now actually set the field in struct_val.  */

      /* Assign val to field fieldno.  */
      if (value_type (val) != field_type)
	val = value_cast (field_type, val);

      bitsize = TYPE_FIELD_BITSIZE (struct_type, fieldno);
      bitpos = TYPE_FIELD_BITPOS (struct_type, fieldno);
      addr = value_contents_writeable (struct_val) + bitpos / 8;
      if (bitsize)
	modify_field (struct_type, addr,
		      value_as_long (val), bitpos % 8, bitsize);
      else
	memcpy (addr, value_contents (val),
		TYPE_LENGTH (value_type (val)));

    }
  return struct_val;
}

/* Recursive helper function for setting elements of array tuples for
   (the deleted) Chill.  The target is ARRAY (which has bounds
   LOW_BOUND to HIGH_BOUND); the element value is ELEMENT; EXP, POS
   and NOSIDE are as usual.  Evaluates index expresions and sets the
   specified element(s) of ARRAY to ELEMENT.  Returns last index
   value.  */

static LONGEST
init_array_element (struct value *array, struct value *element,
		    struct expression *exp, int *pos,
		    enum noside noside, LONGEST low_bound, LONGEST high_bound)
{
  LONGEST index;
  int element_size = TYPE_LENGTH (value_type (element));

  if (exp->elts[*pos].opcode == BINOP_COMMA)
    {
      (*pos)++;
      init_array_element (array, element, exp, pos, noside,
			  low_bound, high_bound);
      return init_array_element (array, element,
				 exp, pos, noside, low_bound, high_bound);
    }
  else if (exp->elts[*pos].opcode == BINOP_RANGE)
    {
      LONGEST low, high;

      (*pos)++;
      low = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));
      high = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));
      if (low < low_bound || high > high_bound)
	error (_("tuple range index out of range"));
      for (index = low; index <= high; index++)
	{
	  memcpy (value_contents_raw (array)
		  + (index - low_bound) * element_size,
		  value_contents (element), element_size);
	}
    }
  else
    {
      index = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));
      if (index < low_bound || index > high_bound)
	error (_("tuple index out of range"));
      memcpy (value_contents_raw (array) + (index - low_bound) * element_size,
	      value_contents (element), element_size);
    }
  return index;
}

static struct value *
value_f90_subarray (struct value *array,
		    struct expression *exp, int *pos, enum noside noside)
{
  int pc = (*pos) + 1;
  LONGEST low_bound, high_bound;
  struct type *range = check_typedef (TYPE_INDEX_TYPE (value_type (array)));
  enum f90_range_type range_type = longest_to_int (exp->elts[pc].longconst);
 
  *pos += 3;

  if (range_type == LOW_BOUND_DEFAULT || range_type == BOTH_BOUND_DEFAULT)
    low_bound = TYPE_LOW_BOUND (range);
  else
    low_bound = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));

  if (range_type == HIGH_BOUND_DEFAULT || range_type == BOTH_BOUND_DEFAULT)
    high_bound = TYPE_HIGH_BOUND (range);
  else
    high_bound = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));

  return value_slice (array, low_bound, high_bound - low_bound + 1);
}


/* Promote value ARG1 as appropriate before performing a unary operation
   on this argument.
   If the result is not appropriate for any particular language then it
   needs to patch this function.  */

void
unop_promote (const struct language_defn *language, struct gdbarch *gdbarch,
	      struct value **arg1)
{
  struct type *type1;

  *arg1 = coerce_ref (*arg1);
  type1 = check_typedef (value_type (*arg1));

  if (is_integral_type (type1))
    {
      switch (language->la_language)
	{
	default:
	  /* Perform integral promotion for ANSI C/C++.
	     If not appropropriate for any particular language
	     it needs to modify this function.  */
	  {
	    struct type *builtin_int = builtin_type (gdbarch)->builtin_int;

	    if (TYPE_LENGTH (type1) < TYPE_LENGTH (builtin_int))
	      *arg1 = value_cast (builtin_int, *arg1);
	  }
	  break;
	}
    }
}

/* Promote values ARG1 and ARG2 as appropriate before performing a binary
   operation on those two operands.
   If the result is not appropriate for any particular language then it
   needs to patch this function.  */

void
binop_promote (const struct language_defn *language, struct gdbarch *gdbarch,
	       struct value **arg1, struct value **arg2)
{
  struct type *promoted_type = NULL;
  struct type *type1;
  struct type *type2;

  *arg1 = coerce_ref (*arg1);
  *arg2 = coerce_ref (*arg2);

  type1 = check_typedef (value_type (*arg1));
  type2 = check_typedef (value_type (*arg2));

  if ((TYPE_CODE (type1) != TYPE_CODE_FLT
       && TYPE_CODE (type1) != TYPE_CODE_DECFLOAT
       && !is_integral_type (type1))
      || (TYPE_CODE (type2) != TYPE_CODE_FLT
	  && TYPE_CODE (type2) != TYPE_CODE_DECFLOAT
	  && !is_integral_type (type2)))
    return;

  if (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT
      || TYPE_CODE (type2) == TYPE_CODE_DECFLOAT)
    {
      /* No promotion required.  */
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_FLT
	   || TYPE_CODE (type2) == TYPE_CODE_FLT)
    {
      switch (language->la_language)
	{
	case language_c:
	case language_cplus:
	case language_asm:
	case language_objc:
	case language_opencl:
	  /* No promotion required.  */
	  break;

	default:
	  /* For other languages the result type is unchanged from gdb
	     version 6.7 for backward compatibility.
	     If either arg was long double, make sure that value is also long
	     double.  Otherwise use double.  */
	  if (TYPE_LENGTH (type1) * 8 > gdbarch_double_bit (gdbarch)
	      || TYPE_LENGTH (type2) * 8 > gdbarch_double_bit (gdbarch))
	    promoted_type = builtin_type (gdbarch)->builtin_long_double;
	  else
	    promoted_type = builtin_type (gdbarch)->builtin_double;
	  break;
	}
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_BOOL
	   && TYPE_CODE (type2) == TYPE_CODE_BOOL)
    {
      /* No promotion required.  */
    }
  else
    /* Integral operations here.  */
    /* FIXME: Also mixed integral/booleans, with result an integer.  */
    {
      const struct builtin_type *builtin = builtin_type (gdbarch);
      unsigned int promoted_len1 = TYPE_LENGTH (type1);
      unsigned int promoted_len2 = TYPE_LENGTH (type2);
      int is_unsigned1 = TYPE_UNSIGNED (type1);
      int is_unsigned2 = TYPE_UNSIGNED (type2);
      unsigned int result_len;
      int unsigned_operation;

      /* Determine type length and signedness after promotion for
         both operands.  */
      if (promoted_len1 < TYPE_LENGTH (builtin->builtin_int))
	{
	  is_unsigned1 = 0;
	  promoted_len1 = TYPE_LENGTH (builtin->builtin_int);
	}
      if (promoted_len2 < TYPE_LENGTH (builtin->builtin_int))
	{
	  is_unsigned2 = 0;
	  promoted_len2 = TYPE_LENGTH (builtin->builtin_int);
	}

      if (promoted_len1 > promoted_len2)
	{
	  unsigned_operation = is_unsigned1;
	  result_len = promoted_len1;
	}
      else if (promoted_len2 > promoted_len1)
	{
	  unsigned_operation = is_unsigned2;
	  result_len = promoted_len2;
	}
      else
	{
	  unsigned_operation = is_unsigned1 || is_unsigned2;
	  result_len = promoted_len1;
	}

      switch (language->la_language)
	{
	case language_c:
	case language_cplus:
	case language_asm:
	case language_objc:
	  if (result_len <= TYPE_LENGTH (builtin->builtin_int))
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_int
			       : builtin->builtin_int);
	    }
	  else if (result_len <= TYPE_LENGTH (builtin->builtin_long))
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_long
			       : builtin->builtin_long);
	    }
	  else
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_long_long
			       : builtin->builtin_long_long);
	    }
	  break;
	case language_opencl:
	  if (result_len <= TYPE_LENGTH (lookup_signed_typename
					 (language, gdbarch, "int")))
	    {
	      promoted_type =
		(unsigned_operation
		 ? lookup_unsigned_typename (language, gdbarch, "int")
		 : lookup_signed_typename (language, gdbarch, "int"));
	    }
	  else if (result_len <= TYPE_LENGTH (lookup_signed_typename
					      (language, gdbarch, "long")))
	    {
	      promoted_type =
		(unsigned_operation
		 ? lookup_unsigned_typename (language, gdbarch, "long")
		 : lookup_signed_typename (language, gdbarch,"long"));
	    }
	  break;
	default:
	  /* For other languages the result type is unchanged from gdb
	     version 6.7 for backward compatibility.
	     If either arg was long long, make sure that value is also long
	     long.  Otherwise use long.  */
	  if (unsigned_operation)
	    {
	      if (result_len > gdbarch_long_bit (gdbarch) / HOST_CHAR_BIT)
		promoted_type = builtin->builtin_unsigned_long_long;
	      else
		promoted_type = builtin->builtin_unsigned_long;
	    }
	  else
	    {
	      if (result_len > gdbarch_long_bit (gdbarch) / HOST_CHAR_BIT)
		promoted_type = builtin->builtin_long_long;
	      else
		promoted_type = builtin->builtin_long;
	    }
	  break;
	}
    }

  if (promoted_type)
    {
      /* Promote both operands to common type.  */
      *arg1 = value_cast (promoted_type, *arg1);
      *arg2 = value_cast (promoted_type, *arg2);
    }
}

static int
ptrmath_type_p (const struct language_defn *lang, struct type *type)
{
  type = check_typedef (type);
  if (TYPE_CODE (type) == TYPE_CODE_REF)
    type = TYPE_TARGET_TYPE (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
      return 1;

    case TYPE_CODE_ARRAY:
      return TYPE_VECTOR (type) ? 0 : lang->c_style_arrays;

    default:
      return 0;
    }
}

/* Constructs a fake method with the given parameter types.
   This function is used by the parser to construct an "expected"
   type for method overload resolution.  */

static struct type *
make_params (int num_types, struct type **param_types)
{
  struct type *type = XZALLOC (struct type);
  TYPE_MAIN_TYPE (type) = XZALLOC (struct main_type);
  TYPE_LENGTH (type) = 1;
  TYPE_CODE (type) = TYPE_CODE_METHOD;
  TYPE_VPTR_FIELDNO (type) = -1;
  TYPE_CHAIN (type) = type;
  if (num_types > 0)
    {
      if (param_types[num_types - 1] == NULL)
	{
	  --num_types;
	  TYPE_VARARGS (type) = 1;
	}
      else if (TYPE_CODE (check_typedef (param_types[num_types - 1]))
	       == TYPE_CODE_VOID)
	{
	  --num_types;
	  /* Caller should have ensured this.  */
	  gdb_assert (num_types == 0);
	  TYPE_PROTOTYPED (type) = 1;
	}
    }

  TYPE_NFIELDS (type) = num_types;
  TYPE_FIELDS (type) = (struct field *)
    TYPE_ZALLOC (type, sizeof (struct field) * num_types);

  while (num_types-- > 0)
    TYPE_FIELD_TYPE (type, num_types) = param_types[num_types];

  return type;
}

struct value *
evaluate_subexp_standard (struct type *expect_type,
			  struct expression *exp, int *pos,
			  enum noside noside)
{
  enum exp_opcode op;
  int tem, tem2, tem3;
  int pc, pc2 = 0, oldpos;
  struct value *arg1 = NULL;
  struct value *arg2 = NULL;
  struct value *arg3;
  struct type *type;
  int nargs;
  struct value **argvec;
  int code;
  int ix;
  long mem_offset;
  struct type **arg_types;
  int save_pos1;
  struct symbol *function = NULL;
  char *function_name = NULL;

  pc = (*pos)++;
  op = exp->elts[pc].opcode;

  switch (op)
    {
    case OP_SCOPE:
      tem = longest_to_int (exp->elts[pc + 2].longconst);
      (*pos) += 4 + BYTES_TO_EXP_ELEM (tem + 1);
      if (noside == EVAL_SKIP)
	goto nosideret;
      arg1 = value_aggregate_elt (exp->elts[pc + 1].type,
				  &exp->elts[pc + 3].string,
				  expect_type, 0, noside);
      if (arg1 == NULL)
	error (_("There is no field named %s"), &exp->elts[pc + 3].string);
      return arg1;

    case OP_LONG:
      (*pos) += 3;
      return value_from_longest (exp->elts[pc + 1].type,
				 exp->elts[pc + 2].longconst);

    case OP_DOUBLE:
      (*pos) += 3;
      return value_from_double (exp->elts[pc + 1].type,
				exp->elts[pc + 2].doubleconst);

    case OP_DECFLOAT:
      (*pos) += 3;
      return value_from_decfloat (exp->elts[pc + 1].type,
				  exp->elts[pc + 2].decfloatconst);

    case OP_ADL_FUNC:
    case OP_VAR_VALUE:
      (*pos) += 3;
      if (noside == EVAL_SKIP)
	goto nosideret;

      /* JYG: We used to just return value_zero of the symbol type
	 if we're asked to avoid side effects.  Otherwise we return
	 value_of_variable (...).  However I'm not sure if
	 value_of_variable () has any side effect.
	 We need a full value object returned here for whatis_exp ()
	 to call evaluate_type () and then pass the full value to
	 value_rtti_target_type () if we are dealing with a pointer
	 or reference to a base class and print object is on.  */

      {
	volatile struct gdb_exception except;
	struct value *ret = NULL;

	TRY_CATCH (except, RETURN_MASK_ERROR)
	  {
	    ret = value_of_variable (exp->elts[pc + 2].symbol,
				     exp->elts[pc + 1].block);
	  }

	if (except.reason < 0)
	  {
	    if (noside == EVAL_AVOID_SIDE_EFFECTS)
	      ret = value_zero (SYMBOL_TYPE (exp->elts[pc + 2].symbol),
				not_lval);
	    else
	      throw_exception (except);
	  }

	return ret;
      }

    case OP_VAR_ENTRY_VALUE:
      (*pos) += 2;
      if (noside == EVAL_SKIP)
	goto nosideret;

      {
	struct symbol *sym = exp->elts[pc + 1].symbol;
	struct frame_info *frame;

	if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  return value_zero (SYMBOL_TYPE (sym), not_lval);

	if (SYMBOL_COMPUTED_OPS (sym) == NULL
	    || SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry == NULL)
	  error (_("Symbol \"%s\" does not have any specific entry value"),
		 SYMBOL_PRINT_NAME (sym));

	frame = get_selected_frame (NULL);
	return SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry (sym, frame);
      }

    case OP_LAST:
      (*pos) += 2;
      return
	access_value_history (longest_to_int (exp->elts[pc + 1].longconst));

    case OP_REGISTER:
      {
	const char *name = &exp->elts[pc + 2].string;
	int regno;
	struct value *val;

	(*pos) += 3 + BYTES_TO_EXP_ELEM (exp->elts[pc + 1].longconst + 1);
	regno = user_reg_map_name_to_regnum (exp->gdbarch,
					     name, strlen (name));
	if (regno == -1)
	  error (_("Register $%s not available."), name);

        /* In EVAL_AVOID_SIDE_EFFECTS mode, we only need to return
           a value with the appropriate register type.  Unfortunately,
           we don't have easy access to the type of user registers.
           So for these registers, we fetch the register value regardless
           of the evaluation mode.  */
	if (noside == EVAL_AVOID_SIDE_EFFECTS
	    && regno < gdbarch_num_regs (exp->gdbarch)
			+ gdbarch_num_pseudo_regs (exp->gdbarch))
	  val = value_zero (register_type (exp->gdbarch, regno), not_lval);
	else
	  val = value_of_register (regno, get_selected_frame (NULL));
	if (val == NULL)
	  error (_("Value of register %s not available."), name);
	else
	  return val;
      }
    case OP_BOOL:
      (*pos) += 2;
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return value_from_longest (type, exp->elts[pc + 1].longconst);

    case OP_INTERNALVAR:
      (*pos) += 2;
      return value_of_internalvar (exp->gdbarch,
				   exp->elts[pc + 1].internalvar);

    case OP_STRING:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      if (noside == EVAL_SKIP)
	goto nosideret;
      type = language_string_char_type (exp->language_defn, exp->gdbarch);
      return value_string (&exp->elts[pc + 2].string, tem, type);

    case OP_OBJC_NSSTRING:		/* Objective C Foundation Class
					   NSString constant.  */
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      if (noside == EVAL_SKIP)
	{
	  goto nosideret;
	}
      return value_nsstring (exp->gdbarch, &exp->elts[pc + 2].string, tem + 1);

    case OP_ARRAY:
      (*pos) += 3;
      tem2 = longest_to_int (exp->elts[pc + 1].longconst);
      tem3 = longest_to_int (exp->elts[pc + 2].longconst);
      nargs = tem3 - tem2 + 1;
      type = expect_type ? check_typedef (expect_type) : NULL_TYPE;

      if (expect_type != NULL_TYPE && noside != EVAL_SKIP
	  && TYPE_CODE (type) == TYPE_CODE_STRUCT)
	{
	  struct value *rec = allocate_value (expect_type);

	  memset (value_contents_raw (rec), '\0', TYPE_LENGTH (type));
	  return evaluate_struct_tuple (rec, exp, pos, noside, nargs);
	}

      if (expect_type != NULL_TYPE && noside != EVAL_SKIP
	  && TYPE_CODE (type) == TYPE_CODE_ARRAY)
	{
	  struct type *range_type = TYPE_INDEX_TYPE (type);
	  struct type *element_type = TYPE_TARGET_TYPE (type);
	  struct value *array = allocate_value (expect_type);
	  int element_size = TYPE_LENGTH (check_typedef (element_type));
	  LONGEST low_bound, high_bound, index;

	  if (get_discrete_bounds (range_type, &low_bound, &high_bound) < 0)
	    {
	      low_bound = 0;
	      high_bound = (TYPE_LENGTH (type) / element_size) - 1;
	    }
	  index = low_bound;
	  memset (value_contents_raw (array), 0, TYPE_LENGTH (expect_type));
	  for (tem = nargs; --nargs >= 0;)
	    {
	      struct value *element;
	      int index_pc = 0;

	      if (exp->elts[*pos].opcode == BINOP_RANGE)
		{
		  index_pc = ++(*pos);
		  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
		}
	      element = evaluate_subexp (element_type, exp, pos, noside);
	      if (value_type (element) != element_type)
		element = value_cast (element_type, element);
	      if (index_pc)
		{
		  int continue_pc = *pos;

		  *pos = index_pc;
		  index = init_array_element (array, element, exp, pos, noside,
					      low_bound, high_bound);
		  *pos = continue_pc;
		}
	      else
		{
		  if (index > high_bound)
		    /* To avoid memory corruption.  */
		    error (_("Too many array elements"));
		  memcpy (value_contents_raw (array)
			  + (index - low_bound) * element_size,
			  value_contents (element),
			  element_size);
		}
	      index++;
	    }
	  return array;
	}

      if (expect_type != NULL_TYPE && noside != EVAL_SKIP
	  && TYPE_CODE (type) == TYPE_CODE_SET)
	{
	  struct value *set = allocate_value (expect_type);
	  gdb_byte *valaddr = value_contents_raw (set);
	  struct type *element_type = TYPE_INDEX_TYPE (type);
	  struct type *check_type = element_type;
	  LONGEST low_bound, high_bound;

	  /* Get targettype of elementtype.  */
	  while (TYPE_CODE (check_type) == TYPE_CODE_RANGE
		 || TYPE_CODE (check_type) == TYPE_CODE_TYPEDEF)
	    check_type = TYPE_TARGET_TYPE (check_type);

	  if (get_discrete_bounds (element_type, &low_bound, &high_bound) < 0)
	    error (_("(power)set type with unknown size"));
	  memset (valaddr, '\0', TYPE_LENGTH (type));
	  for (tem = 0; tem < nargs; tem++)
	    {
	      LONGEST range_low, range_high;
	      struct type *range_low_type, *range_high_type;
	      struct value *elem_val;

	      if (exp->elts[*pos].opcode == BINOP_RANGE)
		{
		  (*pos)++;
		  elem_val = evaluate_subexp (element_type, exp, pos, noside);
		  range_low_type = value_type (elem_val);
		  range_low = value_as_long (elem_val);
		  elem_val = evaluate_subexp (element_type, exp, pos, noside);
		  range_high_type = value_type (elem_val);
		  range_high = value_as_long (elem_val);
		}
	      else
		{
		  elem_val = evaluate_subexp (element_type, exp, pos, noside);
		  range_low_type = range_high_type = value_type (elem_val);
		  range_low = range_high = value_as_long (elem_val);
		}
	      /* Check types of elements to avoid mixture of elements from
	         different types. Also check if type of element is "compatible"
	         with element type of powerset.  */
	      if (TYPE_CODE (range_low_type) == TYPE_CODE_RANGE)
		range_low_type = TYPE_TARGET_TYPE (range_low_type);
	      if (TYPE_CODE (range_high_type) == TYPE_CODE_RANGE)
		range_high_type = TYPE_TARGET_TYPE (range_high_type);
	      if ((TYPE_CODE (range_low_type) != TYPE_CODE (range_high_type))
		  || (TYPE_CODE (range_low_type) == TYPE_CODE_ENUM
		      && (range_low_type != range_high_type)))
		/* different element modes.  */
		error (_("POWERSET tuple elements of different mode"));
	      if ((TYPE_CODE (check_type) != TYPE_CODE (range_low_type))
		  || (TYPE_CODE (check_type) == TYPE_CODE_ENUM
		      && range_low_type != check_type))
		error (_("incompatible POWERSET tuple elements"));
	      if (range_low > range_high)
		{
		  warning (_("empty POWERSET tuple range"));
		  continue;
		}
	      if (range_low < low_bound || range_high > high_bound)
		error (_("POWERSET tuple element out of range"));
	      range_low -= low_bound;
	      range_high -= low_bound;
	      for (; range_low <= range_high; range_low++)
		{
		  int bit_index = (unsigned) range_low % TARGET_CHAR_BIT;

		  if (gdbarch_bits_big_endian (exp->gdbarch))
		    bit_index = TARGET_CHAR_BIT - 1 - bit_index;
		  valaddr[(unsigned) range_low / TARGET_CHAR_BIT]
		    |= 1 << bit_index;
		}
	    }
	  return set;
	}

      argvec = (struct value **) alloca (sizeof (struct value *) * nargs);
      for (tem = 0; tem < nargs; tem++)
	{
	  /* Ensure that array expressions are coerced into pointer
	     objects.  */
	  argvec[tem] = evaluate_subexp_with_coercion (exp, pos, noside);
	}
      if (noside == EVAL_SKIP)
	goto nosideret;
      return value_array (tem2, tem3, argvec);

    case TERNOP_SLICE:
      {
	struct value *array = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	int lowbound
	  = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));
	int upper
	  = value_as_long (evaluate_subexp (NULL_TYPE, exp, pos, noside));

	if (noside == EVAL_SKIP)
	  goto nosideret;
	return value_slice (array, lowbound, upper - lowbound + 1);
      }

    case TERNOP_COND:
      /* Skip third and second args to evaluate the first one.  */
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (value_logical_not (arg1))
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
	  return evaluate_subexp (NULL_TYPE, exp, pos, noside);
	}
      else
	{
	  arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
	  return arg2;
	}

    case OP_OBJC_SELECTOR:
      {				/* Objective C @selector operator.  */
	char *sel = &exp->elts[pc + 2].string;
	int len = longest_to_int (exp->elts[pc + 1].longconst);
	struct type *selector_type;

	(*pos) += 3 + BYTES_TO_EXP_ELEM (len + 1);
	if (noside == EVAL_SKIP)
	  goto nosideret;

	if (sel[len] != 0)
	  sel[len] = 0;		/* Make sure it's terminated.  */

	selector_type = builtin_type (exp->gdbarch)->builtin_data_ptr;
	return value_from_longest (selector_type,
				   lookup_child_selector (exp->gdbarch, sel));
      }

    case OP_OBJC_MSGCALL:
      {				/* Objective C message (method) call.  */

	CORE_ADDR responds_selector = 0;
	CORE_ADDR method_selector = 0;

	CORE_ADDR selector = 0;

	int struct_return = 0;
	int sub_no_side = 0;

	struct value *msg_send = NULL;
	struct value *msg_send_stret = NULL;
	int gnu_runtime = 0;

	struct value *target = NULL;
	struct value *method = NULL;
	struct value *called_method = NULL; 

	struct type *selector_type = NULL;
	struct type *long_type;

	struct value *ret = NULL;
	CORE_ADDR addr = 0;

	selector = exp->elts[pc + 1].longconst;
	nargs = exp->elts[pc + 2].longconst;
	argvec = (struct value **) alloca (sizeof (struct value *) 
					   * (nargs + 5));

	(*pos) += 3;

	long_type = builtin_type (exp->gdbarch)->builtin_long;
	selector_type = builtin_type (exp->gdbarch)->builtin_data_ptr;

	if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  sub_no_side = EVAL_NORMAL;
	else
	  sub_no_side = noside;

	target = evaluate_subexp (selector_type, exp, pos, sub_no_side);

	if (value_as_long (target) == 0)
 	  return value_from_longest (long_type, 0);
	
	if (lookup_minimal_symbol ("objc_msg_lookup", 0, 0))
	  gnu_runtime = 1;
	
	/* Find the method dispatch (Apple runtime) or method lookup
	   (GNU runtime) function for Objective-C.  These will be used
	   to lookup the symbol information for the method.  If we
	   can't find any symbol information, then we'll use these to
	   call the method, otherwise we can call the method
	   directly.  The msg_send_stret function is used in the special
	   case of a method that returns a structure (Apple runtime 
	   only).  */
	if (gnu_runtime)
	  {
	    struct type *type = selector_type;

	    type = lookup_function_type (type);
	    type = lookup_pointer_type (type);
	    type = lookup_function_type (type);
	    type = lookup_pointer_type (type);

	    msg_send = find_function_in_inferior ("objc_msg_lookup", NULL);
	    msg_send_stret
	      = find_function_in_inferior ("objc_msg_lookup", NULL);

	    msg_send = value_from_pointer (type, value_as_address (msg_send));
	    msg_send_stret = value_from_pointer (type, 
					value_as_address (msg_send_stret));
	  }
	else
	  {
	    msg_send = find_function_in_inferior ("objc_msgSend", NULL);
	    /* Special dispatcher for methods returning structs.  */
	    msg_send_stret
	      = find_function_in_inferior ("objc_msgSend_stret", NULL);
	  }

	/* Verify the target object responds to this method.  The
	   standard top-level 'Object' class uses a different name for
	   the verification method than the non-standard, but more
	   often used, 'NSObject' class.  Make sure we check for both.  */

	responds_selector
	  = lookup_child_selector (exp->gdbarch, "respondsToSelector:");
	if (responds_selector == 0)
	  responds_selector
	    = lookup_child_selector (exp->gdbarch, "respondsTo:");
	
	if (responds_selector == 0)
	  error (_("no 'respondsTo:' or 'respondsToSelector:' method"));
	
	method_selector
	  = lookup_child_selector (exp->gdbarch, "methodForSelector:");
	if (method_selector == 0)
	  method_selector
	    = lookup_child_selector (exp->gdbarch, "methodFor:");
	
	if (method_selector == 0)
	  error (_("no 'methodFor:' or 'methodForSelector:' method"));

	/* Call the verification method, to make sure that the target
	 class implements the desired method.  */

	argvec[0] = msg_send;
	argvec[1] = target;
	argvec[2] = value_from_longest (long_type, responds_selector);
	argvec[3] = value_from_longest (long_type, selector);
	argvec[4] = 0;

	ret = call_function_by_hand (argvec[0], 3, argvec + 1);
	if (gnu_runtime)
	  {
	    /* Function objc_msg_lookup returns a pointer.  */
	    argvec[0] = ret;
	    ret = call_function_by_hand (argvec[0], 3, argvec + 1);
	  }
	if (value_as_long (ret) == 0)
	  error (_("Target does not respond to this message selector."));

	/* Call "methodForSelector:" method, to get the address of a
	   function method that implements this selector for this
	   class.  If we can find a symbol at that address, then we
	   know the return type, parameter types etc.  (that's a good
	   thing).  */

	argvec[0] = msg_send;
	argvec[1] = target;
	argvec[2] = value_from_longest (long_type, method_selector);
	argvec[3] = value_from_longest (long_type, selector);
	argvec[4] = 0;

	ret = call_function_by_hand (argvec[0], 3, argvec + 1);
	if (gnu_runtime)
	  {
	    argvec[0] = ret;
	    ret = call_function_by_hand (argvec[0], 3, argvec + 1);
	  }

	/* ret should now be the selector.  */

	addr = value_as_long (ret);
	if (addr)
	  {
	    struct symbol *sym = NULL;

	    /* The address might point to a function descriptor;
	       resolve it to the actual code address instead.  */
	    addr = gdbarch_convert_from_func_ptr_addr (exp->gdbarch, addr,
						       &current_target);

	    /* Is it a high_level symbol?  */
	    sym = find_pc_function (addr);
	    if (sym != NULL) 
	      method = value_of_variable (sym, 0);
	  }

	/* If we found a method with symbol information, check to see
           if it returns a struct.  Otherwise assume it doesn't.  */

	if (method)
	  {
	    CORE_ADDR funaddr;
	    struct type *val_type;

	    funaddr = find_function_addr (method, &val_type);

	    block_for_pc (funaddr);

	    CHECK_TYPEDEF (val_type);
	  
	    if ((val_type == NULL) 
		|| (TYPE_CODE(val_type) == TYPE_CODE_ERROR))
	      {
		if (expect_type != NULL)
		  val_type = expect_type;
	      }

	    struct_return = using_struct_return (exp->gdbarch, method,
						 val_type);
	  }
	else if (expect_type != NULL)
	  {
	    struct_return = using_struct_return (exp->gdbarch, NULL,
						 check_typedef (expect_type));
	  }
	
	/* Found a function symbol.  Now we will substitute its
	   value in place of the message dispatcher (obj_msgSend),
	   so that we call the method directly instead of thru
	   the dispatcher.  The main reason for doing this is that
	   we can now evaluate the return value and parameter values
	   according to their known data types, in case we need to
	   do things like promotion, dereferencing, special handling
	   of structs and doubles, etc.
	  
	   We want to use the type signature of 'method', but still
	   jump to objc_msgSend() or objc_msgSend_stret() to better
	   mimic the behavior of the runtime.  */
	
	if (method)
	  {
	    if (TYPE_CODE (value_type (method)) != TYPE_CODE_FUNC)
	      error (_("method address has symbol information "
		       "with non-function type; skipping"));

	    /* Create a function pointer of the appropriate type, and
	       replace its value with the value of msg_send or
	       msg_send_stret.  We must use a pointer here, as
	       msg_send and msg_send_stret are of pointer type, and
	       the representation may be different on systems that use
	       function descriptors.  */
	    if (struct_return)
	      called_method
		= value_from_pointer (lookup_pointer_type (value_type (method)),
				      value_as_address (msg_send_stret));
	    else
	      called_method
		= value_from_pointer (lookup_pointer_type (value_type (method)),
				      value_as_address (msg_send));
	  }
	else
	  {
	    if (struct_return)
	      called_method = msg_send_stret;
	    else
	      called_method = msg_send;
	  }

	if (noside == EVAL_SKIP)
	  goto nosideret;

	if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  {
	    /* If the return type doesn't look like a function type,
	       call an error.  This can happen if somebody tries to
	       turn a variable into a function call.  This is here
	       because people often want to call, eg, strcmp, which
	       gdb doesn't know is a function.  If gdb isn't asked for
	       it's opinion (ie. through "whatis"), it won't offer
	       it.  */

	    struct type *type = value_type (called_method);

	    if (type && TYPE_CODE (type) == TYPE_CODE_PTR)
	      type = TYPE_TARGET_TYPE (type);
	    type = TYPE_TARGET_TYPE (type);

	    if (type)
	    {
	      if ((TYPE_CODE (type) == TYPE_CODE_ERROR) && expect_type)
		return allocate_value (expect_type);
	      else
		return allocate_value (type);
	    }
	    else
	      error (_("Expression of type other than "
		       "\"method returning ...\" used as a method"));
	  }

	/* Now depending on whether we found a symbol for the method,
	   we will either call the runtime dispatcher or the method
	   directly.  */

	argvec[0] = called_method;
	argvec[1] = target;
	argvec[2] = value_from_longest (long_type, selector);
	/* User-supplied arguments.  */
	for (tem = 0; tem < nargs; tem++)
	  argvec[tem + 3] = evaluate_subexp_with_coercion (exp, pos, noside);
	argvec[tem + 3] = 0;

	if (gnu_runtime && (method != NULL))
	  {
	    /* Function objc_msg_lookup returns a pointer.  */
	    deprecated_set_value_type (argvec[0],
				       lookup_pointer_type (lookup_function_type (value_type (argvec[0]))));
	    argvec[0]
	      = call_function_by_hand (argvec[0], nargs + 2, argvec + 1);
	  }

	ret = call_function_by_hand (argvec[0], nargs + 2, argvec + 1);
	return ret;
      }
      break;

    case OP_FUNCALL:
      (*pos) += 2;
      op = exp->elts[*pos].opcode;
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      /* Allocate arg vector, including space for the function to be
         called in argvec[0] and a terminating NULL.  */
      argvec = (struct value **)
	alloca (sizeof (struct value *) * (nargs + 3));
      if (op == STRUCTOP_MEMBER || op == STRUCTOP_MPTR)
	{
	  /* First, evaluate the structure into arg2.  */
	  pc2 = (*pos)++;

	  if (noside == EVAL_SKIP)
	    goto nosideret;

	  if (op == STRUCTOP_MEMBER)
	    {
	      arg2 = evaluate_subexp_for_address (exp, pos, noside);
	    }
	  else
	    {
	      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	    }

	  /* If the function is a virtual function, then the
	     aggregate value (providing the structure) plays
	     its part by providing the vtable.  Otherwise,
	     it is just along for the ride: call the function
	     directly.  */

	  arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

	  type = check_typedef (value_type (arg1));
	  if (TYPE_CODE (type) == TYPE_CODE_METHODPTR)
	    {
	      if (noside == EVAL_AVOID_SIDE_EFFECTS)
		arg1 = value_zero (TYPE_TARGET_TYPE (type), not_lval);
	      else
		arg1 = cplus_method_ptr_to_value (&arg2, arg1);

	      /* Now, say which argument to start evaluating from.  */
	      nargs++;
	      tem = 2;
	      argvec[1] = arg2;
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_MEMBERPTR)
	    {
	      struct type *type_ptr
		= lookup_pointer_type (TYPE_DOMAIN_TYPE (type));
	      struct type *target_type_ptr
		= lookup_pointer_type (TYPE_TARGET_TYPE (type));

	      /* Now, convert these values to an address.  */
	      arg2 = value_cast (type_ptr, arg2);

	      mem_offset = value_as_long (arg1);

	      arg1 = value_from_pointer (target_type_ptr,
					 value_as_long (arg2) + mem_offset);
	      arg1 = value_ind (arg1);
	      tem = 1;
	    }
	  else
	    error (_("Non-pointer-to-member value used in pointer-to-member "
		     "construct"));
	}
      else if (op == STRUCTOP_STRUCT || op == STRUCTOP_PTR)
	{
	  /* Hair for method invocations.  */
	  int tem2;

	  nargs++;
	  /* First, evaluate the structure into arg2.  */
	  pc2 = (*pos)++;
	  tem2 = longest_to_int (exp->elts[pc2 + 1].longconst);
	  *pos += 3 + BYTES_TO_EXP_ELEM (tem2 + 1);
	  if (noside == EVAL_SKIP)
	    goto nosideret;

	  if (op == STRUCTOP_STRUCT)
	    {
	      /* If v is a variable in a register, and the user types
	         v.method (), this will produce an error, because v has
	         no address.

	         A possible way around this would be to allocate a
	         copy of the variable on the stack, copy in the
	         contents, call the function, and copy out the
	         contents.  I.e. convert this from call by reference
	         to call by copy-return (or whatever it's called).
	         However, this does not work because it is not the
	         same: the method being called could stash a copy of
	         the address, and then future uses through that address
	         (after the method returns) would be expected to
	         use the variable itself, not some copy of it.  */
	      arg2 = evaluate_subexp_for_address (exp, pos, noside);
	    }
	  else
	    {
	      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

	      /* Check to see if the operator '->' has been
	         overloaded.  If the operator has been overloaded
	         replace arg2 with the value returned by the custom
	         operator and continue evaluation.  */
	      while (unop_user_defined_p (op, arg2))
		{
		  volatile struct gdb_exception except;
		  struct value *value = NULL;
		  TRY_CATCH (except, RETURN_MASK_ERROR)
		    {
		      value = value_x_unop (arg2, op, noside);
		    }

		  if (except.reason < 0)
		    {
		      if (except.error == NOT_FOUND_ERROR)
			break;
		      else
			throw_exception (except);
		    }
		  arg2 = value;
		}
	    }
	  /* Now, say which argument to start evaluating from.  */
	  tem = 2;
	}
      else if (op == OP_SCOPE
	       && overload_resolution
	       && (exp->language_defn->la_language == language_cplus))
	{
	  /* Unpack it locally so we can properly handle overload
	     resolution.  */
	  char *name;
	  int local_tem;

	  pc2 = (*pos)++;
	  local_tem = longest_to_int (exp->elts[pc2 + 2].longconst);
	  (*pos) += 4 + BYTES_TO_EXP_ELEM (local_tem + 1);
	  type = exp->elts[pc2 + 1].type;
	  name = &exp->elts[pc2 + 3].string;

	  function = NULL;
	  function_name = NULL;
	  if (TYPE_CODE (type) == TYPE_CODE_NAMESPACE)
	    {
	      function = cp_lookup_symbol_namespace (TYPE_TAG_NAME (type),
						     name,
						     get_selected_block (0),
						     VAR_DOMAIN);
	      if (function == NULL)
		error (_("No symbol \"%s\" in namespace \"%s\"."), 
		       name, TYPE_TAG_NAME (type));

	      tem = 1;
	    }
	  else
	    {
	      gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
			  || TYPE_CODE (type) == TYPE_CODE_UNION);
	      function_name = name;

	      arg2 = value_zero (type, lval_memory);
	      ++nargs;
	      tem = 2;
	    }
	}
      else if (op == OP_ADL_FUNC)
        {
          /* Save the function position and move pos so that the arguments
             can be evaluated.  */
          int func_name_len;

          save_pos1 = *pos;
          tem = 1;

          func_name_len = longest_to_int (exp->elts[save_pos1 + 3].longconst);
          (*pos) += 6 + BYTES_TO_EXP_ELEM (func_name_len + 1);
        }
      else
	{
	  /* Non-method function call.  */
	  save_pos1 = *pos;
	  tem = 1;

	  /* If this is a C++ function wait until overload resolution.  */
	  if (op == OP_VAR_VALUE
	      && overload_resolution
	      && (exp->language_defn->la_language == language_cplus))
	    {
	      (*pos) += 4; /* Skip the evaluation of the symbol.  */
	      argvec[0] = NULL;
	    }
	  else
	    {
	      argvec[0] = evaluate_subexp_with_coercion (exp, pos, noside);
	      type = value_type (argvec[0]);
	      if (type && TYPE_CODE (type) == TYPE_CODE_PTR)
		type = TYPE_TARGET_TYPE (type);
	      if (type && TYPE_CODE (type) == TYPE_CODE_FUNC)
		{
		  for (; tem <= nargs && tem <= TYPE_NFIELDS (type); tem++)
		    {
		      argvec[tem] = evaluate_subexp (TYPE_FIELD_TYPE (type,
								      tem - 1),
						     exp, pos, noside);
		    }
		}
	    }
	}

      /* Evaluate arguments.  */
      for (; tem <= nargs; tem++)
	{
	  /* Ensure that array expressions are coerced into pointer
	     objects.  */
	  argvec[tem] = evaluate_subexp_with_coercion (exp, pos, noside);
	}

      /* Signal end of arglist.  */
      argvec[tem] = 0;
      if (op == OP_ADL_FUNC)
        {
          struct symbol *symp;
          char *func_name;
          int  name_len;
          int string_pc = save_pos1 + 3;

          /* Extract the function name.  */
          name_len = longest_to_int (exp->elts[string_pc].longconst);
          func_name = (char *) alloca (name_len + 1);
          strcpy (func_name, &exp->elts[string_pc + 1].string);

          find_overload_match (&argvec[1], nargs, func_name,
                               NON_METHOD, /* not method */
                               NULL, NULL, /* pass NULL symbol since
					      symbol is unknown */
                               NULL, &symp, NULL, 0);

          /* Now fix the expression being evaluated.  */
          exp->elts[save_pos1 + 2].symbol = symp;
          argvec[0] = evaluate_subexp_with_coercion (exp, &save_pos1, noside);
        }

      if (op == STRUCTOP_STRUCT || op == STRUCTOP_PTR
	  || (op == OP_SCOPE && function_name != NULL))
	{
	  int static_memfuncp;
	  char *tstr;

	  /* Method invocation : stuff "this" as first parameter.  */
	  argvec[1] = arg2;

	  if (op != OP_SCOPE)
	    {
	      /* Name of method from expression.  */
	      tstr = &exp->elts[pc2 + 2].string;
	    }
	  else
	    tstr = function_name;

	  if (overload_resolution && (exp->language_defn->la_language
				      == language_cplus))
	    {
	      /* Language is C++, do some overload resolution before
		 evaluation.  */
	      struct value *valp = NULL;

	      (void) find_overload_match (&argvec[1], nargs, tstr,
	                                  METHOD, /* method */
					  &arg2,  /* the object */
					  NULL, &valp, NULL,
					  &static_memfuncp, 0);

	      if (op == OP_SCOPE && !static_memfuncp)
		{
		  /* For the time being, we don't handle this.  */
		  error (_("Call to overloaded function %s requires "
			   "`this' pointer"),
			 function_name);
		}
	      argvec[1] = arg2;	/* the ``this'' pointer */
	      argvec[0] = valp;	/* Use the method found after overload
				   resolution.  */
	    }
	  else
	    /* Non-C++ case -- or no overload resolution.  */
	    {
	      struct value *temp = arg2;

	      argvec[0] = value_struct_elt (&temp, argvec + 1, tstr,
					    &static_memfuncp,
					    op == STRUCTOP_STRUCT
				       ? "structure" : "structure pointer");
	      /* value_struct_elt updates temp with the correct value
	 	 of the ``this'' pointer if necessary, so modify argvec[1] to
		 reflect any ``this'' changes.  */
	      arg2
		= value_from_longest (lookup_pointer_type(value_type (temp)),
				      value_address (temp)
				      + value_embedded_offset (temp));
	      argvec[1] = arg2;	/* the ``this'' pointer */
	    }

	  if (static_memfuncp)
	    {
	      argvec[1] = argvec[0];
	      nargs--;
	      argvec++;
	    }
	}
      else if (op == STRUCTOP_MEMBER || op == STRUCTOP_MPTR)
	{
	  /* Pointer to member.  argvec[1] is already set up.  */
	  argvec[0] = arg1;
	}
      else if (op == OP_VAR_VALUE || (op == OP_SCOPE && function != NULL))
	{
	  /* Non-member function being called.  */
          /* fn: This can only be done for C++ functions.  A C-style function
             in a C++ program, for instance, does not have the fields that 
             are expected here.  */

	  if (overload_resolution && (exp->language_defn->la_language
				      == language_cplus))
	    {
	      /* Language is C++, do some overload resolution before
		 evaluation.  */
	      struct symbol *symp;
	      int no_adl = 0;

	      /* If a scope has been specified disable ADL.  */
	      if (op == OP_SCOPE)
		no_adl = 1;

	      if (op == OP_VAR_VALUE)
		function = exp->elts[save_pos1+2].symbol;

	      (void) find_overload_match (&argvec[1], nargs,
					  NULL,        /* no need for name */
	                                  NON_METHOD,  /* not method */
	                                  NULL, function, /* the function */
					  NULL, &symp, NULL, no_adl);

	      if (op == OP_VAR_VALUE)
		{
		  /* Now fix the expression being evaluated.  */
		  exp->elts[save_pos1+2].symbol = symp;
		  argvec[0] = evaluate_subexp_with_coercion (exp, &save_pos1,
							     noside);
		}
	      else
		argvec[0] = value_of_variable (symp, get_selected_block (0));
	    }
	  else
	    {
	      /* Not C++, or no overload resolution allowed.  */
	      /* Nothing to be done; argvec already correctly set up.  */
	    }
	}
      else
	{
	  /* It is probably a C-style function.  */
	  /* Nothing to be done; argvec already correctly set up.  */
	}

    do_call_it:

      if (noside == EVAL_SKIP)
	goto nosideret;
      if (argvec[0] == NULL)
	error (_("Cannot evaluate function -- may be inlined"));
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  /* If the return type doesn't look like a function type, call an
	     error.  This can happen if somebody tries to turn a variable into
	     a function call.  This is here because people often want to
	     call, eg, strcmp, which gdb doesn't know is a function.  If
	     gdb isn't asked for it's opinion (ie. through "whatis"),
	     it won't offer it.  */

	  struct type *ftype = value_type (argvec[0]);

	  if (TYPE_CODE (ftype) == TYPE_CODE_INTERNAL_FUNCTION)
	    {
	      /* We don't know anything about what the internal
		 function might return, but we have to return
		 something.  */
	      return value_zero (builtin_type (exp->gdbarch)->builtin_int,
				 not_lval);
	    }
	  else if (TYPE_GNU_IFUNC (ftype))
	    return allocate_value (TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (ftype)));
	  else if (TYPE_TARGET_TYPE (ftype))
	    return allocate_value (TYPE_TARGET_TYPE (ftype));
	  else
	    error (_("Expression of type other than "
		     "\"Function returning ...\" used as function"));
	}
      if (TYPE_CODE (value_type (argvec[0])) == TYPE_CODE_INTERNAL_FUNCTION)
	return call_internal_function (exp->gdbarch, exp->language_defn,
				       argvec[0], nargs, argvec + 1);

      return call_function_by_hand (argvec[0], nargs, argvec + 1);
      /* pai: FIXME save value from call_function_by_hand, then adjust
	 pc by adjust_fn_pc if +ve.  */

    case OP_F77_UNDETERMINED_ARGLIST:

      /* Remember that in F77, functions, substring ops and 
         array subscript operations cannot be disambiguated 
         at parse time.  We have made all array subscript operations, 
         substring operations as well as function calls  come here 
         and we now have to discover what the heck this thing actually was.
         If it is a function, we process just as if we got an OP_FUNCALL.  */

      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 2;

      /* First determine the type code we are dealing with.  */
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      type = check_typedef (value_type (arg1));
      code = TYPE_CODE (type);

      if (code == TYPE_CODE_PTR)
	{
	  /* Fortran always passes variable to subroutines as pointer.
	     So we need to look into its target type to see if it is
	     array, string or function.  If it is, we need to switch
	     to the target value the original one points to.  */ 
	  struct type *target_type = check_typedef (TYPE_TARGET_TYPE (type));

	  if (TYPE_CODE (target_type) == TYPE_CODE_ARRAY
	      || TYPE_CODE (target_type) == TYPE_CODE_STRING
	      || TYPE_CODE (target_type) == TYPE_CODE_FUNC)
	    {
	      arg1 = value_ind (arg1);
	      type = check_typedef (value_type (arg1));
	      code = TYPE_CODE (type);
	    }
	} 

      switch (code)
	{
	case TYPE_CODE_ARRAY:
	  if (exp->elts[*pos].opcode == OP_F90_RANGE)
	    return value_f90_subarray (arg1, exp, pos, noside);
	  else
	    goto multi_f77_subscript;

	case TYPE_CODE_STRING:
	  if (exp->elts[*pos].opcode == OP_F90_RANGE)
	    return value_f90_subarray (arg1, exp, pos, noside);
	  else
	    {
	      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
	      return value_subscript (arg1, value_as_long (arg2));
	    }

	case TYPE_CODE_PTR:
	case TYPE_CODE_FUNC:
	  /* It's a function call.  */
	  /* Allocate arg vector, including space for the function to be
	     called in argvec[0] and a terminating NULL.  */
	  argvec = (struct value **)
	    alloca (sizeof (struct value *) * (nargs + 2));
	  argvec[0] = arg1;
	  tem = 1;
	  for (; tem <= nargs; tem++)
	    argvec[tem] = evaluate_subexp_with_coercion (exp, pos, noside);
	  argvec[tem] = 0;	/* signal end of arglist */
	  goto do_call_it;

	default:
	  error (_("Cannot perform substring on this type"));
	}

    case OP_COMPLEX:
      /* We have a complex number, There should be 2 floating 
         point numbers that compose it.  */
      (*pos) += 2;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      return value_literal_complex (arg1, arg2, exp->elts[pc + 1].type);

    case STRUCTOP_STRUCT:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      arg3 = value_struct_elt (&arg1, NULL, &exp->elts[pc + 2].string,
			       NULL, "structure");
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	arg3 = value_zero (value_type (arg3), not_lval);
      return arg3;

    case STRUCTOP_PTR:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;

      /* Check to see if operator '->' has been overloaded.  If so replace
         arg1 with the value returned by evaluating operator->().  */
      while (unop_user_defined_p (op, arg1))
	{
	  volatile struct gdb_exception except;
	  struct value *value = NULL;
	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      value = value_x_unop (arg1, op, noside);
	    }

	  if (except.reason < 0)
	    {
	      if (except.error == NOT_FOUND_ERROR)
		break;
	      else
		throw_exception (except);
	    }
	  arg1 = value;
	}

      /* JYG: if print object is on we need to replace the base type
	 with rtti type in order to continue on with successful
	 lookup of member / method only available in the rtti type.  */
      {
        struct type *type = value_type (arg1);
        struct type *real_type;
        int full, top, using_enc;
	struct value_print_options opts;

	get_user_print_options (&opts);
        if (opts.objectprint && TYPE_TARGET_TYPE(type)
            && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_CLASS))
          {
            real_type = value_rtti_indirect_type (arg1, &full, &top,
						  &using_enc);
            if (real_type)
                arg1 = value_cast (real_type, arg1);
          }
      }

      arg3 = value_struct_elt (&arg1, NULL, &exp->elts[pc + 2].string,
			       NULL, "structure pointer");
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	arg3 = value_zero (value_type (arg3), not_lval);
      return arg3;

    case STRUCTOP_MEMBER:
    case STRUCTOP_MPTR:
      if (op == STRUCTOP_MEMBER)
	arg1 = evaluate_subexp_for_address (exp, pos, noside);
      else
	arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      if (noside == EVAL_SKIP)
	goto nosideret;

      type = check_typedef (value_type (arg2));
      switch (TYPE_CODE (type))
	{
	case TYPE_CODE_METHODPTR:
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    return value_zero (TYPE_TARGET_TYPE (type), not_lval);
	  else
	    {
	      arg2 = cplus_method_ptr_to_value (&arg1, arg2);
	      gdb_assert (TYPE_CODE (value_type (arg2)) == TYPE_CODE_PTR);
	      return value_ind (arg2);
	    }

	case TYPE_CODE_MEMBERPTR:
	  /* Now, convert these values to an address.  */
	  arg1 = value_cast_pointers (lookup_pointer_type (TYPE_DOMAIN_TYPE (type)),
				      arg1, 1);

	  mem_offset = value_as_long (arg2);

	  arg3 = value_from_pointer (lookup_pointer_type (TYPE_TARGET_TYPE (type)),
				     value_as_long (arg1) + mem_offset);
	  return value_ind (arg3);

	default:
	  error (_("non-pointer-to-member value used "
		   "in pointer-to-member construct"));
	}

    case TYPE_INSTANCE:
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      arg_types = (struct type **) alloca (nargs * sizeof (struct type *));
      for (ix = 0; ix < nargs; ++ix)
	arg_types[ix] = exp->elts[pc + 1 + ix + 1].type;

      expect_type = make_params (nargs, arg_types);
      *(pos) += 3 + nargs;
      arg1 = evaluate_subexp_standard (expect_type, exp, pos, noside);
      xfree (TYPE_FIELDS (expect_type));
      xfree (TYPE_MAIN_TYPE (expect_type));
      xfree (expect_type);
      return arg1;

    case BINOP_CONCAT:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else
	return value_concat (arg1, arg2);

    case BINOP_ASSIGN:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);

      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else
	return value_assign (arg1, arg2);

    case BINOP_ASSIGN_MODIFY:
      (*pos) += 2;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      op = exp->elts[pc + 1].opcode;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, BINOP_ASSIGN_MODIFY, op, noside);
      else if (op == BINOP_ADD && ptrmath_type_p (exp->language_defn,
						  value_type (arg1))
	       && is_integral_type (value_type (arg2)))
	arg2 = value_ptradd (arg1, value_as_long (arg2));
      else if (op == BINOP_SUB && ptrmath_type_p (exp->language_defn,
						  value_type (arg1))
	       && is_integral_type (value_type (arg2)))
	arg2 = value_ptradd (arg1, - value_as_long (arg2));
      else
	{
	  struct value *tmp = arg1;

	  /* For shift and integer exponentiation operations,
	     only promote the first argument.  */
	  if ((op == BINOP_LSH || op == BINOP_RSH || op == BINOP_EXP)
	      && is_integral_type (value_type (arg2)))
	    unop_promote (exp->language_defn, exp->gdbarch, &tmp);
	  else
	    binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);

	  arg2 = value_binop (tmp, arg2, op);
	}
      return value_assign (arg1, arg2);

    case BINOP_ADD:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else if (ptrmath_type_p (exp->language_defn, value_type (arg1))
	       && is_integral_type (value_type (arg2)))
	return value_ptradd (arg1, value_as_long (arg2));
      else if (ptrmath_type_p (exp->language_defn, value_type (arg2))
	       && is_integral_type (value_type (arg1)))
	return value_ptradd (arg2, value_as_long (arg1));
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  return value_binop (arg1, arg2, BINOP_ADD);
	}

    case BINOP_SUB:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else if (ptrmath_type_p (exp->language_defn, value_type (arg1))
	       && ptrmath_type_p (exp->language_defn, value_type (arg2)))
	{
	  /* FIXME -- should be ptrdiff_t */
	  type = builtin_type (exp->gdbarch)->builtin_long;
	  return value_from_longest (type, value_ptrdiff (arg1, arg2));
	}
      else if (ptrmath_type_p (exp->language_defn, value_type (arg1))
	       && is_integral_type (value_type (arg2)))
	return value_ptradd (arg1, - value_as_long (arg2));
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  return value_binop (arg1, arg2, BINOP_SUB);
	}

    case BINOP_EXP:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_INTDIV:
    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_LSH:
    case BINOP_RSH:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else
	{
	  /* If EVAL_AVOID_SIDE_EFFECTS and we're dividing by zero,
	     fudge arg2 to avoid division-by-zero, the caller is
	     (theoretically) only looking for the type of the result.  */
	  if (noside == EVAL_AVOID_SIDE_EFFECTS
	      /* ??? Do we really want to test for BINOP_MOD here?
		 The implementation of value_binop gives it a well-defined
		 value.  */
	      && (op == BINOP_DIV
		  || op == BINOP_INTDIV
		  || op == BINOP_REM
		  || op == BINOP_MOD)
	      && value_logical_not (arg2))
	    {
	      struct value *v_one, *retval;

	      v_one = value_one (value_type (arg2));
	      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &v_one);
	      retval = value_binop (arg1, v_one, op);
	      return retval;
	    }
	  else
	    {
	      /* For shift and integer exponentiation operations,
		 only promote the first argument.  */
	      if ((op == BINOP_LSH || op == BINOP_RSH || op == BINOP_EXP)
		  && is_integral_type (value_type (arg2)))
		unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	      else
		binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);

	      return value_binop (arg1, arg2, op);
	    }
	}

    case BINOP_RANGE:
      evaluate_subexp (NULL_TYPE, exp, pos, noside);
      evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      error (_("':' operator used in invalid context"));

    case BINOP_SUBSCRIPT:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	return value_x_binop (arg1, arg2, op, OP_NULL, noside);
      else
	{
	  /* If the user attempts to subscript something that is not an
	     array or pointer type (like a plain int variable for example),
	     then report this as an error.  */

	  arg1 = coerce_ref (arg1);
	  type = check_typedef (value_type (arg1));
	  if (TYPE_CODE (type) != TYPE_CODE_ARRAY
	      && TYPE_CODE (type) != TYPE_CODE_PTR)
	    {
	      if (TYPE_NAME (type))
		error (_("cannot subscript something of type `%s'"),
		       TYPE_NAME (type));
	      else
		error (_("cannot subscript requested type"));
	    }

	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    return value_zero (TYPE_TARGET_TYPE (type), VALUE_LVAL (arg1));
	  else
	    return value_subscript (arg1, value_as_long (arg2));
	}

    case BINOP_IN:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return value_from_longest (type, (LONGEST) value_in (arg1, arg2));

    case MULTI_SUBSCRIPT:
      (*pos) += 2;
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      while (nargs-- > 0)
	{
	  arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
	  /* FIXME:  EVAL_SKIP handling may not be correct.  */
	  if (noside == EVAL_SKIP)
	    {
	      if (nargs > 0)
		{
		  continue;
		}
	      else
		{
		  goto nosideret;
		}
	    }
	  /* FIXME:  EVAL_AVOID_SIDE_EFFECTS handling may not be correct.  */
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    {
	      /* If the user attempts to subscript something that has no target
	         type (like a plain int variable for example), then report this
	         as an error.  */

	      type = TYPE_TARGET_TYPE (check_typedef (value_type (arg1)));
	      if (type != NULL)
		{
		  arg1 = value_zero (type, VALUE_LVAL (arg1));
		  noside = EVAL_SKIP;
		  continue;
		}
	      else
		{
		  error (_("cannot subscript something of type `%s'"),
			 TYPE_NAME (value_type (arg1)));
		}
	    }

	  if (binop_user_defined_p (op, arg1, arg2))
	    {
	      arg1 = value_x_binop (arg1, arg2, op, OP_NULL, noside);
	    }
	  else
	    {
	      arg1 = coerce_ref (arg1);
	      type = check_typedef (value_type (arg1));

	      switch (TYPE_CODE (type))
		{
		case TYPE_CODE_PTR:
		case TYPE_CODE_ARRAY:
		case TYPE_CODE_STRING:
		  arg1 = value_subscript (arg1, value_as_long (arg2));
		  break;

		default:
		  if (TYPE_NAME (type))
		    error (_("cannot subscript something of type `%s'"),
			   TYPE_NAME (type));
		  else
		    error (_("cannot subscript requested type"));
		}
	    }
	}
      return (arg1);

    multi_f77_subscript:
      {
	LONGEST subscript_array[MAX_FORTRAN_DIMS];
	int ndimensions = 1, i;
	struct value *array = arg1;

	if (nargs > MAX_FORTRAN_DIMS)
	  error (_("Too many subscripts for F77 (%d Max)"), MAX_FORTRAN_DIMS);

	ndimensions = calc_f77_array_dims (type);

	if (nargs != ndimensions)
	  error (_("Wrong number of subscripts"));

	gdb_assert (nargs > 0);

	/* Now that we know we have a legal array subscript expression 
	   let us actually find out where this element exists in the array.  */

	/* Take array indices left to right.  */
	for (i = 0; i < nargs; i++)
	  {
	    /* Evaluate each subscript; it must be a legal integer in F77.  */
	    arg2 = evaluate_subexp_with_coercion (exp, pos, noside);

	    /* Fill in the subscript array.  */

	    subscript_array[i] = value_as_long (arg2);
	  }

	/* Internal type of array is arranged right to left.  */
	for (i = nargs; i > 0; i--)
	  {
	    struct type *array_type = check_typedef (value_type (array));
	    LONGEST index = subscript_array[i - 1];

	    array = value_subscripted_rvalue (array, index,
					      f77_get_lowerbound (array_type));
	  }

	return array;
      }

    case BINOP_LOGICAL_AND:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, noside);
	  goto nosideret;
	}

      oldpos = *pos;
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      *pos = oldpos;

      if (binop_user_defined_p (op, arg1, arg2))
	{
	  arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  tem = value_logical_not (arg1);
	  arg2 = evaluate_subexp (NULL_TYPE, exp, pos,
				  (tem ? EVAL_SKIP : noside));
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type,
			     (LONGEST) (!tem && !value_logical_not (arg2)));
	}

    case BINOP_LOGICAL_OR:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, noside);
	  goto nosideret;
	}

      oldpos = *pos;
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      *pos = oldpos;

      if (binop_user_defined_p (op, arg1, arg2))
	{
	  arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  tem = value_logical_not (arg1);
	  arg2 = evaluate_subexp (NULL_TYPE, exp, pos,
				  (!tem ? EVAL_SKIP : noside));
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type,
			     (LONGEST) (!tem || !value_logical_not (arg2)));
	}

    case BINOP_EQUAL:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_equal (arg1, arg2);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) tem);
	}

    case BINOP_NOTEQUAL:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_equal (arg1, arg2);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) ! tem);
	}

    case BINOP_LESS:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_less (arg1, arg2);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) tem);
	}

    case BINOP_GTR:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_less (arg2, arg1);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) tem);
	}

    case BINOP_GEQ:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_less (arg2, arg1) || value_equal (arg1, arg2);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) tem);
	}

    case BINOP_LEQ:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (binop_user_defined_p (op, arg1, arg2))
	{
	  return value_x_binop (arg1, arg2, op, OP_NULL, noside);
	}
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = value_less (arg1, arg2) || value_equal (arg1, arg2);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) tem);
	}

    case BINOP_REPEAT:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      type = check_typedef (value_type (arg2));
      if (TYPE_CODE (type) != TYPE_CODE_INT)
	error (_("Non-integral right operand for \"@\" operator."));
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  return allocate_repeat_value (value_type (arg1),
				     longest_to_int (value_as_long (arg2)));
	}
      else
	return value_repeat (arg1, longest_to_int (value_as_long (arg2)));

    case BINOP_COMMA:
      evaluate_subexp (NULL_TYPE, exp, pos, noside);
      return evaluate_subexp (NULL_TYPE, exp, pos, noside);

    case UNOP_PLUS:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (unop_user_defined_p (op, arg1))
	return value_x_unop (arg1, op, noside);
      else
	{
	  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  return value_pos (arg1);
	}
      
    case UNOP_NEG:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (unop_user_defined_p (op, arg1))
	return value_x_unop (arg1, op, noside);
      else
	{
	  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  return value_neg (arg1);
	}

    case UNOP_COMPLEMENT:
      /* C++: check for and handle destructor names.  */
      op = exp->elts[*pos].opcode;

      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (unop_user_defined_p (UNOP_COMPLEMENT, arg1))
	return value_x_unop (arg1, UNOP_COMPLEMENT, noside);
      else
	{
	  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  return value_complement (arg1);
	}

    case UNOP_LOGICAL_NOT:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (unop_user_defined_p (op, arg1))
	return value_x_unop (arg1, op, noside);
      else
	{
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) value_logical_not (arg1));
	}

    case UNOP_IND:
      if (expect_type && TYPE_CODE (expect_type) == TYPE_CODE_PTR)
	expect_type = TYPE_TARGET_TYPE (check_typedef (expect_type));
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      type = check_typedef (value_type (arg1));
      if (TYPE_CODE (type) == TYPE_CODE_METHODPTR
	  || TYPE_CODE (type) == TYPE_CODE_MEMBERPTR)
	error (_("Attempt to dereference pointer "
		 "to member without an object"));
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (unop_user_defined_p (op, arg1))
	return value_x_unop (arg1, op, noside);
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  type = check_typedef (value_type (arg1));
	  if (TYPE_CODE (type) == TYPE_CODE_PTR
	      || TYPE_CODE (type) == TYPE_CODE_REF
	  /* In C you can dereference an array to get the 1st elt.  */
	      || TYPE_CODE (type) == TYPE_CODE_ARRAY
	    )
	    return value_zero (TYPE_TARGET_TYPE (type),
			       lval_memory);
	  else if (TYPE_CODE (type) == TYPE_CODE_INT)
	    /* GDB allows dereferencing an int.  */
	    return value_zero (builtin_type (exp->gdbarch)->builtin_int,
			       lval_memory);
	  else
	    error (_("Attempt to take contents of a non-pointer value."));
	}

      /* Allow * on an integer so we can cast it to whatever we want.
	 This returns an int, which seems like the most C-like thing to
	 do.  "long long" variables are rare enough that
	 BUILTIN_TYPE_LONGEST would seem to be a mistake.  */
      if (TYPE_CODE (type) == TYPE_CODE_INT)
	return value_at_lazy (builtin_type (exp->gdbarch)->builtin_int,
			      (CORE_ADDR) value_as_address (arg1));
      return value_ind (arg1);

    case UNOP_ADDR:
      /* C++: check for and handle pointer to members.  */

      op = exp->elts[*pos].opcode;

      if (noside == EVAL_SKIP)
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
	  goto nosideret;
	}
      else
	{
	  struct value *retvalp = evaluate_subexp_for_address (exp, pos,
							       noside);

	  return retvalp;
	}

    case UNOP_SIZEOF:
      if (noside == EVAL_SKIP)
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
	  goto nosideret;
	}
      return evaluate_subexp_for_sizeof (exp, pos);

    case UNOP_CAST:
      (*pos) += 2;
      type = exp->elts[pc + 1].type;
      arg1 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (type != value_type (arg1))
	arg1 = value_cast (type, arg1);
      return arg1;

    case UNOP_CAST_TYPE:
      arg1 = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = value_type (arg1);
      arg1 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (type != value_type (arg1))
	arg1 = value_cast (type, arg1);
      return arg1;

    case UNOP_DYNAMIC_CAST:
      arg1 = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = value_type (arg1);
      arg1 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      return value_dynamic_cast (type, arg1);

    case UNOP_REINTERPRET_CAST:
      arg1 = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = value_type (arg1);
      arg1 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      return value_reinterpret_cast (type, arg1);

    case UNOP_MEMVAL:
      (*pos) += 2;
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value_zero (exp->elts[pc + 1].type, lval_memory);
      else
	return value_at_lazy (exp->elts[pc + 1].type,
			      value_as_address (arg1));

    case UNOP_MEMVAL_TYPE:
      arg1 = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = value_type (arg1);
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value_zero (type, lval_memory);
      else
	return value_at_lazy (type, value_as_address (arg1));

    case UNOP_MEMVAL_TLS:
      (*pos) += 3;
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value_zero (exp->elts[pc + 2].type, lval_memory);
      else
	{
	  CORE_ADDR tls_addr;

	  tls_addr = target_translate_tls_address (exp->elts[pc + 1].objfile,
						   value_as_address (arg1));
	  return value_at_lazy (exp->elts[pc + 2].type, tls_addr);
	}

    case UNOP_PREINCREMENT:
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      else if (unop_user_defined_p (op, arg1))
	{
	  return value_x_unop (arg1, op, noside);
	}
      else
	{
	  if (ptrmath_type_p (exp->language_defn, value_type (arg1)))
	    arg2 = value_ptradd (arg1, 1);
	  else
	    {
	      struct value *tmp = arg1;

	      arg2 = value_one (value_type (arg1));
	      binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	      arg2 = value_binop (tmp, arg2, BINOP_ADD);
	    }

	  return value_assign (arg1, arg2);
	}

    case UNOP_PREDECREMENT:
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      else if (unop_user_defined_p (op, arg1))
	{
	  return value_x_unop (arg1, op, noside);
	}
      else
	{
	  if (ptrmath_type_p (exp->language_defn, value_type (arg1)))
	    arg2 = value_ptradd (arg1, -1);
	  else
	    {
	      struct value *tmp = arg1;

	      arg2 = value_one (value_type (arg1));
	      binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	      arg2 = value_binop (tmp, arg2, BINOP_SUB);
	    }

	  return value_assign (arg1, arg2);
	}

    case UNOP_POSTINCREMENT:
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      else if (unop_user_defined_p (op, arg1))
	{
	  return value_x_unop (arg1, op, noside);
	}
      else
	{
	  arg3 = value_non_lval (arg1);

	  if (ptrmath_type_p (exp->language_defn, value_type (arg1)))
	    arg2 = value_ptradd (arg1, 1);
	  else
	    {
	      struct value *tmp = arg1;

	      arg2 = value_one (value_type (arg1));
	      binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	      arg2 = value_binop (tmp, arg2, BINOP_ADD);
	    }

	  value_assign (arg1, arg2);
	  return arg3;
	}

    case UNOP_POSTDECREMENT:
      arg1 = evaluate_subexp (expect_type, exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	return arg1;
      else if (unop_user_defined_p (op, arg1))
	{
	  return value_x_unop (arg1, op, noside);
	}
      else
	{
	  arg3 = value_non_lval (arg1);

	  if (ptrmath_type_p (exp->language_defn, value_type (arg1)))
	    arg2 = value_ptradd (arg1, -1);
	  else
	    {
	      struct value *tmp = arg1;

	      arg2 = value_one (value_type (arg1));
	      binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	      arg2 = value_binop (tmp, arg2, BINOP_SUB);
	    }

	  value_assign (arg1, arg2);
	  return arg3;
	}

    case OP_THIS:
      (*pos) += 1;
      return value_of_this (exp->language_defn);

    case OP_TYPE:
      /* The value is not supposed to be used.  This is here to make it
         easier to accommodate expressions that contain types.  */
      (*pos) += 2;
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *type = exp->elts[pc + 1].type;

	  /* If this is a typedef, then find its immediate target.  We
	     use check_typedef to resolve stubs, but we ignore its
	     result because we do not want to dig past all
	     typedefs.  */
	  check_typedef (type);
	  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
	    type = TYPE_TARGET_TYPE (type);
	  return allocate_value (type);
	}
      else
        error (_("Attempt to use a type name as an expression"));

    case OP_TYPEOF:
    case OP_DECLTYPE:
      if (noside == EVAL_SKIP)
	{
	  evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
	  goto nosideret;
	}
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  enum exp_opcode sub_op = exp->elts[*pos].opcode;
	  struct value *result;

	  result = evaluate_subexp (NULL_TYPE, exp, pos,
				    EVAL_AVOID_SIDE_EFFECTS);

	  /* 'decltype' has special semantics for lvalues.  */
	  if (op == OP_DECLTYPE
	      && (sub_op == BINOP_SUBSCRIPT
		  || sub_op == STRUCTOP_MEMBER
		  || sub_op == STRUCTOP_MPTR
		  || sub_op == UNOP_IND
		  || sub_op == STRUCTOP_STRUCT
		  || sub_op == STRUCTOP_PTR
		  || sub_op == OP_SCOPE))
	    {
	      struct type *type = value_type (result);

	      if (TYPE_CODE (check_typedef (type)) != TYPE_CODE_REF)
		{
		  type = lookup_reference_type (type);
		  result = allocate_value (type);
		}
	    }

	  return result;
	}
      else
        error (_("Attempt to use a type as an expression"));

    case OP_TYPEID:
      {
	struct value *result;
	enum exp_opcode sub_op = exp->elts[*pos].opcode;

	if (sub_op == OP_TYPE || sub_op == OP_DECLTYPE || sub_op == OP_TYPEOF)
	  result = evaluate_subexp (NULL_TYPE, exp, pos,
				    EVAL_AVOID_SIDE_EFFECTS);
	else
	  result = evaluate_subexp (NULL_TYPE, exp, pos, noside);

	if (noside != EVAL_NORMAL)
	  return allocate_value (cplus_typeid_type (exp->gdbarch));

	return cplus_typeid (result);
      }

    default:
      /* Removing this case and compiling with gcc -Wall reveals that
         a lot of cases are hitting this case.  Some of these should
         probably be removed from expression.h; others are legitimate
         expressions which are (apparently) not fully implemented.

         If there are any cases landing here which mean a user error,
         then they should be separate cases, with more descriptive
         error messages.  */

      error (_("GDB does not (yet) know how to "
	       "evaluate that kind of expression"));
    }

nosideret:
  return value_from_longest (builtin_type (exp->gdbarch)->builtin_int, 1);
}

/* Evaluate a subexpression of EXP, at index *POS,
   and return the address of that subexpression.
   Advance *POS over the subexpression.
   If the subexpression isn't an lvalue, get an error.
   NOSIDE may be EVAL_AVOID_SIDE_EFFECTS;
   then only the type of the result need be correct.  */

static struct value *
evaluate_subexp_for_address (struct expression *exp, int *pos,
			     enum noside noside)
{
  enum exp_opcode op;
  int pc;
  struct symbol *var;
  struct value *x;
  int tem;

  pc = (*pos);
  op = exp->elts[pc].opcode;

  switch (op)
    {
    case UNOP_IND:
      (*pos)++;
      x = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      /* We can't optimize out "&*" if there's a user-defined operator*.  */
      if (unop_user_defined_p (op, x))
	{
	  x = value_x_unop (x, op, noside);
	  goto default_case_after_eval;
	}

      return coerce_array (x);

    case UNOP_MEMVAL:
      (*pos) += 3;
      return value_cast (lookup_pointer_type (exp->elts[pc + 1].type),
			 evaluate_subexp (NULL_TYPE, exp, pos, noside));

    case UNOP_MEMVAL_TYPE:
      {
	struct type *type;

	(*pos) += 1;
	x = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
	type = value_type (x);
	return value_cast (lookup_pointer_type (type),
			   evaluate_subexp (NULL_TYPE, exp, pos, noside));
      }

    case OP_VAR_VALUE:
      var = exp->elts[pc + 2].symbol;

      /* C++: The "address" of a reference should yield the address
       * of the object pointed to.  Let value_addr() deal with it.  */
      if (TYPE_CODE (SYMBOL_TYPE (var)) == TYPE_CODE_REF)
	goto default_case;

      (*pos) += 4;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *type =
	    lookup_pointer_type (SYMBOL_TYPE (var));
	  enum address_class sym_class = SYMBOL_CLASS (var);

	  if (sym_class == LOC_CONST
	      || sym_class == LOC_CONST_BYTES
	      || sym_class == LOC_REGISTER)
	    error (_("Attempt to take address of register or constant."));

	  return
	    value_zero (type, not_lval);
	}
      else
	return address_of_variable (var, exp->elts[pc + 1].block);

    case OP_SCOPE:
      tem = longest_to_int (exp->elts[pc + 2].longconst);
      (*pos) += 5 + BYTES_TO_EXP_ELEM (tem + 1);
      x = value_aggregate_elt (exp->elts[pc + 1].type,
			       &exp->elts[pc + 3].string,
			       NULL, 1, noside);
      if (x == NULL)
	error (_("There is no field named %s"), &exp->elts[pc + 3].string);
      return x;

    default:
    default_case:
      x = evaluate_subexp (NULL_TYPE, exp, pos, noside);
    default_case_after_eval:
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *type = check_typedef (value_type (x));

	  if (TYPE_CODE (type) == TYPE_CODE_REF)
	    return value_zero (lookup_pointer_type (TYPE_TARGET_TYPE (type)),
			       not_lval);
	  else if (VALUE_LVAL (x) == lval_memory || value_must_coerce_to_target (x))
	    return value_zero (lookup_pointer_type (value_type (x)),
			       not_lval);
	  else
	    error (_("Attempt to take address of "
		     "value not located in memory."));
	}
      return value_addr (x);
    }
}

/* Evaluate like `evaluate_subexp' except coercing arrays to pointers.
   When used in contexts where arrays will be coerced anyway, this is
   equivalent to `evaluate_subexp' but much faster because it avoids
   actually fetching array contents (perhaps obsolete now that we have
   value_lazy()).

   Note that we currently only do the coercion for C expressions, where
   arrays are zero based and the coercion is correct.  For other languages,
   with nonzero based arrays, coercion loses.  Use CAST_IS_CONVERSION
   to decide if coercion is appropriate.  */

struct value *
evaluate_subexp_with_coercion (struct expression *exp,
			       int *pos, enum noside noside)
{
  enum exp_opcode op;
  int pc;
  struct value *val;
  struct symbol *var;
  struct type *type;

  pc = (*pos);
  op = exp->elts[pc].opcode;

  switch (op)
    {
    case OP_VAR_VALUE:
      var = exp->elts[pc + 2].symbol;
      type = check_typedef (SYMBOL_TYPE (var));
      if (TYPE_CODE (type) == TYPE_CODE_ARRAY
	  && !TYPE_VECTOR (type)
	  && CAST_IS_CONVERSION (exp->language_defn))
	{
	  (*pos) += 4;
	  val = address_of_variable (var, exp->elts[pc + 1].block);
	  return value_cast (lookup_pointer_type (TYPE_TARGET_TYPE (type)),
			     val);
	}
      /* FALLTHROUGH */

    default:
      return evaluate_subexp (NULL_TYPE, exp, pos, noside);
    }
}

/* Evaluate a subexpression of EXP, at index *POS,
   and return a value for the size of that subexpression.
   Advance *POS over the subexpression.  */

static struct value *
evaluate_subexp_for_sizeof (struct expression *exp, int *pos)
{
  /* FIXME: This should be size_t.  */
  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
  enum exp_opcode op;
  int pc;
  struct type *type;
  struct value *val;

  pc = (*pos);
  op = exp->elts[pc].opcode;

  switch (op)
    {
      /* This case is handled specially
         so that we avoid creating a value for the result type.
         If the result type is very big, it's desirable not to
         create a value unnecessarily.  */
    case UNOP_IND:
      (*pos)++;
      val = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = check_typedef (value_type (val));
      if (TYPE_CODE (type) != TYPE_CODE_PTR
	  && TYPE_CODE (type) != TYPE_CODE_REF
	  && TYPE_CODE (type) != TYPE_CODE_ARRAY)
	error (_("Attempt to take contents of a non-pointer value."));
      type = check_typedef (TYPE_TARGET_TYPE (type));
      return value_from_longest (size_type, (LONGEST) TYPE_LENGTH (type));

    case UNOP_MEMVAL:
      (*pos) += 3;
      type = check_typedef (exp->elts[pc + 1].type);
      return value_from_longest (size_type, (LONGEST) TYPE_LENGTH (type));

    case UNOP_MEMVAL_TYPE:
      (*pos) += 1;
      val = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = check_typedef (value_type (val));
      return value_from_longest (size_type, (LONGEST) TYPE_LENGTH (type));

    case OP_VAR_VALUE:
      (*pos) += 4;
      type = check_typedef (SYMBOL_TYPE (exp->elts[pc + 2].symbol));
      return
	value_from_longest (size_type, (LONGEST) TYPE_LENGTH (type));

    default:
      val = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      return value_from_longest (size_type,
				 (LONGEST) TYPE_LENGTH (value_type (val)));
    }
}

/* Parse a type expression in the string [P..P+LENGTH).  */

struct type *
parse_and_eval_type (char *p, int length)
{
  char *tmp = (char *) alloca (length + 4);
  struct expression *expr;

  tmp[0] = '(';
  memcpy (tmp + 1, p, length);
  tmp[length + 1] = ')';
  tmp[length + 2] = '0';
  tmp[length + 3] = '\0';
  expr = parse_expression (tmp);
  if (expr->elts[0].opcode != UNOP_CAST)
    error (_("Internal error in eval_type."));
  return expr->elts[1].type;
}

int
calc_f77_array_dims (struct type *array_type)
{
  int ndimen = 1;
  struct type *tmp_type;

  if ((TYPE_CODE (array_type) != TYPE_CODE_ARRAY))
    error (_("Can't get dimensions for a non-array type"));

  tmp_type = array_type;

  while ((tmp_type = TYPE_TARGET_TYPE (tmp_type)))
    {
      if (TYPE_CODE (tmp_type) == TYPE_CODE_ARRAY)
	++ndimen;
    }
  return ndimen;
}
