/* Perform arithmetic and other operations on values, for GDB.

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
#include "value.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "target.h"
#include "language.h"
#include <string.h>
#include "doublest.h"
#include "dfp.h"
#include <math.h>
#include "infcall.h"
#include "exceptions.h"

/* Define whether or not the C operator '/' truncates towards zero for
   differently signed operands (truncation direction is undefined in C).  */

#ifndef TRUNCATION_TOWARDS_ZERO
#define TRUNCATION_TOWARDS_ZERO ((-5 / 2) == -2)
#endif

void _initialize_valarith (void);


/* Given a pointer, return the size of its target.
   If the pointer type is void *, then return 1.
   If the target type is incomplete, then error out.
   This isn't a general purpose function, but just a 
   helper for value_ptradd.  */

static LONGEST
find_size_for_pointer_math (struct type *ptr_type)
{
  LONGEST sz = -1;
  struct type *ptr_target;

  gdb_assert (TYPE_CODE (ptr_type) == TYPE_CODE_PTR);
  ptr_target = check_typedef (TYPE_TARGET_TYPE (ptr_type));

  sz = TYPE_LENGTH (ptr_target);
  if (sz == 0)
    {
      if (TYPE_CODE (ptr_type) == TYPE_CODE_VOID)
	sz = 1;
      else
	{
	  const char *name;
	  
	  name = TYPE_NAME (ptr_target);
	  if (name == NULL)
	    name = TYPE_TAG_NAME (ptr_target);
	  if (name == NULL)
	    error (_("Cannot perform pointer math on incomplete types, "
		   "try casting to a known type, or void *."));
	  else
	    error (_("Cannot perform pointer math on incomplete type \"%s\", "
		   "try casting to a known type, or void *."), name);
	}
    }
  return sz;
}

/* Given a pointer ARG1 and an integral value ARG2, return the
   result of C-style pointer arithmetic ARG1 + ARG2.  */

struct value *
value_ptradd (struct value *arg1, LONGEST arg2)
{
  struct type *valptrtype;
  LONGEST sz;
  struct value *result;

  arg1 = coerce_array (arg1);
  valptrtype = check_typedef (value_type (arg1));
  sz = find_size_for_pointer_math (valptrtype);

  result = value_from_pointer (valptrtype,
			       value_as_address (arg1) + sz * arg2);
  if (VALUE_LVAL (result) != lval_internalvar)
    set_value_component_location (result, arg1);
  return result;
}

/* Given two compatible pointer values ARG1 and ARG2, return the
   result of C-style pointer arithmetic ARG1 - ARG2.  */

LONGEST
value_ptrdiff (struct value *arg1, struct value *arg2)
{
  struct type *type1, *type2;
  LONGEST sz;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);
  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));

  gdb_assert (TYPE_CODE (type1) == TYPE_CODE_PTR);
  gdb_assert (TYPE_CODE (type2) == TYPE_CODE_PTR);

  if (TYPE_LENGTH (check_typedef (TYPE_TARGET_TYPE (type1)))
      != TYPE_LENGTH (check_typedef (TYPE_TARGET_TYPE (type2))))
    error (_("First argument of `-' is a pointer and "
	     "second argument is neither\n"
	     "an integer nor a pointer of the same type."));

  sz = TYPE_LENGTH (check_typedef (TYPE_TARGET_TYPE (type1)));
  if (sz == 0) 
    {
      warning (_("Type size unknown, assuming 1. "
               "Try casting to a known type, or void *."));
      sz = 1;
    }

  return (value_as_long (arg1) - value_as_long (arg2)) / sz;
}

/* Return the value of ARRAY[IDX].

   ARRAY may be of type TYPE_CODE_ARRAY or TYPE_CODE_STRING.  If the
   current language supports C-style arrays, it may also be TYPE_CODE_PTR.

   See comments in value_coerce_array() for rationale for reason for
   doing lower bounds adjustment here rather than there.
   FIXME:  Perhaps we should validate that the index is valid and if
   verbosity is set, warn about invalid indices (but still use them).  */

struct value *
value_subscript (struct value *array, LONGEST index)
{
  int c_style = current_language->c_style_arrays;
  struct type *tarray;

  array = coerce_ref (array);
  tarray = check_typedef (value_type (array));

  if (TYPE_CODE (tarray) == TYPE_CODE_ARRAY
      || TYPE_CODE (tarray) == TYPE_CODE_STRING)
    {
      struct type *range_type = TYPE_INDEX_TYPE (tarray);
      LONGEST lowerbound, upperbound;

      get_discrete_bounds (range_type, &lowerbound, &upperbound);
      if (VALUE_LVAL (array) != lval_memory)
	return value_subscripted_rvalue (array, index, lowerbound);

      if (c_style == 0)
	{
	  if (index >= lowerbound && index <= upperbound)
	    return value_subscripted_rvalue (array, index, lowerbound);
	  /* Emit warning unless we have an array of unknown size.
	     An array of unknown size has lowerbound 0 and upperbound -1.  */
	  if (upperbound > -1)
	    warning (_("array or string index out of range"));
	  /* fall doing C stuff */
	  c_style = 1;
	}

      index -= lowerbound;
      array = value_coerce_array (array);
    }

  if (c_style)
    return value_ind (value_ptradd (array, index));
  else
    error (_("not an array or string"));
}

/* Return the value of EXPR[IDX], expr an aggregate rvalue
   (eg, a vector register).  This routine used to promote floats
   to doubles, but no longer does.  */

struct value *
value_subscripted_rvalue (struct value *array, LONGEST index, int lowerbound)
{
  struct type *array_type = check_typedef (value_type (array));
  struct type *elt_type = check_typedef (TYPE_TARGET_TYPE (array_type));
  unsigned int elt_size = TYPE_LENGTH (elt_type);
  unsigned int elt_offs = elt_size * longest_to_int (index - lowerbound);
  struct value *v;

  if (index < lowerbound || (!TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (array_type)
			     && elt_offs >= TYPE_LENGTH (array_type)))
    error (_("no such vector element"));

  if (VALUE_LVAL (array) == lval_memory && value_lazy (array))
    v = allocate_value_lazy (elt_type);
  else
    {
      v = allocate_value (elt_type);
      value_contents_copy (v, value_embedded_offset (v),
			   array, value_embedded_offset (array) + elt_offs,
			   elt_size);
    }

  set_value_component_location (v, array);
  VALUE_REGNUM (v) = VALUE_REGNUM (array);
  VALUE_FRAME_ID (v) = VALUE_FRAME_ID (array);
  set_value_offset (v, value_offset (array) + elt_offs);
  return v;
}


/* Check to see if either argument is a structure, or a reference to
   one.  This is called so we know whether to go ahead with the normal
   binop or look for a user defined function instead.

   For now, we do not overload the `=' operator.  */

int
binop_types_user_defined_p (enum exp_opcode op,
			    struct type *type1, struct type *type2)
{
  if (op == BINOP_ASSIGN || op == BINOP_CONCAT)
    return 0;

  type1 = check_typedef (type1);
  if (TYPE_CODE (type1) == TYPE_CODE_REF)
    type1 = check_typedef (TYPE_TARGET_TYPE (type1));

  type2 = check_typedef (type2);
  if (TYPE_CODE (type2) == TYPE_CODE_REF)
    type2 = check_typedef (TYPE_TARGET_TYPE (type2));

  return (TYPE_CODE (type1) == TYPE_CODE_STRUCT
	  || TYPE_CODE (type2) == TYPE_CODE_STRUCT);
}

/* Check to see if either argument is a structure, or a reference to
   one.  This is called so we know whether to go ahead with the normal
   binop or look for a user defined function instead.

   For now, we do not overload the `=' operator.  */

int
binop_user_defined_p (enum exp_opcode op,
		      struct value *arg1, struct value *arg2)
{
  return binop_types_user_defined_p (op, value_type (arg1), value_type (arg2));
}

/* Check to see if argument is a structure.  This is called so
   we know whether to go ahead with the normal unop or look for a 
   user defined function instead.

   For now, we do not overload the `&' operator.  */

int
unop_user_defined_p (enum exp_opcode op, struct value *arg1)
{
  struct type *type1;

  if (op == UNOP_ADDR)
    return 0;
  type1 = check_typedef (value_type (arg1));
  if (TYPE_CODE (type1) == TYPE_CODE_REF)
    type1 = check_typedef (TYPE_TARGET_TYPE (type1));
  return TYPE_CODE (type1) == TYPE_CODE_STRUCT;
}

/* Try to find an operator named OPERATOR which takes NARGS arguments
   specified in ARGS.  If the operator found is a static member operator
   *STATIC_MEMFUNP will be set to 1, and otherwise 0.
   The search if performed through find_overload_match which will handle
   member operators, non member operators, operators imported implicitly or
   explicitly, and perform correct overload resolution in all of the above
   situations or combinations thereof.  */

static struct value *
value_user_defined_cpp_op (struct value **args, int nargs, char *operator,
                           int *static_memfuncp)
{

  struct symbol *symp = NULL;
  struct value *valp = NULL;

  find_overload_match (args, nargs, operator, BOTH /* could be method */,
                       &args[0] /* objp */,
                       NULL /* pass NULL symbol since symbol is unknown */,
                       &valp, &symp, static_memfuncp, 0);

  if (valp)
    return valp;

  if (symp)
    {
      /* This is a non member function and does not
         expect a reference as its first argument
         rather the explicit structure.  */
      args[0] = value_ind (args[0]);
      return value_of_variable (symp, 0);
    }

  error (_("Could not find %s."), operator);
}

/* Lookup user defined operator NAME.  Return a value representing the
   function, otherwise return NULL.  */

static struct value *
value_user_defined_op (struct value **argp, struct value **args, char *name,
                       int *static_memfuncp, int nargs)
{
  struct value *result = NULL;

  if (current_language->la_language == language_cplus)
    result = value_user_defined_cpp_op (args, nargs, name, static_memfuncp);
  else
    result = value_struct_elt (argp, args, name, static_memfuncp,
                               "structure");

  return result;
}

/* We know either arg1 or arg2 is a structure, so try to find the right
   user defined function.  Create an argument vector that calls 
   arg1.operator @ (arg1,arg2) and return that value (where '@' is any
   binary operator which is legal for GNU C++).

   OP is the operatore, and if it is BINOP_ASSIGN_MODIFY, then OTHEROP
   is the opcode saying how to modify it.  Otherwise, OTHEROP is
   unused.  */

struct value *
value_x_binop (struct value *arg1, struct value *arg2, enum exp_opcode op,
	       enum exp_opcode otherop, enum noside noside)
{
  struct value **argvec;
  char *ptr;
  char tstr[13];
  int static_memfuncp;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (TYPE_CODE (check_typedef (value_type (arg1))) != TYPE_CODE_STRUCT)
    error (_("Can't do that binary op on that type"));	/* FIXME be explicit */

  argvec = (struct value **) alloca (sizeof (struct value *) * 4);
  argvec[1] = value_addr (arg1);
  argvec[2] = arg2;
  argvec[3] = 0;

  /* Make the right function name up.  */
  strcpy (tstr, "operator__");
  ptr = tstr + 8;
  switch (op)
    {
    case BINOP_ADD:
      strcpy (ptr, "+");
      break;
    case BINOP_SUB:
      strcpy (ptr, "-");
      break;
    case BINOP_MUL:
      strcpy (ptr, "*");
      break;
    case BINOP_DIV:
      strcpy (ptr, "/");
      break;
    case BINOP_REM:
      strcpy (ptr, "%");
      break;
    case BINOP_LSH:
      strcpy (ptr, "<<");
      break;
    case BINOP_RSH:
      strcpy (ptr, ">>");
      break;
    case BINOP_BITWISE_AND:
      strcpy (ptr, "&");
      break;
    case BINOP_BITWISE_IOR:
      strcpy (ptr, "|");
      break;
    case BINOP_BITWISE_XOR:
      strcpy (ptr, "^");
      break;
    case BINOP_LOGICAL_AND:
      strcpy (ptr, "&&");
      break;
    case BINOP_LOGICAL_OR:
      strcpy (ptr, "||");
      break;
    case BINOP_MIN:
      strcpy (ptr, "<?");
      break;
    case BINOP_MAX:
      strcpy (ptr, ">?");
      break;
    case BINOP_ASSIGN:
      strcpy (ptr, "=");
      break;
    case BINOP_ASSIGN_MODIFY:
      switch (otherop)
	{
	case BINOP_ADD:
	  strcpy (ptr, "+=");
	  break;
	case BINOP_SUB:
	  strcpy (ptr, "-=");
	  break;
	case BINOP_MUL:
	  strcpy (ptr, "*=");
	  break;
	case BINOP_DIV:
	  strcpy (ptr, "/=");
	  break;
	case BINOP_REM:
	  strcpy (ptr, "%=");
	  break;
	case BINOP_BITWISE_AND:
	  strcpy (ptr, "&=");
	  break;
	case BINOP_BITWISE_IOR:
	  strcpy (ptr, "|=");
	  break;
	case BINOP_BITWISE_XOR:
	  strcpy (ptr, "^=");
	  break;
	case BINOP_MOD:	/* invalid */
	default:
	  error (_("Invalid binary operation specified."));
	}
      break;
    case BINOP_SUBSCRIPT:
      strcpy (ptr, "[]");
      break;
    case BINOP_EQUAL:
      strcpy (ptr, "==");
      break;
    case BINOP_NOTEQUAL:
      strcpy (ptr, "!=");
      break;
    case BINOP_LESS:
      strcpy (ptr, "<");
      break;
    case BINOP_GTR:
      strcpy (ptr, ">");
      break;
    case BINOP_GEQ:
      strcpy (ptr, ">=");
      break;
    case BINOP_LEQ:
      strcpy (ptr, "<=");
      break;
    case BINOP_MOD:		/* invalid */
    default:
      error (_("Invalid binary operation specified."));
    }

  argvec[0] = value_user_defined_op (&arg1, argvec + 1, tstr,
                                     &static_memfuncp, 2);

  if (argvec[0])
    {
      if (static_memfuncp)
	{
	  argvec[1] = argvec[0];
	  argvec++;
	}
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *return_type;

	  return_type
	    = TYPE_TARGET_TYPE (check_typedef (value_type (argvec[0])));
	  return value_zero (return_type, VALUE_LVAL (arg1));
	}
      return call_function_by_hand (argvec[0], 2 - static_memfuncp,
				    argvec + 1);
    }
  throw_error (NOT_FOUND_ERROR,
               _("member function %s not found"), tstr);
#ifdef lint
  return call_function_by_hand (argvec[0], 2 - static_memfuncp, argvec + 1);
#endif
}

/* We know that arg1 is a structure, so try to find a unary user
   defined operator that matches the operator in question.
   Create an argument vector that calls arg1.operator @ (arg1)
   and return that value (where '@' is (almost) any unary operator which
   is legal for GNU C++).  */

struct value *
value_x_unop (struct value *arg1, enum exp_opcode op, enum noside noside)
{
  struct gdbarch *gdbarch = get_type_arch (value_type (arg1));
  struct value **argvec;
  char *ptr;
  char tstr[13], mangle_tstr[13];
  int static_memfuncp, nargs;

  arg1 = coerce_ref (arg1);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (TYPE_CODE (check_typedef (value_type (arg1))) != TYPE_CODE_STRUCT)
    error (_("Can't do that unary op on that type"));	/* FIXME be explicit */

  argvec = (struct value **) alloca (sizeof (struct value *) * 4);
  argvec[1] = value_addr (arg1);
  argvec[2] = 0;

  nargs = 1;

  /* Make the right function name up.  */
  strcpy (tstr, "operator__");
  ptr = tstr + 8;
  strcpy (mangle_tstr, "__");
  switch (op)
    {
    case UNOP_PREINCREMENT:
      strcpy (ptr, "++");
      break;
    case UNOP_PREDECREMENT:
      strcpy (ptr, "--");
      break;
    case UNOP_POSTINCREMENT:
      strcpy (ptr, "++");
      argvec[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);
      argvec[3] = 0;
      nargs ++;
      break;
    case UNOP_POSTDECREMENT:
      strcpy (ptr, "--");
      argvec[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);
      argvec[3] = 0;
      nargs ++;
      break;
    case UNOP_LOGICAL_NOT:
      strcpy (ptr, "!");
      break;
    case UNOP_COMPLEMENT:
      strcpy (ptr, "~");
      break;
    case UNOP_NEG:
      strcpy (ptr, "-");
      break;
    case UNOP_PLUS:
      strcpy (ptr, "+");
      break;
    case UNOP_IND:
      strcpy (ptr, "*");
      break;
    case STRUCTOP_PTR:
      strcpy (ptr, "->");
      break;
    default:
      error (_("Invalid unary operation specified."));
    }

  argvec[0] = value_user_defined_op (&arg1, argvec + 1, tstr,
                                     &static_memfuncp, nargs);

  if (argvec[0])
    {
      if (static_memfuncp)
	{
	  argvec[1] = argvec[0];
	  nargs --;
	  argvec++;
	}
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *return_type;

	  return_type
	    = TYPE_TARGET_TYPE (check_typedef (value_type (argvec[0])));
	  return value_zero (return_type, VALUE_LVAL (arg1));
	}
      return call_function_by_hand (argvec[0], nargs, argvec + 1);
    }
  throw_error (NOT_FOUND_ERROR,
               _("member function %s not found"), tstr);

  return 0;			/* For lint -- never reached */
}


/* Concatenate two values with the following conditions:

   (1)  Both values must be either bitstring values or character string
   values and the resulting value consists of the concatenation of
   ARG1 followed by ARG2.

   or

   One value must be an integer value and the other value must be
   either a bitstring value or character string value, which is
   to be repeated by the number of times specified by the integer
   value.


   (2)  Boolean values are also allowed and are treated as bit string
   values of length 1.

   (3)  Character values are also allowed and are treated as character
   string values of length 1.  */

struct value *
value_concat (struct value *arg1, struct value *arg2)
{
  struct value *inval1;
  struct value *inval2;
  struct value *outval = NULL;
  int inval1len, inval2len;
  int count, idx;
  char *ptr;
  char inchar;
  struct type *type1 = check_typedef (value_type (arg1));
  struct type *type2 = check_typedef (value_type (arg2));
  struct type *char_type;

  /* First figure out if we are dealing with two values to be concatenated
     or a repeat count and a value to be repeated.  INVAL1 is set to the
     first of two concatenated values, or the repeat count.  INVAL2 is set
     to the second of the two concatenated values or the value to be 
     repeated.  */

  if (TYPE_CODE (type2) == TYPE_CODE_INT)
    {
      struct type *tmp = type1;

      type1 = tmp;
      tmp = type2;
      inval1 = arg2;
      inval2 = arg1;
    }
  else
    {
      inval1 = arg1;
      inval2 = arg2;
    }

  /* Now process the input values.  */

  if (TYPE_CODE (type1) == TYPE_CODE_INT)
    {
      /* We have a repeat count.  Validate the second value and then
         construct a value repeated that many times.  */
      if (TYPE_CODE (type2) == TYPE_CODE_STRING
	  || TYPE_CODE (type2) == TYPE_CODE_CHAR)
	{
	  struct cleanup *back_to;

	  count = longest_to_int (value_as_long (inval1));
	  inval2len = TYPE_LENGTH (type2);
	  ptr = (char *) xmalloc (count * inval2len);
	  back_to = make_cleanup (xfree, ptr);
	  if (TYPE_CODE (type2) == TYPE_CODE_CHAR)
	    {
	      char_type = type2;

	      inchar = (char) unpack_long (type2,
					   value_contents (inval2));
	      for (idx = 0; idx < count; idx++)
		{
		  *(ptr + idx) = inchar;
		}
	    }
	  else
	    {
	      char_type = TYPE_TARGET_TYPE (type2);

	      for (idx = 0; idx < count; idx++)
		{
		  memcpy (ptr + (idx * inval2len), value_contents (inval2),
			  inval2len);
		}
	    }
	  outval = value_string (ptr, count * inval2len, char_type);
	  do_cleanups (back_to);
	}
      else if (TYPE_CODE (type2) == TYPE_CODE_BOOL)
	{
	  error (_("unimplemented support for boolean repeats"));
	}
      else
	{
	  error (_("can't repeat values of that type"));
	}
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_STRING
	   || TYPE_CODE (type1) == TYPE_CODE_CHAR)
    {
      struct cleanup *back_to;

      /* We have two character strings to concatenate.  */
      if (TYPE_CODE (type2) != TYPE_CODE_STRING
	  && TYPE_CODE (type2) != TYPE_CODE_CHAR)
	{
	  error (_("Strings can only be concatenated with other strings."));
	}
      inval1len = TYPE_LENGTH (type1);
      inval2len = TYPE_LENGTH (type2);
      ptr = (char *) xmalloc (inval1len + inval2len);
      back_to = make_cleanup (xfree, ptr);
      if (TYPE_CODE (type1) == TYPE_CODE_CHAR)
	{
	  char_type = type1;

	  *ptr = (char) unpack_long (type1, value_contents (inval1));
	}
      else
	{
	  char_type = TYPE_TARGET_TYPE (type1);

	  memcpy (ptr, value_contents (inval1), inval1len);
	}
      if (TYPE_CODE (type2) == TYPE_CODE_CHAR)
	{
	  *(ptr + inval1len) =
	    (char) unpack_long (type2, value_contents (inval2));
	}
      else
	{
	  memcpy (ptr + inval1len, value_contents (inval2), inval2len);
	}
      outval = value_string (ptr, inval1len + inval2len, char_type);
      do_cleanups (back_to);
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_BOOL)
    {
      /* We have two bitstrings to concatenate.  */
      if (TYPE_CODE (type2) != TYPE_CODE_BOOL)
	{
	  error (_("Booleans can only be concatenated "
		   "with other bitstrings or booleans."));
	}
      error (_("unimplemented support for boolean concatenation."));
    }
  else
    {
      /* We don't know how to concatenate these operands.  */
      error (_("illegal operands for concatenation."));
    }
  return (outval);
}

/* Integer exponentiation: V1**V2, where both arguments are
   integers.  Requires V1 != 0 if V2 < 0.  Returns 1 for 0 ** 0.  */

static LONGEST
integer_pow (LONGEST v1, LONGEST v2)
{
  if (v2 < 0)
    {
      if (v1 == 0)
	error (_("Attempt to raise 0 to negative power."));
      else
	return 0;
    }
  else 
    {
      /* The Russian Peasant's Algorithm.  */
      LONGEST v;
      
      v = 1;
      for (;;)
	{
	  if (v2 & 1L) 
	    v *= v1;
	  v2 >>= 1;
	  if (v2 == 0)
	    return v;
	  v1 *= v1;
	}
    }
}

/* Integer exponentiation: V1**V2, where both arguments are
   integers.  Requires V1 != 0 if V2 < 0.  Returns 1 for 0 ** 0.  */

static ULONGEST
uinteger_pow (ULONGEST v1, LONGEST v2)
{
  if (v2 < 0)
    {
      if (v1 == 0)
	error (_("Attempt to raise 0 to negative power."));
      else
	return 0;
    }
  else 
    {
      /* The Russian Peasant's Algorithm.  */
      ULONGEST v;
      
      v = 1;
      for (;;)
	{
	  if (v2 & 1L) 
	    v *= v1;
	  v2 >>= 1;
	  if (v2 == 0)
	    return v;
	  v1 *= v1;
	}
    }
}

/* Obtain decimal value of arguments for binary operation, converting from
   other types if one of them is not decimal floating point.  */
static void
value_args_as_decimal (struct value *arg1, struct value *arg2,
		       gdb_byte *x, int *len_x, enum bfd_endian *byte_order_x,
		       gdb_byte *y, int *len_y, enum bfd_endian *byte_order_y)
{
  struct type *type1, *type2;

  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));

  /* At least one of the arguments must be of decimal float type.  */
  gdb_assert (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT
	      || TYPE_CODE (type2) == TYPE_CODE_DECFLOAT);

  if (TYPE_CODE (type1) == TYPE_CODE_FLT
      || TYPE_CODE (type2) == TYPE_CODE_FLT)
    /* The DFP extension to the C language does not allow mixing of
     * decimal float types with other float types in expressions
     * (see WDTR 24732, page 12).  */
    error (_("Mixing decimal floating types with "
	     "other floating types is not allowed."));

  /* Obtain decimal value of arg1, converting from other types
     if necessary.  */

  if (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT)
    {
      *byte_order_x = gdbarch_byte_order (get_type_arch (type1));
      *len_x = TYPE_LENGTH (type1);
      memcpy (x, value_contents (arg1), *len_x);
    }
  else if (is_integral_type (type1))
    {
      *byte_order_x = gdbarch_byte_order (get_type_arch (type2));
      *len_x = TYPE_LENGTH (type2);
      decimal_from_integral (arg1, x, *len_x, *byte_order_x);
    }
  else
    error (_("Don't know how to convert from %s to %s."), TYPE_NAME (type1),
	     TYPE_NAME (type2));

  /* Obtain decimal value of arg2, converting from other types
     if necessary.  */

  if (TYPE_CODE (type2) == TYPE_CODE_DECFLOAT)
    {
      *byte_order_y = gdbarch_byte_order (get_type_arch (type2));
      *len_y = TYPE_LENGTH (type2);
      memcpy (y, value_contents (arg2), *len_y);
    }
  else if (is_integral_type (type2))
    {
      *byte_order_y = gdbarch_byte_order (get_type_arch (type1));
      *len_y = TYPE_LENGTH (type1);
      decimal_from_integral (arg2, y, *len_y, *byte_order_y);
    }
  else
    error (_("Don't know how to convert from %s to %s."), TYPE_NAME (type1),
	     TYPE_NAME (type2));
}

/* Perform a binary operation on two operands which have reasonable
   representations as integers or floats.  This includes booleans,
   characters, integers, or floats.
   Does not support addition and subtraction on pointers;
   use value_ptradd, value_ptrsub or value_ptrdiff for those operations.  */

static struct value *
scalar_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct value *val;
  struct type *type1, *type2, *result_type;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);

  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));

  if ((TYPE_CODE (type1) != TYPE_CODE_FLT
       && TYPE_CODE (type1) != TYPE_CODE_DECFLOAT
       && !is_integral_type (type1))
      || (TYPE_CODE (type2) != TYPE_CODE_FLT
	  && TYPE_CODE (type2) != TYPE_CODE_DECFLOAT
	  && !is_integral_type (type2)))
    error (_("Argument to arithmetic operation not a number or boolean."));

  if (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT
      || TYPE_CODE (type2) == TYPE_CODE_DECFLOAT)
    {
      int len_v1, len_v2, len_v;
      enum bfd_endian byte_order_v1, byte_order_v2, byte_order_v;
      gdb_byte v1[16], v2[16];
      gdb_byte v[16];

      /* If only one type is decimal float, use its type.
	 Otherwise use the bigger type.  */
      if (TYPE_CODE (type1) != TYPE_CODE_DECFLOAT)
	result_type = type2;
      else if (TYPE_CODE (type2) != TYPE_CODE_DECFLOAT)
	result_type = type1;
      else if (TYPE_LENGTH (type2) > TYPE_LENGTH (type1))
	result_type = type2;
      else
	result_type = type1;

      len_v = TYPE_LENGTH (result_type);
      byte_order_v = gdbarch_byte_order (get_type_arch (result_type));

      value_args_as_decimal (arg1, arg2, v1, &len_v1, &byte_order_v1,
					 v2, &len_v2, &byte_order_v2);

      switch (op)
	{
	case BINOP_ADD:
	case BINOP_SUB:
	case BINOP_MUL:
	case BINOP_DIV:
	case BINOP_EXP:
	  decimal_binop (op, v1, len_v1, byte_order_v1,
			     v2, len_v2, byte_order_v2,
			     v, len_v, byte_order_v);
	  break;

	default:
	  error (_("Operation not valid for decimal floating point number."));
	}

      val = value_from_decfloat (result_type, v);
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_FLT
	   || TYPE_CODE (type2) == TYPE_CODE_FLT)
    {
      /* FIXME-if-picky-about-floating-accuracy: Should be doing this
         in target format.  real.c in GCC probably has the necessary
         code.  */
      DOUBLEST v1, v2, v = 0;

      v1 = value_as_double (arg1);
      v2 = value_as_double (arg2);

      switch (op)
	{
	case BINOP_ADD:
	  v = v1 + v2;
	  break;

	case BINOP_SUB:
	  v = v1 - v2;
	  break;

	case BINOP_MUL:
	  v = v1 * v2;
	  break;

	case BINOP_DIV:
	  v = v1 / v2;
	  break;

	case BINOP_EXP:
	  errno = 0;
	  v = pow (v1, v2);
	  if (errno)
	    error (_("Cannot perform exponentiation: %s"),
		   safe_strerror (errno));
	  break;

	case BINOP_MIN:
	  v = v1 < v2 ? v1 : v2;
	  break;
	      
	case BINOP_MAX:
	  v = v1 > v2 ? v1 : v2;
	  break;

	default:
	  error (_("Integer-only operation on floating point number."));
	}

      /* If only one type is float, use its type.
	 Otherwise use the bigger type.  */
      if (TYPE_CODE (type1) != TYPE_CODE_FLT)
	result_type = type2;
      else if (TYPE_CODE (type2) != TYPE_CODE_FLT)
	result_type = type1;
      else if (TYPE_LENGTH (type2) > TYPE_LENGTH (type1))
	result_type = type2;
      else
	result_type = type1;

      val = allocate_value (result_type);
      store_typed_floating (value_contents_raw (val), value_type (val), v);
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_BOOL
	   || TYPE_CODE (type2) == TYPE_CODE_BOOL)
    {
      LONGEST v1, v2, v = 0;

      v1 = value_as_long (arg1);
      v2 = value_as_long (arg2);

      switch (op)
	{
	case BINOP_BITWISE_AND:
	  v = v1 & v2;
	  break;

	case BINOP_BITWISE_IOR:
	  v = v1 | v2;
	  break;

	case BINOP_BITWISE_XOR:
	  v = v1 ^ v2;
          break;
              
        case BINOP_EQUAL:
          v = v1 == v2;
          break;
          
        case BINOP_NOTEQUAL:
          v = v1 != v2;
	  break;

	default:
	  error (_("Invalid operation on booleans."));
	}

      result_type = type1;

      val = allocate_value (result_type);
      store_signed_integer (value_contents_raw (val),
			    TYPE_LENGTH (result_type),
			    gdbarch_byte_order (get_type_arch (result_type)),
			    v);
    }
  else
    /* Integral operations here.  */
    {
      /* Determine type length of the result, and if the operation should
	 be done unsigned.  For exponentiation and shift operators,
	 use the length and type of the left operand.  Otherwise,
	 use the signedness of the operand with the greater length.
	 If both operands are of equal length, use unsigned operation
	 if one of the operands is unsigned.  */
      if (op == BINOP_RSH || op == BINOP_LSH || op == BINOP_EXP)
	result_type = type1;
      else if (TYPE_LENGTH (type1) > TYPE_LENGTH (type2))
	result_type = type1;
      else if (TYPE_LENGTH (type2) > TYPE_LENGTH (type1))
	result_type = type2;
      else if (TYPE_UNSIGNED (type1))
	result_type = type1;
      else if (TYPE_UNSIGNED (type2))
	result_type = type2;
      else
	result_type = type1;

      if (TYPE_UNSIGNED (result_type))
	{
	  LONGEST v2_signed = value_as_long (arg2);
	  ULONGEST v1, v2, v = 0;

	  v1 = (ULONGEST) value_as_long (arg1);
	  v2 = (ULONGEST) v2_signed;

	  switch (op)
	    {
	    case BINOP_ADD:
	      v = v1 + v2;
	      break;

	    case BINOP_SUB:
	      v = v1 - v2;
	      break;

	    case BINOP_MUL:
	      v = v1 * v2;
	      break;

	    case BINOP_DIV:
	    case BINOP_INTDIV:
	      if (v2 != 0)
		v = v1 / v2;
	      else
		error (_("Division by zero"));
	      break;

	    case BINOP_EXP:
              v = uinteger_pow (v1, v2_signed);
	      break;

	    case BINOP_REM:
	      if (v2 != 0)
		v = v1 % v2;
	      else
		error (_("Division by zero"));
	      break;

	    case BINOP_MOD:
	      /* Knuth 1.2.4, integer only.  Note that unlike the C '%' op,
	         v1 mod 0 has a defined value, v1.  */
	      if (v2 == 0)
		{
		  v = v1;
		}
	      else
		{
		  v = v1 / v2;
		  /* Note floor(v1/v2) == v1/v2 for unsigned.  */
		  v = v1 - (v2 * v);
		}
	      break;

	    case BINOP_LSH:
	      v = v1 << v2;
	      break;

	    case BINOP_RSH:
	      v = v1 >> v2;
	      break;

	    case BINOP_BITWISE_AND:
	      v = v1 & v2;
	      break;

	    case BINOP_BITWISE_IOR:
	      v = v1 | v2;
	      break;

	    case BINOP_BITWISE_XOR:
	      v = v1 ^ v2;
	      break;

	    case BINOP_LOGICAL_AND:
	      v = v1 && v2;
	      break;

	    case BINOP_LOGICAL_OR:
	      v = v1 || v2;
	      break;

	    case BINOP_MIN:
	      v = v1 < v2 ? v1 : v2;
	      break;

	    case BINOP_MAX:
	      v = v1 > v2 ? v1 : v2;
	      break;

	    case BINOP_EQUAL:
	      v = v1 == v2;
	      break;

            case BINOP_NOTEQUAL:
              v = v1 != v2;
              break;

	    case BINOP_LESS:
	      v = v1 < v2;
	      break;

	    case BINOP_GTR:
	      v = v1 > v2;
	      break;

	    case BINOP_LEQ:
	      v = v1 <= v2;
	      break;

	    case BINOP_GEQ:
	      v = v1 >= v2;
	      break;

	    default:
	      error (_("Invalid binary operation on numbers."));
	    }

	  val = allocate_value (result_type);
	  store_unsigned_integer (value_contents_raw (val),
				  TYPE_LENGTH (value_type (val)),
				  gdbarch_byte_order
				    (get_type_arch (result_type)),
				  v);
	}
      else
	{
	  LONGEST v1, v2, v = 0;

	  v1 = value_as_long (arg1);
	  v2 = value_as_long (arg2);

	  switch (op)
	    {
	    case BINOP_ADD:
	      v = v1 + v2;
	      break;

	    case BINOP_SUB:
	      v = v1 - v2;
	      break;

	    case BINOP_MUL:
	      v = v1 * v2;
	      break;

	    case BINOP_DIV:
	    case BINOP_INTDIV:
	      if (v2 != 0)
		v = v1 / v2;
	      else
		error (_("Division by zero"));
              break;

	    case BINOP_EXP:
              v = integer_pow (v1, v2);
	      break;

	    case BINOP_REM:
	      if (v2 != 0)
		v = v1 % v2;
	      else
		error (_("Division by zero"));
	      break;

	    case BINOP_MOD:
	      /* Knuth 1.2.4, integer only.  Note that unlike the C '%' op,
	         X mod 0 has a defined value, X.  */
	      if (v2 == 0)
		{
		  v = v1;
		}
	      else
		{
		  v = v1 / v2;
		  /* Compute floor.  */
		  if (TRUNCATION_TOWARDS_ZERO && (v < 0) && ((v1 % v2) != 0))
		    {
		      v--;
		    }
		  v = v1 - (v2 * v);
		}
	      break;

	    case BINOP_LSH:
	      v = v1 << v2;
	      break;

	    case BINOP_RSH:
	      v = v1 >> v2;
	      break;

	    case BINOP_BITWISE_AND:
	      v = v1 & v2;
	      break;

	    case BINOP_BITWISE_IOR:
	      v = v1 | v2;
	      break;

	    case BINOP_BITWISE_XOR:
	      v = v1 ^ v2;
	      break;

	    case BINOP_LOGICAL_AND:
	      v = v1 && v2;
	      break;

	    case BINOP_LOGICAL_OR:
	      v = v1 || v2;
	      break;

	    case BINOP_MIN:
	      v = v1 < v2 ? v1 : v2;
	      break;

	    case BINOP_MAX:
	      v = v1 > v2 ? v1 : v2;
	      break;

	    case BINOP_EQUAL:
	      v = v1 == v2;
	      break;

            case BINOP_NOTEQUAL:
              v = v1 != v2;
              break;

	    case BINOP_LESS:
	      v = v1 < v2;
	      break;

	    case BINOP_GTR:
	      v = v1 > v2;
	      break;

	    case BINOP_LEQ:
	      v = v1 <= v2;
	      break;

	    case BINOP_GEQ:
	      v = v1 >= v2;
	      break;

	    default:
	      error (_("Invalid binary operation on numbers."));
	    }

	  val = allocate_value (result_type);
	  store_signed_integer (value_contents_raw (val),
				TYPE_LENGTH (value_type (val)),
				gdbarch_byte_order
				  (get_type_arch (result_type)),
				v);
	}
    }

  return val;
}

/* Widen a scalar value SCALAR_VALUE to vector type VECTOR_TYPE by
   replicating SCALAR_VALUE for each element of the vector.  Only scalar
   types that can be cast to the type of one element of the vector are
   acceptable.  The newly created vector value is returned upon success,
   otherwise an error is thrown.  */

struct value *
value_vector_widen (struct value *scalar_value, struct type *vector_type)
{
  /* Widen the scalar to a vector.  */
  struct type *eltype, *scalar_type;
  struct value *val, *elval;
  LONGEST low_bound, high_bound;
  int i;

  CHECK_TYPEDEF (vector_type);

  gdb_assert (TYPE_CODE (vector_type) == TYPE_CODE_ARRAY
	      && TYPE_VECTOR (vector_type));

  if (!get_array_bounds (vector_type, &low_bound, &high_bound))
    error (_("Could not determine the vector bounds"));

  eltype = check_typedef (TYPE_TARGET_TYPE (vector_type));
  elval = value_cast (eltype, scalar_value);

  scalar_type = check_typedef (value_type (scalar_value));

  /* If we reduced the length of the scalar then check we didn't loose any
     important bits.  */
  if (TYPE_LENGTH (eltype) < TYPE_LENGTH (scalar_type)
      && !value_equal (elval, scalar_value))
    error (_("conversion of scalar to vector involves truncation"));

  val = allocate_value (vector_type);
  for (i = 0; i < high_bound - low_bound + 1; i++)
    /* Duplicate the contents of elval into the destination vector.  */
    memcpy (value_contents_writeable (val) + (i * TYPE_LENGTH (eltype)),
	    value_contents_all (elval), TYPE_LENGTH (eltype));

  return val;
}

/* Performs a binary operation on two vector operands by calling scalar_binop
   for each pair of vector components.  */

static struct value *
vector_binop (struct value *val1, struct value *val2, enum exp_opcode op)
{
  struct value *val, *tmp, *mark;
  struct type *type1, *type2, *eltype1, *eltype2;
  int t1_is_vec, t2_is_vec, elsize, i;
  LONGEST low_bound1, high_bound1, low_bound2, high_bound2;

  type1 = check_typedef (value_type (val1));
  type2 = check_typedef (value_type (val2));

  t1_is_vec = (TYPE_CODE (type1) == TYPE_CODE_ARRAY
	       && TYPE_VECTOR (type1)) ? 1 : 0;
  t2_is_vec = (TYPE_CODE (type2) == TYPE_CODE_ARRAY
	       && TYPE_VECTOR (type2)) ? 1 : 0;

  if (!t1_is_vec || !t2_is_vec)
    error (_("Vector operations are only supported among vectors"));

  if (!get_array_bounds (type1, &low_bound1, &high_bound1)
      || !get_array_bounds (type2, &low_bound2, &high_bound2))
    error (_("Could not determine the vector bounds"));

  eltype1 = check_typedef (TYPE_TARGET_TYPE (type1));
  eltype2 = check_typedef (TYPE_TARGET_TYPE (type2));
  elsize = TYPE_LENGTH (eltype1);

  if (TYPE_CODE (eltype1) != TYPE_CODE (eltype2)
      || elsize != TYPE_LENGTH (eltype2)
      || TYPE_UNSIGNED (eltype1) != TYPE_UNSIGNED (eltype2)
      || low_bound1 != low_bound2 || high_bound1 != high_bound2)
    error (_("Cannot perform operation on vectors with different types"));

  val = allocate_value (type1);
  mark = value_mark ();
  for (i = 0; i < high_bound1 - low_bound1 + 1; i++)
    {
      tmp = value_binop (value_subscript (val1, i),
			 value_subscript (val2, i), op);
      memcpy (value_contents_writeable (val) + i * elsize,
	      value_contents_all (tmp),
	      elsize);
     }
  value_free_to_mark (mark);

  return val;
}

/* Perform a binary operation on two operands.  */

struct value *
value_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct value *val;
  struct type *type1 = check_typedef (value_type (arg1));
  struct type *type2 = check_typedef (value_type (arg2));
  int t1_is_vec = (TYPE_CODE (type1) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type1));
  int t2_is_vec = (TYPE_CODE (type2) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type2));

  if (!t1_is_vec && !t2_is_vec)
    val = scalar_binop (arg1, arg2, op);
  else if (t1_is_vec && t2_is_vec)
    val = vector_binop (arg1, arg2, op);
  else
    {
      /* Widen the scalar operand to a vector.  */
      struct value **v = t1_is_vec ? &arg2 : &arg1;
      struct type *t = t1_is_vec ? type2 : type1;
      
      if (TYPE_CODE (t) != TYPE_CODE_FLT
	  && TYPE_CODE (t) != TYPE_CODE_DECFLOAT
	  && !is_integral_type (t))
	error (_("Argument to operation not a number or boolean."));

      /* Replicate the scalar value to make a vector value.  */
      *v = value_vector_widen (*v, t1_is_vec ? type1 : type2);

      val = vector_binop (arg1, arg2, op);
    }

  return val;
}

/* Simulate the C operator ! -- return 1 if ARG1 contains zero.  */

int
value_logical_not (struct value *arg1)
{
  int len;
  const gdb_byte *p;
  struct type *type1;

  arg1 = coerce_array (arg1);
  type1 = check_typedef (value_type (arg1));

  if (TYPE_CODE (type1) == TYPE_CODE_FLT)
    return 0 == value_as_double (arg1);
  else if (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT)
    return decimal_is_zero (value_contents (arg1), TYPE_LENGTH (type1),
			    gdbarch_byte_order (get_type_arch (type1)));

  len = TYPE_LENGTH (type1);
  p = value_contents (arg1);

  while (--len >= 0)
    {
      if (*p++)
	break;
    }

  return len < 0;
}

/* Perform a comparison on two string values (whose content are not
   necessarily null terminated) based on their length.  */

static int
value_strcmp (struct value *arg1, struct value *arg2)
{
  int len1 = TYPE_LENGTH (value_type (arg1));
  int len2 = TYPE_LENGTH (value_type (arg2));
  const gdb_byte *s1 = value_contents (arg1);
  const gdb_byte *s2 = value_contents (arg2);
  int i, len = len1 < len2 ? len1 : len2;

  for (i = 0; i < len; i++)
    {
      if (s1[i] < s2[i])
        return -1;
      else if (s1[i] > s2[i])
        return 1;
      else
        continue;
    }

  if (len1 < len2)
    return -1;
  else if (len1 > len2)
    return 1;
  else
    return 0;
}

/* Simulate the C operator == by returning a 1
   iff ARG1 and ARG2 have equal contents.  */

int
value_equal (struct value *arg1, struct value *arg2)
{
  int len;
  const gdb_byte *p1;
  const gdb_byte *p2;
  struct type *type1, *type2;
  enum type_code code1;
  enum type_code code2;
  int is_int1, is_int2;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);

  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));
  code1 = TYPE_CODE (type1);
  code2 = TYPE_CODE (type2);
  is_int1 = is_integral_type (type1);
  is_int2 = is_integral_type (type2);

  if (is_int1 && is_int2)
    return longest_to_int (value_as_long (value_binop (arg1, arg2,
						       BINOP_EQUAL)));
  else if ((code1 == TYPE_CODE_FLT || is_int1)
	   && (code2 == TYPE_CODE_FLT || is_int2))
    {
      /* NOTE: kettenis/20050816: Avoid compiler bug on systems where
	 `long double' values are returned in static storage (m68k).  */
      DOUBLEST d = value_as_double (arg1);

      return d == value_as_double (arg2);
    }
  else if ((code1 == TYPE_CODE_DECFLOAT || is_int1)
	   && (code2 == TYPE_CODE_DECFLOAT || is_int2))
    {
      gdb_byte v1[16], v2[16];
      int len_v1, len_v2;
      enum bfd_endian byte_order_v1, byte_order_v2;

      value_args_as_decimal (arg1, arg2, v1, &len_v1, &byte_order_v1,
					 v2, &len_v2, &byte_order_v2);

      return decimal_compare (v1, len_v1, byte_order_v1,
			      v2, len_v2, byte_order_v2) == 0;
    }

  /* FIXME: Need to promote to either CORE_ADDR or LONGEST, whichever
     is bigger.  */
  else if (code1 == TYPE_CODE_PTR && is_int2)
    return value_as_address (arg1) == (CORE_ADDR) value_as_long (arg2);
  else if (code2 == TYPE_CODE_PTR && is_int1)
    return (CORE_ADDR) value_as_long (arg1) == value_as_address (arg2);

  else if (code1 == code2
	   && ((len = (int) TYPE_LENGTH (type1))
	       == (int) TYPE_LENGTH (type2)))
    {
      p1 = value_contents (arg1);
      p2 = value_contents (arg2);
      while (--len >= 0)
	{
	  if (*p1++ != *p2++)
	    break;
	}
      return len < 0;
    }
  else if (code1 == TYPE_CODE_STRING && code2 == TYPE_CODE_STRING)
    {
      return value_strcmp (arg1, arg2) == 0;
    }
  else
    {
      error (_("Invalid type combination in equality test."));
      return 0;			/* For lint -- never reached.  */
    }
}

/* Compare values based on their raw contents.  Useful for arrays since
   value_equal coerces them to pointers, thus comparing just the address
   of the array instead of its contents.  */

int
value_equal_contents (struct value *arg1, struct value *arg2)
{
  struct type *type1, *type2;

  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));

  return (TYPE_CODE (type1) == TYPE_CODE (type2)
	  && TYPE_LENGTH (type1) == TYPE_LENGTH (type2)
	  && memcmp (value_contents (arg1), value_contents (arg2),
		     TYPE_LENGTH (type1)) == 0);
}

/* Simulate the C operator < by returning 1
   iff ARG1's contents are less than ARG2's.  */

int
value_less (struct value *arg1, struct value *arg2)
{
  enum type_code code1;
  enum type_code code2;
  struct type *type1, *type2;
  int is_int1, is_int2;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);

  type1 = check_typedef (value_type (arg1));
  type2 = check_typedef (value_type (arg2));
  code1 = TYPE_CODE (type1);
  code2 = TYPE_CODE (type2);
  is_int1 = is_integral_type (type1);
  is_int2 = is_integral_type (type2);

  if (is_int1 && is_int2)
    return longest_to_int (value_as_long (value_binop (arg1, arg2,
						       BINOP_LESS)));
  else if ((code1 == TYPE_CODE_FLT || is_int1)
	   && (code2 == TYPE_CODE_FLT || is_int2))
    {
      /* NOTE: kettenis/20050816: Avoid compiler bug on systems where
	 `long double' values are returned in static storage (m68k).  */
      DOUBLEST d = value_as_double (arg1);

      return d < value_as_double (arg2);
    }
  else if ((code1 == TYPE_CODE_DECFLOAT || is_int1)
	   && (code2 == TYPE_CODE_DECFLOAT || is_int2))
    {
      gdb_byte v1[16], v2[16];
      int len_v1, len_v2;
      enum bfd_endian byte_order_v1, byte_order_v2;

      value_args_as_decimal (arg1, arg2, v1, &len_v1, &byte_order_v1,
					 v2, &len_v2, &byte_order_v2);

      return decimal_compare (v1, len_v1, byte_order_v1,
			      v2, len_v2, byte_order_v2) == -1;
    }
  else if (code1 == TYPE_CODE_PTR && code2 == TYPE_CODE_PTR)
    return value_as_address (arg1) < value_as_address (arg2);

  /* FIXME: Need to promote to either CORE_ADDR or LONGEST, whichever
     is bigger.  */
  else if (code1 == TYPE_CODE_PTR && is_int2)
    return value_as_address (arg1) < (CORE_ADDR) value_as_long (arg2);
  else if (code2 == TYPE_CODE_PTR && is_int1)
    return (CORE_ADDR) value_as_long (arg1) < value_as_address (arg2);
  else if (code1 == TYPE_CODE_STRING && code2 == TYPE_CODE_STRING)
    return value_strcmp (arg1, arg2) < 0;
  else
    {
      error (_("Invalid type combination in ordering comparison."));
      return 0;
    }
}

/* The unary operators +, - and ~.  They free the argument ARG1.  */

struct value *
value_pos (struct value *arg1)
{
  struct type *type;

  arg1 = coerce_ref (arg1);
  type = check_typedef (value_type (arg1));

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    return value_from_double (type, value_as_double (arg1));
  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    return value_from_decfloat (type, value_contents (arg1));
  else if (is_integral_type (type))
    {
      return value_from_longest (type, value_as_long (arg1));
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type))
    {
      struct value *val = allocate_value (type);

      memcpy (value_contents_raw (val), value_contents (arg1),
              TYPE_LENGTH (type));
      return val;
    }
  else
    {
      error (_("Argument to positive operation not a number."));
      return 0;			/* For lint -- never reached.  */
    }
}

struct value *
value_neg (struct value *arg1)
{
  struct type *type;

  arg1 = coerce_ref (arg1);
  type = check_typedef (value_type (arg1));

  if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    {
      struct value *val = allocate_value (type);
      int len = TYPE_LENGTH (type);
      gdb_byte decbytes[16];  /* a decfloat is at most 128 bits long.  */

      memcpy (decbytes, value_contents (arg1), len);

      if (gdbarch_byte_order (get_type_arch (type)) == BFD_ENDIAN_LITTLE)
	decbytes[len-1] = decbytes[len - 1] | 0x80;
      else
	decbytes[0] = decbytes[0] | 0x80;

      memcpy (value_contents_raw (val), decbytes, len);
      return val;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_FLT)
    return value_from_double (type, -value_as_double (arg1));
  else if (is_integral_type (type))
    {
      return value_from_longest (type, -value_as_long (arg1));
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type))
    {
      struct value *tmp, *val = allocate_value (type);
      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type));
      int i;
      LONGEST low_bound, high_bound;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the vector bounds"));

      for (i = 0; i < high_bound - low_bound + 1; i++)
	{
	  tmp = value_neg (value_subscript (arg1, i));
	  memcpy (value_contents_writeable (val) + i * TYPE_LENGTH (eltype),
		  value_contents_all (tmp), TYPE_LENGTH (eltype));
	}
      return val;
    }
  else
    {
      error (_("Argument to negate operation not a number."));
      return 0;			/* For lint -- never reached.  */
    }
}

struct value *
value_complement (struct value *arg1)
{
  struct type *type;
  struct value *val;

  arg1 = coerce_ref (arg1);
  type = check_typedef (value_type (arg1));

  if (is_integral_type (type))
    val = value_from_longest (type, ~value_as_long (arg1));
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type))
    {
      struct value *tmp;
      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type));
      int i;
      LONGEST low_bound, high_bound;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the vector bounds"));

      val = allocate_value (type);
      for (i = 0; i < high_bound - low_bound + 1; i++)
        {
          tmp = value_complement (value_subscript (arg1, i));
          memcpy (value_contents_writeable (val) + i * TYPE_LENGTH (eltype),
                  value_contents_all (tmp), TYPE_LENGTH (eltype));
        }
    }
  else
    error (_("Argument to complement operation not an integer, boolean."));

  return val;
}

/* The INDEX'th bit of SET value whose value_type is TYPE,
   and whose value_contents is valaddr.
   Return -1 if out of range, -2 other error.  */

int
value_bit_index (struct type *type, const gdb_byte *valaddr, int index)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  LONGEST low_bound, high_bound;
  LONGEST word;
  unsigned rel_index;
  struct type *range = TYPE_INDEX_TYPE (type);

  if (get_discrete_bounds (range, &low_bound, &high_bound) < 0)
    return -2;
  if (index < low_bound || index > high_bound)
    return -1;
  rel_index = index - low_bound;
  word = extract_unsigned_integer (valaddr + (rel_index / TARGET_CHAR_BIT), 1,
				   gdbarch_byte_order (gdbarch));
  rel_index %= TARGET_CHAR_BIT;
  if (gdbarch_bits_big_endian (gdbarch))
    rel_index = TARGET_CHAR_BIT - 1 - rel_index;
  return (word >> rel_index) & 1;
}

int
value_in (struct value *element, struct value *set)
{
  int member;
  struct type *settype = check_typedef (value_type (set));
  struct type *eltype = check_typedef (value_type (element));

  if (TYPE_CODE (eltype) == TYPE_CODE_RANGE)
    eltype = TYPE_TARGET_TYPE (eltype);
  if (TYPE_CODE (settype) != TYPE_CODE_SET)
    error (_("Second argument of 'IN' has wrong type"));
  if (TYPE_CODE (eltype) != TYPE_CODE_INT
      && TYPE_CODE (eltype) != TYPE_CODE_CHAR
      && TYPE_CODE (eltype) != TYPE_CODE_ENUM
      && TYPE_CODE (eltype) != TYPE_CODE_BOOL)
    error (_("First argument of 'IN' has wrong type"));
  member = value_bit_index (settype, value_contents (set),
			    value_as_long (element));
  if (member < 0)
    error (_("First argument of 'IN' not in range"));
  return member;
}

void
_initialize_valarith (void)
{
}
