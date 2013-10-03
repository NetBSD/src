/* Perform non-arithmetic operations on values, for GDB.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "value.h"
#include "frame.h"
#include "inferior.h"
#include "gdbcore.h"
#include "target.h"
#include "demangle.h"
#include "language.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "cp-abi.h"
#include "block.h"
#include "infcall.h"
#include "dictionary.h"
#include "cp-support.h"
#include "dfp.h"
#include "user-regs.h"
#include "tracepoint.h"
#include <errno.h>
#include "gdb_string.h"
#include "gdb_assert.h"
#include "cp-support.h"
#include "observer.h"
#include "objfiles.h"
#include "symtab.h"
#include "exceptions.h"

extern unsigned int overload_debug;
/* Local functions.  */

static int typecmp (int staticp, int varargs, int nargs,
		    struct field t1[], struct value *t2[]);

static struct value *search_struct_field (const char *, struct value *, 
					  int, struct type *, int);

static struct value *search_struct_method (const char *, struct value **,
					   struct value **,
					   int, int *, struct type *);

static int find_oload_champ_namespace (struct value **, int,
				       const char *, const char *,
				       struct symbol ***,
				       struct badness_vector **,
				       const int no_adl);

static
int find_oload_champ_namespace_loop (struct value **, int,
				     const char *, const char *,
				     int, struct symbol ***,
				     struct badness_vector **, int *,
				     const int no_adl);

static int find_oload_champ (struct value **, int, int, int,
			     struct fn_field *, struct symbol **,
			     struct badness_vector **);

static int oload_method_static (int, struct fn_field *, int);

enum oload_classification { STANDARD, NON_STANDARD, INCOMPATIBLE };

static enum
oload_classification classify_oload_match (struct badness_vector *,
					   int, int);

static struct value *value_struct_elt_for_reference (struct type *,
						     int, struct type *,
						     char *,
						     struct type *,
						     int, enum noside);

static struct value *value_namespace_elt (const struct type *,
					  char *, int , enum noside);

static struct value *value_maybe_namespace_elt (const struct type *,
						char *, int,
						enum noside);

static CORE_ADDR allocate_space_in_inferior (int);

static struct value *cast_into_complex (struct type *, struct value *);

static struct fn_field *find_method_list (struct value **, const char *,
					  int, struct type *, int *,
					  struct type **, int *);

void _initialize_valops (void);

#if 0
/* Flag for whether we want to abandon failed expression evals by
   default.  */

static int auto_abandon = 0;
#endif

int overload_resolution = 0;
static void
show_overload_resolution (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, 
			  const char *value)
{
  fprintf_filtered (file, _("Overload resolution in evaluating "
			    "C++ functions is %s.\n"),
		    value);
}

/* Find the address of function name NAME in the inferior.  If OBJF_P
   is non-NULL, *OBJF_P will be set to the OBJFILE where the function
   is defined.  */

struct value *
find_function_in_inferior (const char *name, struct objfile **objf_p)
{
  struct symbol *sym;

  sym = lookup_symbol (name, 0, VAR_DOMAIN, 0);
  if (sym != NULL)
    {
      if (SYMBOL_CLASS (sym) != LOC_BLOCK)
	{
	  error (_("\"%s\" exists in this program but is not a function."),
		 name);
	}

      if (objf_p)
	*objf_p = SYMBOL_SYMTAB (sym)->objfile;

      return value_of_variable (sym, NULL);
    }
  else
    {
      struct minimal_symbol *msymbol = 
	lookup_minimal_symbol (name, NULL, NULL);

      if (msymbol != NULL)
	{
	  struct objfile *objfile = msymbol_objfile (msymbol);
	  struct gdbarch *gdbarch = get_objfile_arch (objfile);

	  struct type *type;
	  CORE_ADDR maddr;
	  type = lookup_pointer_type (builtin_type (gdbarch)->builtin_char);
	  type = lookup_function_type (type);
	  type = lookup_pointer_type (type);
	  maddr = SYMBOL_VALUE_ADDRESS (msymbol);

	  if (objf_p)
	    *objf_p = objfile;

	  return value_from_pointer (type, maddr);
	}
      else
	{
	  if (!target_has_execution)
	    error (_("evaluation of this expression "
		     "requires the target program to be active"));
	  else
	    error (_("evaluation of this expression requires the "
		     "program to have a function \"%s\"."),
		   name);
	}
    }
}

/* Allocate NBYTES of space in the inferior using the inferior's
   malloc and return a value that is a pointer to the allocated
   space.  */

struct value *
value_allocate_space_in_inferior (int len)
{
  struct objfile *objf;
  struct value *val = find_function_in_inferior ("malloc", &objf);
  struct gdbarch *gdbarch = get_objfile_arch (objf);
  struct value *blocklen;

  blocklen = value_from_longest (builtin_type (gdbarch)->builtin_int, len);
  val = call_function_by_hand (val, 1, &blocklen);
  if (value_logical_not (val))
    {
      if (!target_has_execution)
	error (_("No memory available to program now: "
		 "you need to start the target first"));
      else
	error (_("No memory available to program: call to malloc failed"));
    }
  return val;
}

static CORE_ADDR
allocate_space_in_inferior (int len)
{
  return value_as_long (value_allocate_space_in_inferior (len));
}

/* Cast struct value VAL to type TYPE and return as a value.
   Both type and val must be of TYPE_CODE_STRUCT or TYPE_CODE_UNION
   for this to work.  Typedef to one of the codes is permitted.
   Returns NULL if the cast is neither an upcast nor a downcast.  */

static struct value *
value_cast_structs (struct type *type, struct value *v2)
{
  struct type *t1;
  struct type *t2;
  struct value *v;

  gdb_assert (type != NULL && v2 != NULL);

  t1 = check_typedef (type);
  t2 = check_typedef (value_type (v2));

  /* Check preconditions.  */
  gdb_assert ((TYPE_CODE (t1) == TYPE_CODE_STRUCT
	       || TYPE_CODE (t1) == TYPE_CODE_UNION)
	      && !!"Precondition is that type is of STRUCT or UNION kind.");
  gdb_assert ((TYPE_CODE (t2) == TYPE_CODE_STRUCT
	       || TYPE_CODE (t2) == TYPE_CODE_UNION)
	      && !!"Precondition is that value is of STRUCT or UNION kind");

  if (TYPE_NAME (t1) != NULL
      && TYPE_NAME (t2) != NULL
      && !strcmp (TYPE_NAME (t1), TYPE_NAME (t2)))
    return NULL;

  /* Upcasting: look in the type of the source to see if it contains the
     type of the target as a superclass.  If so, we'll need to
     offset the pointer rather than just change its type.  */
  if (TYPE_NAME (t1) != NULL)
    {
      v = search_struct_field (type_name_no_tag (t1),
			       v2, 0, t2, 1);
      if (v)
	return v;
    }

  /* Downcasting: look in the type of the target to see if it contains the
     type of the source as a superclass.  If so, we'll need to
     offset the pointer rather than just change its type.  */
  if (TYPE_NAME (t2) != NULL)
    {
      /* Try downcasting using the run-time type of the value.  */
      int full, top, using_enc;
      struct type *real_type;

      real_type = value_rtti_type (v2, &full, &top, &using_enc);
      if (real_type)
	{
	  v = value_full_object (v2, real_type, full, top, using_enc);
	  v = value_at_lazy (real_type, value_address (v));

	  /* We might be trying to cast to the outermost enclosing
	     type, in which case search_struct_field won't work.  */
	  if (TYPE_NAME (real_type) != NULL
	      && !strcmp (TYPE_NAME (real_type), TYPE_NAME (t1)))
	    return v;

	  v = search_struct_field (type_name_no_tag (t2), v, 0, real_type, 1);
	  if (v)
	    return v;
	}

      /* Try downcasting using information from the destination type
	 T2.  This wouldn't work properly for classes with virtual
	 bases, but those were handled above.  */
      v = search_struct_field (type_name_no_tag (t2),
			       value_zero (t1, not_lval), 0, t1, 1);
      if (v)
	{
	  /* Downcasting is possible (t1 is superclass of v2).  */
	  CORE_ADDR addr2 = value_address (v2);

	  addr2 -= value_address (v) + value_embedded_offset (v);
	  return value_at (type, addr2);
	}
    }

  return NULL;
}

/* Cast one pointer or reference type to another.  Both TYPE and
   the type of ARG2 should be pointer types, or else both should be
   reference types.  If SUBCLASS_CHECK is non-zero, this will force a
   check to see whether TYPE is a superclass of ARG2's type.  If
   SUBCLASS_CHECK is zero, then the subclass check is done only when
   ARG2 is itself non-zero.  Returns the new pointer or reference.  */

struct value *
value_cast_pointers (struct type *type, struct value *arg2,
		     int subclass_check)
{
  struct type *type1 = check_typedef (type);
  struct type *type2 = check_typedef (value_type (arg2));
  struct type *t1 = check_typedef (TYPE_TARGET_TYPE (type1));
  struct type *t2 = check_typedef (TYPE_TARGET_TYPE (type2));

  if (TYPE_CODE (t1) == TYPE_CODE_STRUCT
      && TYPE_CODE (t2) == TYPE_CODE_STRUCT
      && (subclass_check || !value_logical_not (arg2)))
    {
      struct value *v2;

      if (TYPE_CODE (type2) == TYPE_CODE_REF)
	v2 = coerce_ref (arg2);
      else
	v2 = value_ind (arg2);
      gdb_assert (TYPE_CODE (check_typedef (value_type (v2)))
		  == TYPE_CODE_STRUCT && !!"Why did coercion fail?");
      v2 = value_cast_structs (t1, v2);
      /* At this point we have what we can have, un-dereference if needed.  */
      if (v2)
	{
	  struct value *v = value_addr (v2);

	  deprecated_set_value_type (v, type);
	  return v;
	}
   }

  /* No superclass found, just change the pointer type.  */
  arg2 = value_copy (arg2);
  deprecated_set_value_type (arg2, type);
  set_value_enclosing_type (arg2, type);
  set_value_pointed_to_offset (arg2, 0);	/* pai: chk_val */
  return arg2;
}

/* Cast value ARG2 to type TYPE and return as a value.
   More general than a C cast: accepts any two types of the same length,
   and if ARG2 is an lvalue it can be cast into anything at all.  */
/* In C++, casts may change pointer or object representations.  */

struct value *
value_cast (struct type *type, struct value *arg2)
{
  enum type_code code1;
  enum type_code code2;
  int scalar;
  struct type *type2;

  int convert_to_boolean = 0;

  if (value_type (arg2) == type)
    return arg2;

  code1 = TYPE_CODE (check_typedef (type));

  /* Check if we are casting struct reference to struct reference.  */
  if (code1 == TYPE_CODE_REF)
    {
      /* We dereference type; then we recurse and finally
         we generate value of the given reference.  Nothing wrong with 
	 that.  */
      struct type *t1 = check_typedef (type);
      struct type *dereftype = check_typedef (TYPE_TARGET_TYPE (t1));
      struct value *val =  value_cast (dereftype, arg2);

      return value_ref (val); 
    }

  code2 = TYPE_CODE (check_typedef (value_type (arg2)));

  if (code2 == TYPE_CODE_REF)
    /* We deref the value and then do the cast.  */
    return value_cast (type, coerce_ref (arg2)); 

  CHECK_TYPEDEF (type);
  code1 = TYPE_CODE (type);
  arg2 = coerce_ref (arg2);
  type2 = check_typedef (value_type (arg2));

  /* You can't cast to a reference type.  See value_cast_pointers
     instead.  */
  gdb_assert (code1 != TYPE_CODE_REF);

  /* A cast to an undetermined-length array_type, such as 
     (TYPE [])OBJECT, is treated like a cast to (TYPE [N])OBJECT,
     where N is sizeof(OBJECT)/sizeof(TYPE).  */
  if (code1 == TYPE_CODE_ARRAY)
    {
      struct type *element_type = TYPE_TARGET_TYPE (type);
      unsigned element_length = TYPE_LENGTH (check_typedef (element_type));

      if (element_length > 0 && TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
	{
	  struct type *range_type = TYPE_INDEX_TYPE (type);
	  int val_length = TYPE_LENGTH (type2);
	  LONGEST low_bound, high_bound, new_length;

	  if (get_discrete_bounds (range_type, &low_bound, &high_bound) < 0)
	    low_bound = 0, high_bound = 0;
	  new_length = val_length / element_length;
	  if (val_length % element_length != 0)
	    warning (_("array element type size does not "
		       "divide object size in cast"));
	  /* FIXME-type-allocation: need a way to free this type when
	     we are done with it.  */
	  range_type = create_range_type ((struct type *) NULL,
					  TYPE_TARGET_TYPE (range_type),
					  low_bound,
					  new_length + low_bound - 1);
	  deprecated_set_value_type (arg2, 
				     create_array_type ((struct type *) NULL,
							element_type, 
							range_type));
	  return arg2;
	}
    }

  if (current_language->c_style_arrays
      && TYPE_CODE (type2) == TYPE_CODE_ARRAY
      && !TYPE_VECTOR (type2))
    arg2 = value_coerce_array (arg2);

  if (TYPE_CODE (type2) == TYPE_CODE_FUNC)
    arg2 = value_coerce_function (arg2);

  type2 = check_typedef (value_type (arg2));
  code2 = TYPE_CODE (type2);

  if (code1 == TYPE_CODE_COMPLEX)
    return cast_into_complex (type, arg2);
  if (code1 == TYPE_CODE_BOOL)
    {
      code1 = TYPE_CODE_INT;
      convert_to_boolean = 1;
    }
  if (code1 == TYPE_CODE_CHAR)
    code1 = TYPE_CODE_INT;
  if (code2 == TYPE_CODE_BOOL || code2 == TYPE_CODE_CHAR)
    code2 = TYPE_CODE_INT;

  scalar = (code2 == TYPE_CODE_INT || code2 == TYPE_CODE_FLT
	    || code2 == TYPE_CODE_DECFLOAT || code2 == TYPE_CODE_ENUM
	    || code2 == TYPE_CODE_RANGE);

  if ((code1 == TYPE_CODE_STRUCT || code1 == TYPE_CODE_UNION)
      && (code2 == TYPE_CODE_STRUCT || code2 == TYPE_CODE_UNION)
      && TYPE_NAME (type) != 0)
    {
      struct value *v = value_cast_structs (type, arg2);

      if (v)
	return v;
    }

  if (code1 == TYPE_CODE_FLT && scalar)
    return value_from_double (type, value_as_double (arg2));
  else if (code1 == TYPE_CODE_DECFLOAT && scalar)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
      int dec_len = TYPE_LENGTH (type);
      gdb_byte dec[16];

      if (code2 == TYPE_CODE_FLT)
	decimal_from_floating (arg2, dec, dec_len, byte_order);
      else if (code2 == TYPE_CODE_DECFLOAT)
	decimal_convert (value_contents (arg2), TYPE_LENGTH (type2),
			 byte_order, dec, dec_len, byte_order);
      else
	/* The only option left is an integral type.  */
	decimal_from_integral (arg2, dec, dec_len, byte_order);

      return value_from_decfloat (type, dec);
    }
  else if ((code1 == TYPE_CODE_INT || code1 == TYPE_CODE_ENUM
	    || code1 == TYPE_CODE_RANGE)
	   && (scalar || code2 == TYPE_CODE_PTR
	       || code2 == TYPE_CODE_MEMBERPTR))
    {
      LONGEST longest;

      /* When we cast pointers to integers, we mustn't use
         gdbarch_pointer_to_address to find the address the pointer
         represents, as value_as_long would.  GDB should evaluate
         expressions just as the compiler would --- and the compiler
         sees a cast as a simple reinterpretation of the pointer's
         bits.  */
      if (code2 == TYPE_CODE_PTR)
        longest = extract_unsigned_integer
		    (value_contents (arg2), TYPE_LENGTH (type2),
		     gdbarch_byte_order (get_type_arch (type2)));
      else
        longest = value_as_long (arg2);
      return value_from_longest (type, convert_to_boolean ?
				 (LONGEST) (longest ? 1 : 0) : longest);
    }
  else if (code1 == TYPE_CODE_PTR && (code2 == TYPE_CODE_INT  
				      || code2 == TYPE_CODE_ENUM 
				      || code2 == TYPE_CODE_RANGE))
    {
      /* TYPE_LENGTH (type) is the length of a pointer, but we really
	 want the length of an address! -- we are really dealing with
	 addresses (i.e., gdb representations) not pointers (i.e.,
	 target representations) here.

	 This allows things like "print *(int *)0x01000234" to work
	 without printing a misleading message -- which would
	 otherwise occur when dealing with a target having two byte
	 pointers and four byte addresses.  */

      int addr_bit = gdbarch_addr_bit (get_type_arch (type2));
      LONGEST longest = value_as_long (arg2);

      if (addr_bit < sizeof (LONGEST) * HOST_CHAR_BIT)
	{
	  if (longest >= ((LONGEST) 1 << addr_bit)
	      || longest <= -((LONGEST) 1 << addr_bit))
	    warning (_("value truncated"));
	}
      return value_from_longest (type, longest);
    }
  else if (code1 == TYPE_CODE_METHODPTR && code2 == TYPE_CODE_INT
	   && value_as_long (arg2) == 0)
    {
      struct value *result = allocate_value (type);

      cplus_make_method_ptr (type, value_contents_writeable (result), 0, 0);
      return result;
    }
  else if (code1 == TYPE_CODE_MEMBERPTR && code2 == TYPE_CODE_INT
	   && value_as_long (arg2) == 0)
    {
      /* The Itanium C++ ABI represents NULL pointers to members as
	 minus one, instead of biasing the normal case.  */
      return value_from_longest (type, -1);
    }
  else if (code1 == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
	   && code2 == TYPE_CODE_ARRAY && TYPE_VECTOR (type2)
	   && TYPE_LENGTH (type) != TYPE_LENGTH (type2))
    error (_("Cannot convert between vector values of different sizes"));
  else if (code1 == TYPE_CODE_ARRAY && TYPE_VECTOR (type) && scalar
	   && TYPE_LENGTH (type) != TYPE_LENGTH (type2))
    error (_("can only cast scalar to vector of same size"));
  else if (code1 == TYPE_CODE_VOID)
    {
      return value_zero (type, not_lval);
    }
  else if (TYPE_LENGTH (type) == TYPE_LENGTH (type2))
    {
      if (code1 == TYPE_CODE_PTR && code2 == TYPE_CODE_PTR)
	return value_cast_pointers (type, arg2, 0);

      arg2 = value_copy (arg2);
      deprecated_set_value_type (arg2, type);
      set_value_enclosing_type (arg2, type);
      set_value_pointed_to_offset (arg2, 0);	/* pai: chk_val */
      return arg2;
    }
  else if (VALUE_LVAL (arg2) == lval_memory)
    return value_at_lazy (type, value_address (arg2));
  else
    {
      error (_("Invalid cast."));
      return 0;
    }
}

/* The C++ reinterpret_cast operator.  */

struct value *
value_reinterpret_cast (struct type *type, struct value *arg)
{
  struct value *result;
  struct type *real_type = check_typedef (type);
  struct type *arg_type, *dest_type;
  int is_ref = 0;
  enum type_code dest_code, arg_code;

  /* Do reference, function, and array conversion.  */
  arg = coerce_array (arg);

  /* Attempt to preserve the type the user asked for.  */
  dest_type = type;

  /* If we are casting to a reference type, transform
     reinterpret_cast<T&>(V) to *reinterpret_cast<T*>(&V).  */
  if (TYPE_CODE (real_type) == TYPE_CODE_REF)
    {
      is_ref = 1;
      arg = value_addr (arg);
      dest_type = lookup_pointer_type (TYPE_TARGET_TYPE (dest_type));
      real_type = lookup_pointer_type (real_type);
    }

  arg_type = value_type (arg);

  dest_code = TYPE_CODE (real_type);
  arg_code = TYPE_CODE (arg_type);

  /* We can convert pointer types, or any pointer type to int, or int
     type to pointer.  */
  if ((dest_code == TYPE_CODE_PTR && arg_code == TYPE_CODE_INT)
      || (dest_code == TYPE_CODE_INT && arg_code == TYPE_CODE_PTR)
      || (dest_code == TYPE_CODE_METHODPTR && arg_code == TYPE_CODE_INT)
      || (dest_code == TYPE_CODE_INT && arg_code == TYPE_CODE_METHODPTR)
      || (dest_code == TYPE_CODE_MEMBERPTR && arg_code == TYPE_CODE_INT)
      || (dest_code == TYPE_CODE_INT && arg_code == TYPE_CODE_MEMBERPTR)
      || (dest_code == arg_code
	  && (dest_code == TYPE_CODE_PTR
	      || dest_code == TYPE_CODE_METHODPTR
	      || dest_code == TYPE_CODE_MEMBERPTR)))
    result = value_cast (dest_type, arg);
  else
    error (_("Invalid reinterpret_cast"));

  if (is_ref)
    result = value_cast (type, value_ref (value_ind (result)));

  return result;
}

/* A helper for value_dynamic_cast.  This implements the first of two
   runtime checks: we iterate over all the base classes of the value's
   class which are equal to the desired class; if only one of these
   holds the value, then it is the answer.  */

static int
dynamic_cast_check_1 (struct type *desired_type,
		      const gdb_byte *valaddr,
		      int embedded_offset,
		      CORE_ADDR address,
		      struct value *val,
		      struct type *search_type,
		      CORE_ADDR arg_addr,
		      struct type *arg_type,
		      struct value **result)
{
  int i, result_count = 0;

  for (i = 0; i < TYPE_N_BASECLASSES (search_type) && result_count < 2; ++i)
    {
      int offset = baseclass_offset (search_type, i, valaddr, embedded_offset,
				     address, val);

      if (class_types_same_p (desired_type, TYPE_BASECLASS (search_type, i)))
	{
	  if (address + embedded_offset + offset >= arg_addr
	      && address + embedded_offset + offset < arg_addr + TYPE_LENGTH (arg_type))
	    {
	      ++result_count;
	      if (!*result)
		*result = value_at_lazy (TYPE_BASECLASS (search_type, i),
					 address + embedded_offset + offset);
	    }
	}
      else
	result_count += dynamic_cast_check_1 (desired_type,
					      valaddr,
					      embedded_offset + offset,
					      address, val,
					      TYPE_BASECLASS (search_type, i),
					      arg_addr,
					      arg_type,
					      result);
    }

  return result_count;
}

/* A helper for value_dynamic_cast.  This implements the second of two
   runtime checks: we look for a unique public sibling class of the
   argument's declared class.  */

static int
dynamic_cast_check_2 (struct type *desired_type,
		      const gdb_byte *valaddr,
		      int embedded_offset,
		      CORE_ADDR address,
		      struct value *val,
		      struct type *search_type,
		      struct value **result)
{
  int i, result_count = 0;

  for (i = 0; i < TYPE_N_BASECLASSES (search_type) && result_count < 2; ++i)
    {
      int offset;

      if (! BASETYPE_VIA_PUBLIC (search_type, i))
	continue;

      offset = baseclass_offset (search_type, i, valaddr, embedded_offset,
				 address, val);
      if (class_types_same_p (desired_type, TYPE_BASECLASS (search_type, i)))
	{
	  ++result_count;
	  if (*result == NULL)
	    *result = value_at_lazy (TYPE_BASECLASS (search_type, i),
				     address + embedded_offset + offset);
	}
      else
	result_count += dynamic_cast_check_2 (desired_type,
					      valaddr,
					      embedded_offset + offset,
					      address, val,
					      TYPE_BASECLASS (search_type, i),
					      result);
    }

  return result_count;
}

/* The C++ dynamic_cast operator.  */

struct value *
value_dynamic_cast (struct type *type, struct value *arg)
{
  int full, top, using_enc;
  struct type *resolved_type = check_typedef (type);
  struct type *arg_type = check_typedef (value_type (arg));
  struct type *class_type, *rtti_type;
  struct value *result, *tem, *original_arg = arg;
  CORE_ADDR addr;
  int is_ref = TYPE_CODE (resolved_type) == TYPE_CODE_REF;

  if (TYPE_CODE (resolved_type) != TYPE_CODE_PTR
      && TYPE_CODE (resolved_type) != TYPE_CODE_REF)
    error (_("Argument to dynamic_cast must be a pointer or reference type"));
  if (TYPE_CODE (TYPE_TARGET_TYPE (resolved_type)) != TYPE_CODE_VOID
      && TYPE_CODE (TYPE_TARGET_TYPE (resolved_type)) != TYPE_CODE_CLASS)
    error (_("Argument to dynamic_cast must be pointer to class or `void *'"));

  class_type = check_typedef (TYPE_TARGET_TYPE (resolved_type));
  if (TYPE_CODE (resolved_type) == TYPE_CODE_PTR)
    {
      if (TYPE_CODE (arg_type) != TYPE_CODE_PTR
	  && ! (TYPE_CODE (arg_type) == TYPE_CODE_INT
		&& value_as_long (arg) == 0))
	error (_("Argument to dynamic_cast does not have pointer type"));
      if (TYPE_CODE (arg_type) == TYPE_CODE_PTR)
	{
	  arg_type = check_typedef (TYPE_TARGET_TYPE (arg_type));
	  if (TYPE_CODE (arg_type) != TYPE_CODE_CLASS)
	    error (_("Argument to dynamic_cast does "
		     "not have pointer to class type"));
	}

      /* Handle NULL pointers.  */
      if (value_as_long (arg) == 0)
	return value_zero (type, not_lval);

      arg = value_ind (arg);
    }
  else
    {
      if (TYPE_CODE (arg_type) != TYPE_CODE_CLASS)
	error (_("Argument to dynamic_cast does not have class type"));
    }

  /* If the classes are the same, just return the argument.  */
  if (class_types_same_p (class_type, arg_type))
    return value_cast (type, arg);

  /* If the target type is a unique base class of the argument's
     declared type, just cast it.  */
  if (is_ancestor (class_type, arg_type))
    {
      if (is_unique_ancestor (class_type, arg))
	return value_cast (type, original_arg);
      error (_("Ambiguous dynamic_cast"));
    }

  rtti_type = value_rtti_type (arg, &full, &top, &using_enc);
  if (! rtti_type)
    error (_("Couldn't determine value's most derived type for dynamic_cast"));

  /* Compute the most derived object's address.  */
  addr = value_address (arg);
  if (full)
    {
      /* Done.  */
    }
  else if (using_enc)
    addr += top;
  else
    addr += top + value_embedded_offset (arg);

  /* dynamic_cast<void *> means to return a pointer to the
     most-derived object.  */
  if (TYPE_CODE (resolved_type) == TYPE_CODE_PTR
      && TYPE_CODE (TYPE_TARGET_TYPE (resolved_type)) == TYPE_CODE_VOID)
    return value_at_lazy (type, addr);

  tem = value_at (type, addr);

  /* The first dynamic check specified in 5.2.7.  */
  if (is_public_ancestor (arg_type, TYPE_TARGET_TYPE (resolved_type)))
    {
      if (class_types_same_p (rtti_type, TYPE_TARGET_TYPE (resolved_type)))
	return tem;
      result = NULL;
      if (dynamic_cast_check_1 (TYPE_TARGET_TYPE (resolved_type),
				value_contents_for_printing (tem),
				value_embedded_offset (tem),
				value_address (tem), tem,
				rtti_type, addr,
				arg_type,
				&result) == 1)
	return value_cast (type,
			   is_ref ? value_ref (result) : value_addr (result));
    }

  /* The second dynamic check specified in 5.2.7.  */
  result = NULL;
  if (is_public_ancestor (arg_type, rtti_type)
      && dynamic_cast_check_2 (TYPE_TARGET_TYPE (resolved_type),
			       value_contents_for_printing (tem),
			       value_embedded_offset (tem),
			       value_address (tem), tem,
			       rtti_type, &result) == 1)
    return value_cast (type,
		       is_ref ? value_ref (result) : value_addr (result));

  if (TYPE_CODE (resolved_type) == TYPE_CODE_PTR)
    return value_zero (type, not_lval);

  error (_("dynamic_cast failed"));
}

/* Create a value of type TYPE that is zero, and return it.  */

struct value *
value_zero (struct type *type, enum lval_type lv)
{
  struct value *val = allocate_value (type);

  VALUE_LVAL (val) = (lv == lval_computed ? not_lval : lv);
  return val;
}

/* Create a not_lval value of numeric type TYPE that is one, and return it.  */

struct value *
value_one (struct type *type)
{
  struct type *type1 = check_typedef (type);
  struct value *val;

  if (TYPE_CODE (type1) == TYPE_CODE_DECFLOAT)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
      gdb_byte v[16];

      decimal_from_string (v, TYPE_LENGTH (type), byte_order, "1");
      val = value_from_decfloat (type, v);
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_FLT)
    {
      val = value_from_double (type, (DOUBLEST) 1);
    }
  else if (is_integral_type (type1))
    {
      val = value_from_longest (type, (LONGEST) 1);
    }
  else if (TYPE_CODE (type1) == TYPE_CODE_ARRAY && TYPE_VECTOR (type1))
    {
      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type1));
      int i;
      LONGEST low_bound, high_bound;
      struct value *tmp;

      if (!get_array_bounds (type1, &low_bound, &high_bound))
	error (_("Could not determine the vector bounds"));

      val = allocate_value (type);
      for (i = 0; i < high_bound - low_bound + 1; i++)
	{
	  tmp = value_one (eltype);
	  memcpy (value_contents_writeable (val) + i * TYPE_LENGTH (eltype),
		  value_contents_all (tmp), TYPE_LENGTH (eltype));
	}
    }
  else
    {
      error (_("Not a numeric type."));
    }

  /* value_one result is never used for assignments to.  */
  gdb_assert (VALUE_LVAL (val) == not_lval);

  return val;
}

/* Helper function for value_at, value_at_lazy, and value_at_lazy_stack.  */

static struct value *
get_value_at (struct type *type, CORE_ADDR addr, int lazy)
{
  struct value *val;

  if (TYPE_CODE (check_typedef (type)) == TYPE_CODE_VOID)
    error (_("Attempt to dereference a generic pointer."));

  val = value_from_contents_and_address (type, NULL, addr);

  if (!lazy)
    value_fetch_lazy (val);

  return val;
}

/* Return a value with type TYPE located at ADDR.

   Call value_at only if the data needs to be fetched immediately;
   if we can be 'lazy' and defer the fetch, perhaps indefinately, call
   value_at_lazy instead.  value_at_lazy simply records the address of
   the data and sets the lazy-evaluation-required flag.  The lazy flag
   is tested in the value_contents macro, which is used if and when
   the contents are actually required.

   Note: value_at does *NOT* handle embedded offsets; perform such
   adjustments before or after calling it.  */

struct value *
value_at (struct type *type, CORE_ADDR addr)
{
  return get_value_at (type, addr, 0);
}

/* Return a lazy value with type TYPE located at ADDR (cf. value_at).  */

struct value *
value_at_lazy (struct type *type, CORE_ADDR addr)
{
  return get_value_at (type, addr, 1);
}

/* Called only from the value_contents and value_contents_all()
   macros, if the current data for a variable needs to be loaded into
   value_contents(VAL).  Fetches the data from the user's process, and
   clears the lazy flag to indicate that the data in the buffer is
   valid.

   If the value is zero-length, we avoid calling read_memory, which
   would abort.  We mark the value as fetched anyway -- all 0 bytes of
   it.

   This function returns a value because it is used in the
   value_contents macro as part of an expression, where a void would
   not work.  The value is ignored.  */

int
value_fetch_lazy (struct value *val)
{
  gdb_assert (value_lazy (val));
  allocate_value_contents (val);
  if (value_bitsize (val))
    {
      /* To read a lazy bitfield, read the entire enclosing value.  This
	 prevents reading the same block of (possibly volatile) memory once
         per bitfield.  It would be even better to read only the containing
         word, but we have no way to record that just specific bits of a
         value have been fetched.  */
      struct type *type = check_typedef (value_type (val));
      enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
      struct value *parent = value_parent (val);
      LONGEST offset = value_offset (val);
      LONGEST num;

      if (!value_bits_valid (val,
			     TARGET_CHAR_BIT * offset + value_bitpos (val),
			     value_bitsize (val)))
	error (_("value has been optimized out"));

      if (!unpack_value_bits_as_long (value_type (val),
				      value_contents_for_printing (parent),
				      offset,
				      value_bitpos (val),
				      value_bitsize (val), parent, &num))
	mark_value_bytes_unavailable (val,
				      value_embedded_offset (val),
				      TYPE_LENGTH (type));
      else
	store_signed_integer (value_contents_raw (val), TYPE_LENGTH (type),
			      byte_order, num);
    }
  else if (VALUE_LVAL (val) == lval_memory)
    {
      CORE_ADDR addr = value_address (val);
      struct type *type = check_typedef (value_enclosing_type (val));

      if (TYPE_LENGTH (type))
	read_value_memory (val, 0, value_stack (val),
			   addr, value_contents_all_raw (val),
			   TYPE_LENGTH (type));
    }
  else if (VALUE_LVAL (val) == lval_register)
    {
      struct frame_info *frame;
      int regnum;
      struct type *type = check_typedef (value_type (val));
      struct value *new_val = val, *mark = value_mark ();

      /* Offsets are not supported here; lazy register values must
	 refer to the entire register.  */
      gdb_assert (value_offset (val) == 0);

      while (VALUE_LVAL (new_val) == lval_register && value_lazy (new_val))
	{
	  frame = frame_find_by_id (VALUE_FRAME_ID (new_val));
	  regnum = VALUE_REGNUM (new_val);

	  gdb_assert (frame != NULL);

	  /* Convertible register routines are used for multi-register
	     values and for interpretation in different types
	     (e.g. float or int from a double register).  Lazy
	     register values should have the register's natural type,
	     so they do not apply.  */
	  gdb_assert (!gdbarch_convert_register_p (get_frame_arch (frame),
						   regnum, type));

	  new_val = get_frame_register_value (frame, regnum);
	}

      /* If it's still lazy (for instance, a saved register on the
	 stack), fetch it.  */
      if (value_lazy (new_val))
	value_fetch_lazy (new_val);

      /* If the register was not saved, mark it optimized out.  */
      if (value_optimized_out (new_val))
	set_value_optimized_out (val, 1);
      else
	{
	  set_value_lazy (val, 0);
	  value_contents_copy (val, value_embedded_offset (val),
			       new_val, value_embedded_offset (new_val),
			       TYPE_LENGTH (type));
	}

      if (frame_debug)
	{
	  struct gdbarch *gdbarch;
	  frame = frame_find_by_id (VALUE_FRAME_ID (val));
	  regnum = VALUE_REGNUM (val);
	  gdbarch = get_frame_arch (frame);

	  fprintf_unfiltered (gdb_stdlog,
			      "{ value_fetch_lazy "
			      "(frame=%d,regnum=%d(%s),...) ",
			      frame_relative_level (frame), regnum,
			      user_reg_map_regnum_to_name (gdbarch, regnum));

	  fprintf_unfiltered (gdb_stdlog, "->");
	  if (value_optimized_out (new_val))
	    fprintf_unfiltered (gdb_stdlog, " optimized out");
	  else
	    {
	      int i;
	      const gdb_byte *buf = value_contents (new_val);

	      if (VALUE_LVAL (new_val) == lval_register)
		fprintf_unfiltered (gdb_stdlog, " register=%d",
				    VALUE_REGNUM (new_val));
	      else if (VALUE_LVAL (new_val) == lval_memory)
		fprintf_unfiltered (gdb_stdlog, " address=%s",
				    paddress (gdbarch,
					      value_address (new_val)));
	      else
		fprintf_unfiltered (gdb_stdlog, " computed");

	      fprintf_unfiltered (gdb_stdlog, " bytes=");
	      fprintf_unfiltered (gdb_stdlog, "[");
	      for (i = 0; i < register_size (gdbarch, regnum); i++)
		fprintf_unfiltered (gdb_stdlog, "%02x", buf[i]);
	      fprintf_unfiltered (gdb_stdlog, "]");
	    }

	  fprintf_unfiltered (gdb_stdlog, " }\n");
	}

      /* Dispose of the intermediate values.  This prevents
	 watchpoints from trying to watch the saved frame pointer.  */
      value_free_to_mark (mark);
    }
  else if (VALUE_LVAL (val) == lval_computed
	   && value_computed_funcs (val)->read != NULL)
    value_computed_funcs (val)->read (val);
  else if (value_optimized_out (val))
    /* Keep it optimized out.  */;
  else
    internal_error (__FILE__, __LINE__, _("Unexpected lazy value type."));

  set_value_lazy (val, 0);
  return 0;
}

void
read_value_memory (struct value *val, int embedded_offset,
		   int stack, CORE_ADDR memaddr,
		   gdb_byte *buffer, size_t length)
{
  if (length)
    {
      VEC(mem_range_s) *available_memory;

      if (get_traceframe_number () < 0
	  || !traceframe_available_memory (&available_memory, memaddr, length))
	{
	  if (stack)
	    read_stack (memaddr, buffer, length);
	  else
	    read_memory (memaddr, buffer, length);
	}
      else
	{
	  struct target_section_table *table;
	  struct cleanup *old_chain;
	  CORE_ADDR unavail;
	  mem_range_s *r;
	  int i;

	  /* Fallback to reading from read-only sections.  */
	  table = target_get_section_table (&exec_ops);
	  available_memory =
	    section_table_available_memory (available_memory,
					    memaddr, length,
					    table->sections,
					    table->sections_end);

	  old_chain = make_cleanup (VEC_cleanup(mem_range_s),
				    &available_memory);

	  normalize_mem_ranges (available_memory);

	  /* Mark which bytes are unavailable, and read those which
	     are available.  */

	  unavail = memaddr;

	  for (i = 0;
	       VEC_iterate (mem_range_s, available_memory, i, r);
	       i++)
	    {
	      if (mem_ranges_overlap (r->start, r->length,
				      memaddr, length))
		{
		  CORE_ADDR lo1, hi1, lo2, hi2;
		  CORE_ADDR start, end;

		  /* Get the intersection window.  */
		  lo1 = memaddr;
		  hi1 = memaddr + length;
		  lo2 = r->start;
		  hi2 = r->start + r->length;
		  start = max (lo1, lo2);
		  end = min (hi1, hi2);

		  gdb_assert (end - memaddr <= length);

		  if (start > unavail)
		    mark_value_bytes_unavailable (val,
						  (embedded_offset
						   + unavail - memaddr),
						  start - unavail);
		  unavail = end;

		  read_memory (start, buffer + start - memaddr, end - start);
		}
	    }

	  if (unavail != memaddr + length)
	    mark_value_bytes_unavailable (val,
					  embedded_offset + unavail - memaddr,
					  (memaddr + length) - unavail);

	  do_cleanups (old_chain);
	}
    }
}

/* Store the contents of FROMVAL into the location of TOVAL.
   Return a new value with the location of TOVAL and contents of FROMVAL.  */

struct value *
value_assign (struct value *toval, struct value *fromval)
{
  struct type *type;
  struct value *val;
  struct frame_id old_frame;

  if (!deprecated_value_modifiable (toval))
    error (_("Left operand of assignment is not a modifiable lvalue."));

  toval = coerce_ref (toval);

  type = value_type (toval);
  if (VALUE_LVAL (toval) != lval_internalvar)
    fromval = value_cast (type, fromval);
  else
    {
      /* Coerce arrays and functions to pointers, except for arrays
	 which only live in GDB's storage.  */
      if (!value_must_coerce_to_target (fromval))
	fromval = coerce_array (fromval);
    }

  CHECK_TYPEDEF (type);

  /* Since modifying a register can trash the frame chain, and
     modifying memory can trash the frame cache, we save the old frame
     and then restore the new frame afterwards.  */
  old_frame = get_frame_id (deprecated_safe_get_selected_frame ());

  switch (VALUE_LVAL (toval))
    {
    case lval_internalvar:
      set_internalvar (VALUE_INTERNALVAR (toval), fromval);
      return value_of_internalvar (get_type_arch (type),
				   VALUE_INTERNALVAR (toval));

    case lval_internalvar_component:
      {
	int offset = value_offset (toval);

	/* Are we dealing with a bitfield?

	   It is important to mention that `value_parent (toval)' is
	   non-NULL iff `value_bitsize (toval)' is non-zero.  */
	if (value_bitsize (toval))
	  {
	    /* VALUE_INTERNALVAR below refers to the parent value, while
	       the offset is relative to this parent value.  */
	    gdb_assert (value_parent (value_parent (toval)) == NULL);
	    offset += value_offset (value_parent (toval));
	  }

	set_internalvar_component (VALUE_INTERNALVAR (toval),
				   offset,
				   value_bitpos (toval),
				   value_bitsize (toval),
				   fromval);
      }
      break;

    case lval_memory:
      {
	const gdb_byte *dest_buffer;
	CORE_ADDR changed_addr;
	int changed_len;
        gdb_byte buffer[sizeof (LONGEST)];

	if (value_bitsize (toval))
	  {
	    struct value *parent = value_parent (toval);

	    changed_addr = value_address (parent) + value_offset (toval);
	    changed_len = (value_bitpos (toval)
			   + value_bitsize (toval)
			   + HOST_CHAR_BIT - 1)
	      / HOST_CHAR_BIT;

	    /* If we can read-modify-write exactly the size of the
	       containing type (e.g. short or int) then do so.  This
	       is safer for volatile bitfields mapped to hardware
	       registers.  */
	    if (changed_len < TYPE_LENGTH (type)
		&& TYPE_LENGTH (type) <= (int) sizeof (LONGEST)
		&& ((LONGEST) changed_addr % TYPE_LENGTH (type)) == 0)
	      changed_len = TYPE_LENGTH (type);

	    if (changed_len > (int) sizeof (LONGEST))
	      error (_("Can't handle bitfields which "
		       "don't fit in a %d bit word."),
		     (int) sizeof (LONGEST) * HOST_CHAR_BIT);

	    read_memory (changed_addr, buffer, changed_len);
	    modify_field (type, buffer, value_as_long (fromval),
			  value_bitpos (toval), value_bitsize (toval));
	    dest_buffer = buffer;
	  }
	else
	  {
	    changed_addr = value_address (toval);
	    changed_len = TYPE_LENGTH (type);
	    dest_buffer = value_contents (fromval);
	  }

	write_memory_with_notification (changed_addr, dest_buffer, changed_len);
      }
      break;

    case lval_register:
      {
	struct frame_info *frame;
	struct gdbarch *gdbarch;
	int value_reg;

	/* Figure out which frame this is in currently.  */
	frame = frame_find_by_id (VALUE_FRAME_ID (toval));
	value_reg = VALUE_REGNUM (toval);

	if (!frame)
	  error (_("Value being assigned to is no longer active."));

	gdbarch = get_frame_arch (frame);
	if (gdbarch_convert_register_p (gdbarch, VALUE_REGNUM (toval), type))
	  {
	    /* If TOVAL is a special machine register requiring
	       conversion of program values to a special raw
	       format.  */
	    gdbarch_value_to_register (gdbarch, frame,
				       VALUE_REGNUM (toval), type,
				       value_contents (fromval));
	  }
	else
	  {
	    if (value_bitsize (toval))
	      {
		struct value *parent = value_parent (toval);
		int offset = value_offset (parent) + value_offset (toval);
		int changed_len;
		gdb_byte buffer[sizeof (LONGEST)];
		int optim, unavail;

		changed_len = (value_bitpos (toval)
			       + value_bitsize (toval)
			       + HOST_CHAR_BIT - 1)
		  / HOST_CHAR_BIT;

		if (changed_len > (int) sizeof (LONGEST))
		  error (_("Can't handle bitfields which "
			   "don't fit in a %d bit word."),
			 (int) sizeof (LONGEST) * HOST_CHAR_BIT);

		if (!get_frame_register_bytes (frame, value_reg, offset,
					       changed_len, buffer,
					       &optim, &unavail))
		  {
		    if (optim)
		      error (_("value has been optimized out"));
		    if (unavail)
		      throw_error (NOT_AVAILABLE_ERROR,
				   _("value is not available"));
		  }

		modify_field (type, buffer, value_as_long (fromval),
			      value_bitpos (toval), value_bitsize (toval));

		put_frame_register_bytes (frame, value_reg, offset,
					  changed_len, buffer);
	      }
	    else
	      {
		put_frame_register_bytes (frame, value_reg,
					  value_offset (toval),
					  TYPE_LENGTH (type),
					  value_contents (fromval));
	      }
	  }

	if (deprecated_register_changed_hook)
	  deprecated_register_changed_hook (-1);
	break;
      }

    case lval_computed:
      {
	const struct lval_funcs *funcs = value_computed_funcs (toval);

	if (funcs->write != NULL)
	  {
	    funcs->write (toval, fromval);
	    break;
	  }
      }
      /* Fall through.  */

    default:
      error (_("Left operand of assignment is not an lvalue."));
    }

  /* Assigning to the stack pointer, frame pointer, and other
     (architecture and calling convention specific) registers may
     cause the frame cache and regcache to be out of date.  Assigning to memory
     also can.  We just do this on all assignments to registers or
     memory, for simplicity's sake; I doubt the slowdown matters.  */
  switch (VALUE_LVAL (toval))
    {
    case lval_memory:
    case lval_register:
    case lval_computed:

      observer_notify_target_changed (&current_target);

      /* Having destroyed the frame cache, restore the selected
	 frame.  */

      /* FIXME: cagney/2002-11-02: There has to be a better way of
	 doing this.  Instead of constantly saving/restoring the
	 frame.  Why not create a get_selected_frame() function that,
	 having saved the selected frame's ID can automatically
	 re-find the previously selected frame automatically.  */

      {
	struct frame_info *fi = frame_find_by_id (old_frame);

	if (fi != NULL)
	  select_frame (fi);
      }

      break;
    default:
      break;
    }
  
  /* If the field does not entirely fill a LONGEST, then zero the sign
     bits.  If the field is signed, and is negative, then sign
     extend.  */
  if ((value_bitsize (toval) > 0)
      && (value_bitsize (toval) < 8 * (int) sizeof (LONGEST)))
    {
      LONGEST fieldval = value_as_long (fromval);
      LONGEST valmask = (((ULONGEST) 1) << value_bitsize (toval)) - 1;

      fieldval &= valmask;
      if (!TYPE_UNSIGNED (type) 
	  && (fieldval & (valmask ^ (valmask >> 1))))
	fieldval |= ~valmask;

      fromval = value_from_longest (type, fieldval);
    }

  /* The return value is a copy of TOVAL so it shares its location
     information, but its contents are updated from FROMVAL.  This
     implies the returned value is not lazy, even if TOVAL was.  */
  val = value_copy (toval);
  set_value_lazy (val, 0);
  memcpy (value_contents_raw (val), value_contents (fromval),
	  TYPE_LENGTH (type));

  /* We copy over the enclosing type and pointed-to offset from FROMVAL
     in the case of pointer types.  For object types, the enclosing type
     and embedded offset must *not* be copied: the target object refered
     to by TOVAL retains its original dynamic type after assignment.  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      set_value_enclosing_type (val, value_enclosing_type (fromval));
      set_value_pointed_to_offset (val, value_pointed_to_offset (fromval));
    }

  return val;
}

/* Extend a value VAL to COUNT repetitions of its type.  */

struct value *
value_repeat (struct value *arg1, int count)
{
  struct value *val;

  if (VALUE_LVAL (arg1) != lval_memory)
    error (_("Only values in memory can be extended with '@'."));
  if (count < 1)
    error (_("Invalid number %d of repetitions."), count);

  val = allocate_repeat_value (value_enclosing_type (arg1), count);

  VALUE_LVAL (val) = lval_memory;
  set_value_address (val, value_address (arg1));

  read_value_memory (val, 0, value_stack (val), value_address (val),
		     value_contents_all_raw (val),
		     TYPE_LENGTH (value_enclosing_type (val)));

  return val;
}

struct value *
value_of_variable (struct symbol *var, const struct block *b)
{
  struct frame_info *frame;

  if (!symbol_read_needs_frame (var))
    frame = NULL;
  else if (!b)
    frame = get_selected_frame (_("No frame selected."));
  else
    {
      frame = block_innermost_frame (b);
      if (!frame)
	{
	  if (BLOCK_FUNCTION (b) && !block_inlined_p (b)
	      && SYMBOL_PRINT_NAME (BLOCK_FUNCTION (b)))
	    error (_("No frame is currently executing in block %s."),
		   SYMBOL_PRINT_NAME (BLOCK_FUNCTION (b)));
	  else
	    error (_("No frame is currently executing in specified block"));
	}
    }

  return read_var_value (var, frame);
}

struct value *
address_of_variable (struct symbol *var, const struct block *b)
{
  struct type *type = SYMBOL_TYPE (var);
  struct value *val;

  /* Evaluate it first; if the result is a memory address, we're fine.
     Lazy evaluation pays off here.  */

  val = value_of_variable (var, b);

  if ((VALUE_LVAL (val) == lval_memory && value_lazy (val))
      || TYPE_CODE (type) == TYPE_CODE_FUNC)
    {
      CORE_ADDR addr = value_address (val);

      return value_from_pointer (lookup_pointer_type (type), addr);
    }

  /* Not a memory address; check what the problem was.  */
  switch (VALUE_LVAL (val))
    {
    case lval_register:
      {
	struct frame_info *frame;
	const char *regname;

	frame = frame_find_by_id (VALUE_FRAME_ID (val));
	gdb_assert (frame);

	regname = gdbarch_register_name (get_frame_arch (frame),
					 VALUE_REGNUM (val));
	gdb_assert (regname && *regname);

	error (_("Address requested for identifier "
		 "\"%s\" which is in register $%s"),
	       SYMBOL_PRINT_NAME (var), regname);
	break;
      }

    default:
      error (_("Can't take address of \"%s\" which isn't an lvalue."),
	     SYMBOL_PRINT_NAME (var));
      break;
    }

  return val;
}

/* Return one if VAL does not live in target memory, but should in order
   to operate on it.  Otherwise return zero.  */

int
value_must_coerce_to_target (struct value *val)
{
  struct type *valtype;

  /* The only lval kinds which do not live in target memory.  */
  if (VALUE_LVAL (val) != not_lval
      && VALUE_LVAL (val) != lval_internalvar)
    return 0;

  valtype = check_typedef (value_type (val));

  switch (TYPE_CODE (valtype))
    {
    case TYPE_CODE_ARRAY:
      return TYPE_VECTOR (valtype) ? 0 : 1;
    case TYPE_CODE_STRING:
      return 1;
    default:
      return 0;
    }
}

/* Make sure that VAL lives in target memory if it's supposed to.  For
   instance, strings are constructed as character arrays in GDB's
   storage, and this function copies them to the target.  */

struct value *
value_coerce_to_target (struct value *val)
{
  LONGEST length;
  CORE_ADDR addr;

  if (!value_must_coerce_to_target (val))
    return val;

  length = TYPE_LENGTH (check_typedef (value_type (val)));
  addr = allocate_space_in_inferior (length);
  write_memory (addr, value_contents (val), length);
  return value_at_lazy (value_type (val), addr);
}

/* Given a value which is an array, return a value which is a pointer
   to its first element, regardless of whether or not the array has a
   nonzero lower bound.

   FIXME: A previous comment here indicated that this routine should
   be substracting the array's lower bound.  It's not clear to me that
   this is correct.  Given an array subscripting operation, it would
   certainly work to do the adjustment here, essentially computing:

   (&array[0] - (lowerbound * sizeof array[0])) + (index * sizeof array[0])

   However I believe a more appropriate and logical place to account
   for the lower bound is to do so in value_subscript, essentially
   computing:

   (&array[0] + ((index - lowerbound) * sizeof array[0]))

   As further evidence consider what would happen with operations
   other than array subscripting, where the caller would get back a
   value that had an address somewhere before the actual first element
   of the array, and the information about the lower bound would be
   lost because of the coercion to pointer type.  */

struct value *
value_coerce_array (struct value *arg1)
{
  struct type *type = check_typedef (value_type (arg1));

  /* If the user tries to do something requiring a pointer with an
     array that has not yet been pushed to the target, then this would
     be a good time to do so.  */
  arg1 = value_coerce_to_target (arg1);

  if (VALUE_LVAL (arg1) != lval_memory)
    error (_("Attempt to take address of value not located in memory."));

  return value_from_pointer (lookup_pointer_type (TYPE_TARGET_TYPE (type)),
			     value_address (arg1));
}

/* Given a value which is a function, return a value which is a pointer
   to it.  */

struct value *
value_coerce_function (struct value *arg1)
{
  struct value *retval;

  if (VALUE_LVAL (arg1) != lval_memory)
    error (_("Attempt to take address of value not located in memory."));

  retval = value_from_pointer (lookup_pointer_type (value_type (arg1)),
			       value_address (arg1));
  return retval;
}

/* Return a pointer value for the object for which ARG1 is the
   contents.  */

struct value *
value_addr (struct value *arg1)
{
  struct value *arg2;
  struct type *type = check_typedef (value_type (arg1));

  if (TYPE_CODE (type) == TYPE_CODE_REF)
    {
      /* Copy the value, but change the type from (T&) to (T*).  We
         keep the same location information, which is efficient, and
         allows &(&X) to get the location containing the reference.  */
      arg2 = value_copy (arg1);
      deprecated_set_value_type (arg2, 
				 lookup_pointer_type (TYPE_TARGET_TYPE (type)));
      return arg2;
    }
  if (TYPE_CODE (type) == TYPE_CODE_FUNC)
    return value_coerce_function (arg1);

  /* If this is an array that has not yet been pushed to the target,
     then this would be a good time to force it to memory.  */
  arg1 = value_coerce_to_target (arg1);

  if (VALUE_LVAL (arg1) != lval_memory)
    error (_("Attempt to take address of value not located in memory."));

  /* Get target memory address.  */
  arg2 = value_from_pointer (lookup_pointer_type (value_type (arg1)),
			     (value_address (arg1)
			      + value_embedded_offset (arg1)));

  /* This may be a pointer to a base subobject; so remember the
     full derived object's type ...  */
  set_value_enclosing_type (arg2,
			    lookup_pointer_type (value_enclosing_type (arg1)));
  /* ... and also the relative position of the subobject in the full
     object.  */
  set_value_pointed_to_offset (arg2, value_embedded_offset (arg1));
  return arg2;
}

/* Return a reference value for the object for which ARG1 is the
   contents.  */

struct value *
value_ref (struct value *arg1)
{
  struct value *arg2;
  struct type *type = check_typedef (value_type (arg1));

  if (TYPE_CODE (type) == TYPE_CODE_REF)
    return arg1;

  arg2 = value_addr (arg1);
  deprecated_set_value_type (arg2, lookup_reference_type (type));
  return arg2;
}

/* Given a value of a pointer type, apply the C unary * operator to
   it.  */

struct value *
value_ind (struct value *arg1)
{
  struct type *base_type;
  struct value *arg2;

  arg1 = coerce_array (arg1);

  base_type = check_typedef (value_type (arg1));

  if (VALUE_LVAL (arg1) == lval_computed)
    {
      const struct lval_funcs *funcs = value_computed_funcs (arg1);

      if (funcs->indirect)
	{
	  struct value *result = funcs->indirect (arg1);

	  if (result)
	    return result;
	}
    }

  if (TYPE_CODE (base_type) == TYPE_CODE_PTR)
    {
      struct type *enc_type;

      /* We may be pointing to something embedded in a larger object.
         Get the real type of the enclosing object.  */
      enc_type = check_typedef (value_enclosing_type (arg1));
      enc_type = TYPE_TARGET_TYPE (enc_type);

      if (TYPE_CODE (check_typedef (enc_type)) == TYPE_CODE_FUNC
	  || TYPE_CODE (check_typedef (enc_type)) == TYPE_CODE_METHOD)
	/* For functions, go through find_function_addr, which knows
	   how to handle function descriptors.  */
	arg2 = value_at_lazy (enc_type, 
			      find_function_addr (arg1, NULL));
      else
	/* Retrieve the enclosing object pointed to.  */
	arg2 = value_at_lazy (enc_type, 
			      (value_as_address (arg1)
			       - value_pointed_to_offset (arg1)));

      return readjust_indirect_value_type (arg2, enc_type, base_type, arg1);
    }

  error (_("Attempt to take contents of a non-pointer value."));
  return 0;			/* For lint -- never reached.  */
}

/* Create a value for an array by allocating space in GDB, copying the
   data into that space, and then setting up an array value.

   The array bounds are set from LOWBOUND and HIGHBOUND, and the array
   is populated from the values passed in ELEMVEC.

   The element type of the array is inherited from the type of the
   first element, and all elements must have the same size (though we
   don't currently enforce any restriction on their types).  */

struct value *
value_array (int lowbound, int highbound, struct value **elemvec)
{
  int nelem;
  int idx;
  unsigned int typelength;
  struct value *val;
  struct type *arraytype;

  /* Validate that the bounds are reasonable and that each of the
     elements have the same size.  */

  nelem = highbound - lowbound + 1;
  if (nelem <= 0)
    {
      error (_("bad array bounds (%d, %d)"), lowbound, highbound);
    }
  typelength = TYPE_LENGTH (value_enclosing_type (elemvec[0]));
  for (idx = 1; idx < nelem; idx++)
    {
      if (TYPE_LENGTH (value_enclosing_type (elemvec[idx])) != typelength)
	{
	  error (_("array elements must all be the same size"));
	}
    }

  arraytype = lookup_array_range_type (value_enclosing_type (elemvec[0]),
				       lowbound, highbound);

  if (!current_language->c_style_arrays)
    {
      val = allocate_value (arraytype);
      for (idx = 0; idx < nelem; idx++)
	value_contents_copy (val, idx * typelength, elemvec[idx], 0,
			     typelength);
      return val;
    }

  /* Allocate space to store the array, and then initialize it by
     copying in each element.  */

  val = allocate_value (arraytype);
  for (idx = 0; idx < nelem; idx++)
    value_contents_copy (val, idx * typelength, elemvec[idx], 0, typelength);
  return val;
}

struct value *
value_cstring (char *ptr, ssize_t len, struct type *char_type)
{
  struct value *val;
  int lowbound = current_language->string_lower_bound;
  ssize_t highbound = len / TYPE_LENGTH (char_type);
  struct type *stringtype
    = lookup_array_range_type (char_type, lowbound, highbound + lowbound - 1);

  val = allocate_value (stringtype);
  memcpy (value_contents_raw (val), ptr, len);
  return val;
}

/* Create a value for a string constant by allocating space in the
   inferior, copying the data into that space, and returning the
   address with type TYPE_CODE_STRING.  PTR points to the string
   constant data; LEN is number of characters.

   Note that string types are like array of char types with a lower
   bound of zero and an upper bound of LEN - 1.  Also note that the
   string may contain embedded null bytes.  */

struct value *
value_string (char *ptr, ssize_t len, struct type *char_type)
{
  struct value *val;
  int lowbound = current_language->string_lower_bound;
  ssize_t highbound = len / TYPE_LENGTH (char_type);
  struct type *stringtype
    = lookup_string_range_type (char_type, lowbound, highbound + lowbound - 1);

  val = allocate_value (stringtype);
  memcpy (value_contents_raw (val), ptr, len);
  return val;
}


/* See if we can pass arguments in T2 to a function which takes
   arguments of types T1.  T1 is a list of NARGS arguments, and T2 is
   a NULL-terminated vector.  If some arguments need coercion of some
   sort, then the coerced values are written into T2.  Return value is
   0 if the arguments could be matched, or the position at which they
   differ if not.

   STATICP is nonzero if the T1 argument list came from a static
   member function.  T2 will still include the ``this'' pointer, but
   it will be skipped.

   For non-static member functions, we ignore the first argument,
   which is the type of the instance variable.  This is because we
   want to handle calls with objects from derived classes.  This is
   not entirely correct: we should actually check to make sure that a
   requested operation is type secure, shouldn't we?  FIXME.  */

static int
typecmp (int staticp, int varargs, int nargs,
	 struct field t1[], struct value *t2[])
{
  int i;

  if (t2 == 0)
    internal_error (__FILE__, __LINE__, 
		    _("typecmp: no argument list"));

  /* Skip ``this'' argument if applicable.  T2 will always include
     THIS.  */
  if (staticp)
    t2 ++;

  for (i = 0;
       (i < nargs) && TYPE_CODE (t1[i].type) != TYPE_CODE_VOID;
       i++)
    {
      struct type *tt1, *tt2;

      if (!t2[i])
	return i + 1;

      tt1 = check_typedef (t1[i].type);
      tt2 = check_typedef (value_type (t2[i]));

      if (TYPE_CODE (tt1) == TYPE_CODE_REF
      /* We should be doing hairy argument matching, as below.  */
	  && (TYPE_CODE (check_typedef (TYPE_TARGET_TYPE (tt1)))
	      == TYPE_CODE (tt2)))
	{
	  if (TYPE_CODE (tt2) == TYPE_CODE_ARRAY)
	    t2[i] = value_coerce_array (t2[i]);
	  else
	    t2[i] = value_ref (t2[i]);
	  continue;
	}

      /* djb - 20000715 - Until the new type structure is in the
	 place, and we can attempt things like implicit conversions,
	 we need to do this so you can take something like a map<const
	 char *>, and properly access map["hello"], because the
	 argument to [] will be a reference to a pointer to a char,
	 and the argument will be a pointer to a char.  */
      while (TYPE_CODE(tt1) == TYPE_CODE_REF
	     || TYPE_CODE (tt1) == TYPE_CODE_PTR)
	{
	  tt1 = check_typedef( TYPE_TARGET_TYPE(tt1) );
	}
      while (TYPE_CODE(tt2) == TYPE_CODE_ARRAY
	     || TYPE_CODE(tt2) == TYPE_CODE_PTR
	     || TYPE_CODE(tt2) == TYPE_CODE_REF)
	{
	  tt2 = check_typedef (TYPE_TARGET_TYPE(tt2));
	}
      if (TYPE_CODE (tt1) == TYPE_CODE (tt2))
	continue;
      /* Array to pointer is a `trivial conversion' according to the
	 ARM.  */

      /* We should be doing much hairier argument matching (see
         section 13.2 of the ARM), but as a quick kludge, just check
         for the same type code.  */
      if (TYPE_CODE (t1[i].type) != TYPE_CODE (value_type (t2[i])))
	return i + 1;
    }
  if (varargs || t2[i] == NULL)
    return 0;
  return i + 1;
}

/* Helper class for do_search_struct_field that updates *RESULT_PTR
   and *LAST_BOFFSET, and possibly throws an exception if the field
   search has yielded ambiguous results.  */

static void
update_search_result (struct value **result_ptr, struct value *v,
		      int *last_boffset, int boffset,
		      const char *name, struct type *type)
{
  if (v != NULL)
    {
      if (*result_ptr != NULL
	  /* The result is not ambiguous if all the classes that are
	     found occupy the same space.  */
	  && *last_boffset != boffset)
	error (_("base class '%s' is ambiguous in type '%s'"),
	       name, TYPE_SAFE_NAME (type));
      *result_ptr = v;
      *last_boffset = boffset;
    }
}

/* A helper for search_struct_field.  This does all the work; most
   arguments are as passed to search_struct_field.  The result is
   stored in *RESULT_PTR, which must be initialized to NULL.
   OUTERMOST_TYPE is the type of the initial type passed to
   search_struct_field; this is used for error reporting when the
   lookup is ambiguous.  */

static void
do_search_struct_field (const char *name, struct value *arg1, int offset,
			struct type *type, int looking_for_baseclass,
			struct value **result_ptr,
			int *last_boffset,
			struct type *outermost_type)
{
  int i;
  int nbases;

  CHECK_TYPEDEF (type);
  nbases = TYPE_N_BASECLASSES (type);

  if (!looking_for_baseclass)
    for (i = TYPE_NFIELDS (type) - 1; i >= nbases; i--)
      {
	const char *t_field_name = TYPE_FIELD_NAME (type, i);

	if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	  {
	    struct value *v;

	    if (field_is_static (&TYPE_FIELD (type, i)))
	      {
		v = value_static_field (type, i);
		if (v == 0)
		  error (_("field %s is nonexistent or "
			   "has been optimized out"),
			 name);
	      }
	    else
	      v = value_primitive_field (arg1, offset, i, type);
	    *result_ptr = v;
	    return;
	  }

	if (t_field_name
	    && (t_field_name[0] == '\0'
		|| (TYPE_CODE (type) == TYPE_CODE_UNION
		    && (strcmp_iw (t_field_name, "else") == 0))))
	  {
	    struct type *field_type = TYPE_FIELD_TYPE (type, i);

	    if (TYPE_CODE (field_type) == TYPE_CODE_UNION
		|| TYPE_CODE (field_type) == TYPE_CODE_STRUCT)
	      {
		/* Look for a match through the fields of an anonymous
		   union, or anonymous struct.  C++ provides anonymous
		   unions.

		   In the GNU Chill (now deleted from GDB)
		   implementation of variant record types, each
		   <alternative field> has an (anonymous) union type,
		   each member of the union represents a <variant
		   alternative>.  Each <variant alternative> is
		   represented as a struct, with a member for each
		   <variant field>.  */

		struct value *v = NULL;
		int new_offset = offset;

		/* This is pretty gross.  In G++, the offset in an
		   anonymous union is relative to the beginning of the
		   enclosing struct.  In the GNU Chill (now deleted
		   from GDB) implementation of variant records, the
		   bitpos is zero in an anonymous union field, so we
		   have to add the offset of the union here.  */
		if (TYPE_CODE (field_type) == TYPE_CODE_STRUCT
		    || (TYPE_NFIELDS (field_type) > 0
			&& TYPE_FIELD_BITPOS (field_type, 0) == 0))
		  new_offset += TYPE_FIELD_BITPOS (type, i) / 8;

		do_search_struct_field (name, arg1, new_offset, 
					field_type,
					looking_for_baseclass, &v,
					last_boffset,
					outermost_type);
		if (v)
		  {
		    *result_ptr = v;
		    return;
		  }
	      }
	  }
      }

  for (i = 0; i < nbases; i++)
    {
      struct value *v = NULL;
      struct type *basetype = check_typedef (TYPE_BASECLASS (type, i));
      /* If we are looking for baseclasses, this is what we get when
         we hit them.  But it could happen that the base part's member
         name is not yet filled in.  */
      int found_baseclass = (looking_for_baseclass
			     && TYPE_BASECLASS_NAME (type, i) != NULL
			     && (strcmp_iw (name, 
					    TYPE_BASECLASS_NAME (type, 
								 i)) == 0));
      int boffset = value_embedded_offset (arg1) + offset;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  struct value *v2;

	  boffset = baseclass_offset (type, i,
				      value_contents_for_printing (arg1),
				      value_embedded_offset (arg1) + offset,
				      value_address (arg1),
				      arg1);

	  /* The virtual base class pointer might have been clobbered
	     by the user program.  Make sure that it still points to a
	     valid memory location.  */

	  boffset += value_embedded_offset (arg1) + offset;
	  if (boffset < 0
	      || boffset >= TYPE_LENGTH (value_enclosing_type (arg1)))
	    {
	      CORE_ADDR base_addr;

	      v2  = allocate_value (basetype);
	      base_addr = value_address (arg1) + boffset;
	      if (target_read_memory (base_addr, 
				      value_contents_raw (v2),
				      TYPE_LENGTH (basetype)) != 0)
		error (_("virtual baseclass botch"));
	      VALUE_LVAL (v2) = lval_memory;
	      set_value_address (v2, base_addr);
	    }
	  else
	    {
	      v2 = value_copy (arg1);
	      deprecated_set_value_type (v2, basetype);
	      set_value_embedded_offset (v2, boffset);
	    }

	  if (found_baseclass)
	    v = v2;
	  else
	    {
	      do_search_struct_field (name, v2, 0,
				      TYPE_BASECLASS (type, i),
				      looking_for_baseclass,
				      result_ptr, last_boffset,
				      outermost_type);
	    }
	}
      else if (found_baseclass)
	v = value_primitive_field (arg1, offset, i, type);
      else
	{
	  do_search_struct_field (name, arg1,
				  offset + TYPE_BASECLASS_BITPOS (type, 
								  i) / 8,
				  basetype, looking_for_baseclass,
				  result_ptr, last_boffset,
				  outermost_type);
	}

      update_search_result (result_ptr, v, last_boffset,
			    boffset, name, outermost_type);
    }
}

/* Helper function used by value_struct_elt to recurse through
   baseclasses.  Look for a field NAME in ARG1.  Adjust the address of
   ARG1 by OFFSET bytes, and search in it assuming it has (class) type
   TYPE.  If found, return value, else return NULL.

   If LOOKING_FOR_BASECLASS, then instead of looking for struct
   fields, look for a baseclass named NAME.  */

static struct value *
search_struct_field (const char *name, struct value *arg1, int offset,
		     struct type *type, int looking_for_baseclass)
{
  struct value *result = NULL;
  int boffset = 0;

  do_search_struct_field (name, arg1, offset, type, looking_for_baseclass,
			  &result, &boffset, type);
  return result;
}

/* Helper function used by value_struct_elt to recurse through
   baseclasses.  Look for a field NAME in ARG1.  Adjust the address of
   ARG1 by OFFSET bytes, and search in it assuming it has (class) type
   TYPE.

   If found, return value, else if name matched and args not return
   (value) -1, else return NULL.  */

static struct value *
search_struct_method (const char *name, struct value **arg1p,
		      struct value **args, int offset,
		      int *static_memfuncp, struct type *type)
{
  int i;
  struct value *v;
  int name_matched = 0;
  char dem_opname[64];

  CHECK_TYPEDEF (type);
  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; i--)
    {
      const char *t_field_name = TYPE_FN_FIELDLIST_NAME (type, i);

      /* FIXME!  May need to check for ARM demangling here.  */
      if (strncmp (t_field_name, "__", 2) == 0 ||
	  strncmp (t_field_name, "op", 2) == 0 ||
	  strncmp (t_field_name, "type", 4) == 0)
	{
	  if (cplus_demangle_opname (t_field_name, dem_opname, DMGL_ANSI))
	    t_field_name = dem_opname;
	  else if (cplus_demangle_opname (t_field_name, dem_opname, 0))
	    t_field_name = dem_opname;
	}
      if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	{
	  int j = TYPE_FN_FIELDLIST_LENGTH (type, i) - 1;
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);

	  name_matched = 1;
	  check_stub_method_group (type, i);
	  if (j > 0 && args == 0)
	    error (_("cannot resolve overloaded method "
		     "`%s': no arguments supplied"), name);
	  else if (j == 0 && args == 0)
	    {
	      v = value_fn_field (arg1p, f, j, type, offset);
	      if (v != NULL)
		return v;
	    }
	  else
	    while (j >= 0)
	      {
		if (!typecmp (TYPE_FN_FIELD_STATIC_P (f, j),
			      TYPE_VARARGS (TYPE_FN_FIELD_TYPE (f, j)),
			      TYPE_NFIELDS (TYPE_FN_FIELD_TYPE (f, j)),
			      TYPE_FN_FIELD_ARGS (f, j), args))
		  {
		    if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		      return value_virtual_fn_field (arg1p, f, j, 
						     type, offset);
		    if (TYPE_FN_FIELD_STATIC_P (f, j) 
			&& static_memfuncp)
		      *static_memfuncp = 1;
		    v = value_fn_field (arg1p, f, j, type, offset);
		    if (v != NULL)
		      return v;       
		  }
		j--;
	      }
	}
    }

  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    {
      int base_offset;
      int this_offset;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
	  struct value *base_val;
	  const gdb_byte *base_valaddr;

	  /* The virtual base class pointer might have been
	     clobbered by the user program.  Make sure that it
	    still points to a valid memory location.  */

	  if (offset < 0 || offset >= TYPE_LENGTH (type))
	    {
	      gdb_byte *tmp;
	      struct cleanup *back_to;
	      CORE_ADDR address;

	      tmp = xmalloc (TYPE_LENGTH (baseclass));
	      back_to = make_cleanup (xfree, tmp);
	      address = value_address (*arg1p);

	      if (target_read_memory (address + offset,
				      tmp, TYPE_LENGTH (baseclass)) != 0)
		error (_("virtual baseclass botch"));

	      base_val = value_from_contents_and_address (baseclass,
							  tmp,
							  address + offset);
	      base_valaddr = value_contents_for_printing (base_val);
	      this_offset = 0;
	      do_cleanups (back_to);
	    }
	  else
	    {
	      base_val = *arg1p;
	      base_valaddr = value_contents_for_printing (*arg1p);
	      this_offset = offset;
	    }

	  base_offset = baseclass_offset (type, i, base_valaddr,
					  this_offset, value_address (base_val),
					  base_val);
	}
      else
	{
	  base_offset = TYPE_BASECLASS_BITPOS (type, i) / 8;
	}
      v = search_struct_method (name, arg1p, args, base_offset + offset,
				static_memfuncp, TYPE_BASECLASS (type, i));
      if (v == (struct value *) - 1)
	{
	  name_matched = 1;
	}
      else if (v)
	{
	  /* FIXME-bothner:  Why is this commented out?  Why is it here?  */
	  /* *arg1p = arg1_tmp; */
	  return v;
	}
    }
  if (name_matched)
    return (struct value *) - 1;
  else
    return NULL;
}

/* Given *ARGP, a value of type (pointer to a)* structure/union,
   extract the component named NAME from the ultimate target
   structure/union and return it as a value with its appropriate type.
   ERR is used in the error message if *ARGP's type is wrong.

   C++: ARGS is a list of argument types to aid in the selection of
   an appropriate method.  Also, handle derived types.

   STATIC_MEMFUNCP, if non-NULL, points to a caller-supplied location
   where the truthvalue of whether the function that was resolved was
   a static member function or not is stored.

   ERR is an error message to be printed in case the field is not
   found.  */

struct value *
value_struct_elt (struct value **argp, struct value **args,
		  const char *name, int *static_memfuncp, const char *err)
{
  struct type *t;
  struct value *v;

  *argp = coerce_array (*argp);

  t = check_typedef (value_type (*argp));

  /* Follow pointers until we get to a non-pointer.  */

  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      *argp = value_ind (*argp);
      /* Don't coerce fn pointer to fn and then back again!  */
      if (TYPE_CODE (value_type (*argp)) != TYPE_CODE_FUNC)
	*argp = coerce_array (*argp);
      t = check_typedef (value_type (*argp));
    }

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error (_("Attempt to extract a component of a value that is not a %s."),
	   err);

  /* Assume it's not, unless we see that it is.  */
  if (static_memfuncp)
    *static_memfuncp = 0;

  if (!args)
    {
      /* if there are no arguments ...do this...  */

      /* Try as a field first, because if we succeed, there is less
         work to be done.  */
      v = search_struct_field (name, *argp, 0, t, 0);
      if (v)
	return v;

      /* C++: If it was not found as a data field, then try to
         return it as a pointer to a method.  */
      v = search_struct_method (name, argp, args, 0, 
				static_memfuncp, t);

      if (v == (struct value *) - 1)
	error (_("Cannot take address of method %s."), name);
      else if (v == 0)
	{
	  if (TYPE_NFN_FIELDS (t))
	    error (_("There is no member or method named %s."), name);
	  else
	    error (_("There is no member named %s."), name);
	}
      return v;
    }

    v = search_struct_method (name, argp, args, 0, 
			      static_memfuncp, t);
  
  if (v == (struct value *) - 1)
    {
      error (_("One of the arguments you tried to pass to %s could not "
	       "be converted to what the function wants."), name);
    }
  else if (v == 0)
    {
      /* See if user tried to invoke data as function.  If so, hand it
         back.  If it's not callable (i.e., a pointer to function),
         gdb should give an error.  */
      v = search_struct_field (name, *argp, 0, t, 0);
      /* If we found an ordinary field, then it is not a method call.
	 So, treat it as if it were a static member function.  */
      if (v && static_memfuncp)
	*static_memfuncp = 1;
    }

  if (!v)
    throw_error (NOT_FOUND_ERROR,
                 _("Structure has no component named %s."), name);
  return v;
}

/* Search through the methods of an object (and its bases) to find a
   specified method.  Return the pointer to the fn_field list of
   overloaded instances.

   Helper function for value_find_oload_list.
   ARGP is a pointer to a pointer to a value (the object).
   METHOD is a string containing the method name.
   OFFSET is the offset within the value.
   TYPE is the assumed type of the object.
   NUM_FNS is the number of overloaded instances.
   BASETYPE is set to the actual type of the subobject where the
      method is found.
   BOFFSET is the offset of the base subobject where the method is found.  */

static struct fn_field *
find_method_list (struct value **argp, const char *method,
		  int offset, struct type *type, int *num_fns,
		  struct type **basetype, int *boffset)
{
  int i;
  struct fn_field *f;
  CHECK_TYPEDEF (type);

  *num_fns = 0;

  /* First check in object itself.  */
  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; i--)
    {
      /* pai: FIXME What about operators and type conversions?  */
      const char *fn_field_name = TYPE_FN_FIELDLIST_NAME (type, i);

      if (fn_field_name && (strcmp_iw (fn_field_name, method) == 0))
	{
	  int len = TYPE_FN_FIELDLIST_LENGTH (type, i);
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);

	  *num_fns = len;
	  *basetype = type;
	  *boffset = offset;

	  /* Resolve any stub methods.  */
	  check_stub_method_group (type, i);

	  return f;
	}
    }

  /* Not found in object, check in base subobjects.  */
  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    {
      int base_offset;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  base_offset = baseclass_offset (type, i,
					  value_contents_for_printing (*argp),
					  value_offset (*argp) + offset,
					  value_address (*argp), *argp);
	}
      else /* Non-virtual base, simply use bit position from debug
	      info.  */
	{
	  base_offset = TYPE_BASECLASS_BITPOS (type, i) / 8;
	}
      f = find_method_list (argp, method, base_offset + offset,
			    TYPE_BASECLASS (type, i), num_fns, 
			    basetype, boffset);
      if (f)
	return f;
    }
  return NULL;
}

/* Return the list of overloaded methods of a specified name.

   ARGP is a pointer to a pointer to a value (the object).
   METHOD is the method name.
   OFFSET is the offset within the value contents.
   NUM_FNS is the number of overloaded instances.
   BASETYPE is set to the type of the base subobject that defines the
      method.
   BOFFSET is the offset of the base subobject which defines the method.  */

static struct fn_field *
value_find_oload_method_list (struct value **argp, const char *method,
			      int offset, int *num_fns, 
			      struct type **basetype, int *boffset)
{
  struct type *t;

  t = check_typedef (value_type (*argp));

  /* Code snarfed from value_struct_elt.  */
  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      *argp = value_ind (*argp);
      /* Don't coerce fn pointer to fn and then back again!  */
      if (TYPE_CODE (value_type (*argp)) != TYPE_CODE_FUNC)
	*argp = coerce_array (*argp);
      t = check_typedef (value_type (*argp));
    }

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error (_("Attempt to extract a component of a "
	     "value that is not a struct or union"));

  return find_method_list (argp, method, 0, t, num_fns, 
			   basetype, boffset);
}

/* Given an array of arguments (ARGS) (which includes an
   entry for "this" in the case of C++ methods), the number of
   arguments NARGS, the NAME of a function, and whether it's a method or
   not (METHOD), find the best function that matches on the argument types
   according to the overload resolution rules.

   METHOD can be one of three values:
     NON_METHOD for non-member functions.
     METHOD: for member functions.
     BOTH: used for overload resolution of operators where the
       candidates are expected to be either member or non member
       functions.  In this case the first argument ARGTYPES
       (representing 'this') is expected to be a reference to the
       target object, and will be dereferenced when attempting the
       non-member search.

   In the case of class methods, the parameter OBJ is an object value
   in which to search for overloaded methods.

   In the case of non-method functions, the parameter FSYM is a symbol
   corresponding to one of the overloaded functions.

   Return value is an integer: 0 -> good match, 10 -> debugger applied
   non-standard coercions, 100 -> incompatible.

   If a method is being searched for, VALP will hold the value.
   If a non-method is being searched for, SYMP will hold the symbol 
   for it.

   If a method is being searched for, and it is a static method,
   then STATICP will point to a non-zero value.

   If NO_ADL argument dependent lookup is disabled.  This is used to prevent
   ADL overload candidates when performing overload resolution for a fully
   qualified name.

   Note: This function does *not* check the value of
   overload_resolution.  Caller must check it to see whether overload
   resolution is permitted.  */

int
find_overload_match (struct value **args, int nargs,
		     const char *name, enum oload_search_type method,
		     struct value **objp, struct symbol *fsym,
		     struct value **valp, struct symbol **symp, 
		     int *staticp, const int no_adl)
{
  struct value *obj = (objp ? *objp : NULL);
  struct type *obj_type = obj ? value_type (obj) : NULL;
  /* Index of best overloaded function.  */
  int func_oload_champ = -1;
  int method_oload_champ = -1;

  /* The measure for the current best match.  */
  struct badness_vector *method_badness = NULL;
  struct badness_vector *func_badness = NULL;

  struct value *temp = obj;
  /* For methods, the list of overloaded methods.  */
  struct fn_field *fns_ptr = NULL;
  /* For non-methods, the list of overloaded function symbols.  */
  struct symbol **oload_syms = NULL;
  /* Number of overloaded instances being considered.  */
  int num_fns = 0;
  struct type *basetype = NULL;
  int boffset;

  struct cleanup *all_cleanups = make_cleanup (null_cleanup, NULL);

  const char *obj_type_name = NULL;
  const char *func_name = NULL;
  enum oload_classification match_quality;
  enum oload_classification method_match_quality = INCOMPATIBLE;
  enum oload_classification func_match_quality = INCOMPATIBLE;

  /* Get the list of overloaded methods or functions.  */
  if (method == METHOD || method == BOTH)
    {
      gdb_assert (obj);

      /* OBJ may be a pointer value rather than the object itself.  */
      obj = coerce_ref (obj);
      while (TYPE_CODE (check_typedef (value_type (obj))) == TYPE_CODE_PTR)
	obj = coerce_ref (value_ind (obj));
      obj_type_name = TYPE_NAME (value_type (obj));

      /* First check whether this is a data member, e.g. a pointer to
	 a function.  */
      if (TYPE_CODE (check_typedef (value_type (obj))) == TYPE_CODE_STRUCT)
	{
	  *valp = search_struct_field (name, obj, 0,
				       check_typedef (value_type (obj)), 0);
	  if (*valp)
	    {
	      *staticp = 1;
	      do_cleanups (all_cleanups);
	      return 0;
	    }
	}

      /* Retrieve the list of methods with the name NAME.  */
      fns_ptr = value_find_oload_method_list (&temp, name, 
					      0, &num_fns, 
					      &basetype, &boffset);
      /* If this is a method only search, and no methods were found
         the search has faild.  */
      if (method == METHOD && (!fns_ptr || !num_fns))
	error (_("Couldn't find method %s%s%s"),
	       obj_type_name,
	       (obj_type_name && *obj_type_name) ? "::" : "",
	       name);
      /* If we are dealing with stub method types, they should have
	 been resolved by find_method_list via
	 value_find_oload_method_list above.  */
      if (fns_ptr)
	{
	  gdb_assert (TYPE_DOMAIN_TYPE (fns_ptr[0].type) != NULL);
	  method_oload_champ = find_oload_champ (args, nargs, method,
	                                         num_fns, fns_ptr,
	                                         oload_syms, &method_badness);

	  method_match_quality =
	      classify_oload_match (method_badness, nargs,
	                            oload_method_static (method, fns_ptr,
	                                                 method_oload_champ));

	  make_cleanup (xfree, method_badness);
	}

    }

  if (method == NON_METHOD || method == BOTH)
    {
      const char *qualified_name = NULL;

      /* If the overload match is being search for both as a method
         and non member function, the first argument must now be
         dereferenced.  */
      if (method == BOTH)
	args[0] = value_ind (args[0]);

      if (fsym)
        {
          qualified_name = SYMBOL_NATURAL_NAME (fsym);

          /* If we have a function with a C++ name, try to extract just
	     the function part.  Do not try this for non-functions (e.g.
	     function pointers).  */
          if (qualified_name
              && TYPE_CODE (check_typedef (SYMBOL_TYPE (fsym)))
	      == TYPE_CODE_FUNC)
            {
	      char *temp;

	      temp = cp_func_name (qualified_name);

	      /* If cp_func_name did not remove anything, the name of the
	         symbol did not include scope or argument types - it was
	         probably a C-style function.  */
	      if (temp)
		{
		  make_cleanup (xfree, temp);
		  if (strcmp (temp, qualified_name) == 0)
		    func_name = NULL;
		  else
		    func_name = temp;
		}
            }
        }
      else
	{
	  func_name = name;
	  qualified_name = name;
	}

      /* If there was no C++ name, this must be a C-style function or
	 not a function at all.  Just return the same symbol.  Do the
	 same if cp_func_name fails for some reason.  */
      if (func_name == NULL)
        {
	  *symp = fsym;
	  do_cleanups (all_cleanups);
          return 0;
        }

      func_oload_champ = find_oload_champ_namespace (args, nargs,
                                                     func_name,
                                                     qualified_name,
                                                     &oload_syms,
                                                     &func_badness,
                                                     no_adl);

      if (func_oload_champ >= 0)
	func_match_quality = classify_oload_match (func_badness, nargs, 0);

      make_cleanup (xfree, oload_syms);
      make_cleanup (xfree, func_badness);
    }

  /* Did we find a match ?  */
  if (method_oload_champ == -1 && func_oload_champ == -1)
    throw_error (NOT_FOUND_ERROR,
                 _("No symbol \"%s\" in current context."),
                 name);

  /* If we have found both a method match and a function
     match, find out which one is better, and calculate match
     quality.  */
  if (method_oload_champ >= 0 && func_oload_champ >= 0)
    {
      switch (compare_badness (func_badness, method_badness))
        {
	  case 0: /* Top two contenders are equally good.  */
	    /* FIXME: GDB does not support the general ambiguous case.
	     All candidates should be collected and presented the
	     user.  */
	    error (_("Ambiguous overload resolution"));
	    break;
	  case 1: /* Incomparable top contenders.  */
	    /* This is an error incompatible candidates
	       should not have been proposed.  */
	    error (_("Internal error: incompatible "
		     "overload candidates proposed"));
	    break;
	  case 2: /* Function champion.  */
	    method_oload_champ = -1;
	    match_quality = func_match_quality;
	    break;
	  case 3: /* Method champion.  */
	    func_oload_champ = -1;
	    match_quality = method_match_quality;
	    break;
	  default:
	    error (_("Internal error: unexpected overload comparison result"));
	    break;
        }
    }
  else
    {
      /* We have either a method match or a function match.  */
      if (method_oload_champ >= 0)
	match_quality = method_match_quality;
      else
	match_quality = func_match_quality;
    }

  if (match_quality == INCOMPATIBLE)
    {
      if (method == METHOD)
	error (_("Cannot resolve method %s%s%s to any overloaded instance"),
	       obj_type_name,
	       (obj_type_name && *obj_type_name) ? "::" : "",
	       name);
      else
	error (_("Cannot resolve function %s to any overloaded instance"),
	       func_name);
    }
  else if (match_quality == NON_STANDARD)
    {
      if (method == METHOD)
	warning (_("Using non-standard conversion to match "
		   "method %s%s%s to supplied arguments"),
		 obj_type_name,
		 (obj_type_name && *obj_type_name) ? "::" : "",
		 name);
      else
	warning (_("Using non-standard conversion to match "
		   "function %s to supplied arguments"),
		 func_name);
    }

  if (staticp != NULL)
    *staticp = oload_method_static (method, fns_ptr, method_oload_champ);

  if (method_oload_champ >= 0)
    {
      if (TYPE_FN_FIELD_VIRTUAL_P (fns_ptr, method_oload_champ))
	*valp = value_virtual_fn_field (&temp, fns_ptr, method_oload_champ,
					basetype, boffset);
      else
	*valp = value_fn_field (&temp, fns_ptr, method_oload_champ,
				basetype, boffset);
    }
  else
    *symp = oload_syms[func_oload_champ];

  if (objp)
    {
      struct type *temp_type = check_typedef (value_type (temp));
      struct type *objtype = check_typedef (obj_type);

      if (TYPE_CODE (temp_type) != TYPE_CODE_PTR
	  && (TYPE_CODE (objtype) == TYPE_CODE_PTR
	      || TYPE_CODE (objtype) == TYPE_CODE_REF))
	{
	  temp = value_addr (temp);
	}
      *objp = temp;
    }

  do_cleanups (all_cleanups);

  switch (match_quality)
    {
    case INCOMPATIBLE:
      return 100;
    case NON_STANDARD:
      return 10;
    default:				/* STANDARD */
      return 0;
    }
}

/* Find the best overload match, searching for FUNC_NAME in namespaces
   contained in QUALIFIED_NAME until it either finds a good match or
   runs out of namespaces.  It stores the overloaded functions in
   *OLOAD_SYMS, and the badness vector in *OLOAD_CHAMP_BV.  The
   calling function is responsible for freeing *OLOAD_SYMS and
   *OLOAD_CHAMP_BV.  If NO_ADL, argument dependent lookup is not 
   performned.  */

static int
find_oload_champ_namespace (struct value **args, int nargs,
			    const char *func_name,
			    const char *qualified_name,
			    struct symbol ***oload_syms,
			    struct badness_vector **oload_champ_bv,
			    const int no_adl)
{
  int oload_champ;

  find_oload_champ_namespace_loop (args, nargs,
				   func_name,
				   qualified_name, 0,
				   oload_syms, oload_champ_bv,
				   &oload_champ,
				   no_adl);

  return oload_champ;
}

/* Helper function for find_oload_champ_namespace; NAMESPACE_LEN is
   how deep we've looked for namespaces, and the champ is stored in
   OLOAD_CHAMP.  The return value is 1 if the champ is a good one, 0
   if it isn't.  Other arguments are the same as in
   find_oload_champ_namespace

   It is the caller's responsibility to free *OLOAD_SYMS and
   *OLOAD_CHAMP_BV.  */

static int
find_oload_champ_namespace_loop (struct value **args, int nargs,
				 const char *func_name,
				 const char *qualified_name,
				 int namespace_len,
				 struct symbol ***oload_syms,
				 struct badness_vector **oload_champ_bv,
				 int *oload_champ,
				 const int no_adl)
{
  int next_namespace_len = namespace_len;
  int searched_deeper = 0;
  int num_fns = 0;
  struct cleanup *old_cleanups;
  int new_oload_champ;
  struct symbol **new_oload_syms;
  struct badness_vector *new_oload_champ_bv;
  char *new_namespace;

  if (next_namespace_len != 0)
    {
      gdb_assert (qualified_name[next_namespace_len] == ':');
      next_namespace_len +=  2;
    }
  next_namespace_len +=
    cp_find_first_component (qualified_name + next_namespace_len);

  /* Initialize these to values that can safely be xfree'd.  */
  *oload_syms = NULL;
  *oload_champ_bv = NULL;

  /* First, see if we have a deeper namespace we can search in.
     If we get a good match there, use it.  */

  if (qualified_name[next_namespace_len] == ':')
    {
      searched_deeper = 1;

      if (find_oload_champ_namespace_loop (args, nargs,
					   func_name, qualified_name,
					   next_namespace_len,
					   oload_syms, oload_champ_bv,
					   oload_champ, no_adl))
	{
	  return 1;
	}
    };

  /* If we reach here, either we're in the deepest namespace or we
     didn't find a good match in a deeper namespace.  But, in the
     latter case, we still have a bad match in a deeper namespace;
     note that we might not find any match at all in the current
     namespace.  (There's always a match in the deepest namespace,
     because this overload mechanism only gets called if there's a
     function symbol to start off with.)  */

  old_cleanups = make_cleanup (xfree, *oload_syms);
  make_cleanup (xfree, *oload_champ_bv);
  new_namespace = alloca (namespace_len + 1);
  strncpy (new_namespace, qualified_name, namespace_len);
  new_namespace[namespace_len] = '\0';
  new_oload_syms = make_symbol_overload_list (func_name,
					      new_namespace);

  /* If we have reached the deepest level perform argument
     determined lookup.  */
  if (!searched_deeper && !no_adl)
    {
      int ix;
      struct type **arg_types;

      /* Prepare list of argument types for overload resolution.  */
      arg_types = (struct type **)
	alloca (nargs * (sizeof (struct type *)));
      for (ix = 0; ix < nargs; ix++)
	arg_types[ix] = value_type (args[ix]);
      make_symbol_overload_list_adl (arg_types, nargs, func_name);
    }

  while (new_oload_syms[num_fns])
    ++num_fns;

  new_oload_champ = find_oload_champ (args, nargs, 0, num_fns,
				      NULL, new_oload_syms,
				      &new_oload_champ_bv);

  /* Case 1: We found a good match.  Free earlier matches (if any),
     and return it.  Case 2: We didn't find a good match, but we're
     not the deepest function.  Then go with the bad match that the
     deeper function found.  Case 3: We found a bad match, and we're
     the deepest function.  Then return what we found, even though
     it's a bad match.  */

  if (new_oload_champ != -1
      && classify_oload_match (new_oload_champ_bv, nargs, 0) == STANDARD)
    {
      *oload_syms = new_oload_syms;
      *oload_champ = new_oload_champ;
      *oload_champ_bv = new_oload_champ_bv;
      do_cleanups (old_cleanups);
      return 1;
    }
  else if (searched_deeper)
    {
      xfree (new_oload_syms);
      xfree (new_oload_champ_bv);
      discard_cleanups (old_cleanups);
      return 0;
    }
  else
    {
      *oload_syms = new_oload_syms;
      *oload_champ = new_oload_champ;
      *oload_champ_bv = new_oload_champ_bv;
      do_cleanups (old_cleanups);
      return 0;
    }
}

/* Look for a function to take NARGS args of ARGS.  Find
   the best match from among the overloaded methods or functions
   (depending on METHOD) given by FNS_PTR or OLOAD_SYMS, respectively.
   The number of methods/functions in the list is given by NUM_FNS.
   Return the index of the best match; store an indication of the
   quality of the match in OLOAD_CHAMP_BV.

   It is the caller's responsibility to free *OLOAD_CHAMP_BV.  */

static int
find_oload_champ (struct value **args, int nargs, int method,
		  int num_fns, struct fn_field *fns_ptr,
		  struct symbol **oload_syms,
		  struct badness_vector **oload_champ_bv)
{
  int ix;
  /* A measure of how good an overloaded instance is.  */
  struct badness_vector *bv;
  /* Index of best overloaded function.  */
  int oload_champ = -1;
  /* Current ambiguity state for overload resolution.  */
  int oload_ambiguous = 0;
  /* 0 => no ambiguity, 1 => two good funcs, 2 => incomparable funcs.  */

  *oload_champ_bv = NULL;

  /* Consider each candidate in turn.  */
  for (ix = 0; ix < num_fns; ix++)
    {
      int jj;
      int static_offset = oload_method_static (method, fns_ptr, ix);
      int nparms;
      struct type **parm_types;

      if (method)
	{
	  nparms = TYPE_NFIELDS (TYPE_FN_FIELD_TYPE (fns_ptr, ix));
	}
      else
	{
	  /* If it's not a method, this is the proper place.  */
	  nparms = TYPE_NFIELDS (SYMBOL_TYPE (oload_syms[ix]));
	}

      /* Prepare array of parameter types.  */
      parm_types = (struct type **) 
	xmalloc (nparms * (sizeof (struct type *)));
      for (jj = 0; jj < nparms; jj++)
	parm_types[jj] = (method
			  ? (TYPE_FN_FIELD_ARGS (fns_ptr, ix)[jj].type)
			  : TYPE_FIELD_TYPE (SYMBOL_TYPE (oload_syms[ix]), 
					     jj));

      /* Compare parameter types to supplied argument types.  Skip
         THIS for static methods.  */
      bv = rank_function (parm_types, nparms, 
			  args + static_offset,
			  nargs - static_offset);

      if (!*oload_champ_bv)
	{
	  *oload_champ_bv = bv;
	  oload_champ = 0;
	}
      else /* See whether current candidate is better or worse than
	      previous best.  */
	switch (compare_badness (bv, *oload_champ_bv))
	  {
	  case 0:		/* Top two contenders are equally good.  */
	    oload_ambiguous = 1;
	    break;
	  case 1:		/* Incomparable top contenders.  */
	    oload_ambiguous = 2;
	    break;
	  case 2:		/* New champion, record details.  */
	    *oload_champ_bv = bv;
	    oload_ambiguous = 0;
	    oload_champ = ix;
	    break;
	  case 3:
	  default:
	    break;
	  }
      xfree (parm_types);
      if (overload_debug)
	{
	  if (method)
	    fprintf_filtered (gdb_stderr,
			      "Overloaded method instance %s, # of parms %d\n",
			      fns_ptr[ix].physname, nparms);
	  else
	    fprintf_filtered (gdb_stderr,
			      "Overloaded function instance "
			      "%s # of parms %d\n",
			      SYMBOL_DEMANGLED_NAME (oload_syms[ix]), 
			      nparms);
	  for (jj = 0; jj < nargs - static_offset; jj++)
	    fprintf_filtered (gdb_stderr,
			      "...Badness @ %d : %d\n", 
			      jj, bv->rank[jj].rank);
	  fprintf_filtered (gdb_stderr, "Overload resolution "
			    "champion is %d, ambiguous? %d\n", 
			    oload_champ, oload_ambiguous);
	}
    }

  return oload_champ;
}

/* Return 1 if we're looking at a static method, 0 if we're looking at
   a non-static method or a function that isn't a method.  */

static int
oload_method_static (int method, struct fn_field *fns_ptr, int index)
{
  if (method && fns_ptr && index >= 0
      && TYPE_FN_FIELD_STATIC_P (fns_ptr, index))
    return 1;
  else
    return 0;
}

/* Check how good an overload match OLOAD_CHAMP_BV represents.  */

static enum oload_classification
classify_oload_match (struct badness_vector *oload_champ_bv,
		      int nargs,
		      int static_offset)
{
  int ix;
  enum oload_classification worst = STANDARD;

  for (ix = 1; ix <= nargs - static_offset; ix++)
    {
      /* If this conversion is as bad as INCOMPATIBLE_TYPE_BADNESS
         or worse return INCOMPATIBLE.  */
      if (compare_ranks (oload_champ_bv->rank[ix],
                         INCOMPATIBLE_TYPE_BADNESS) <= 0)
	return INCOMPATIBLE;	/* Truly mismatched types.  */
      /* Otherwise If this conversion is as bad as
         NS_POINTER_CONVERSION_BADNESS or worse return NON_STANDARD.  */
      else if (compare_ranks (oload_champ_bv->rank[ix],
                              NS_POINTER_CONVERSION_BADNESS) <= 0)
	worst = NON_STANDARD;	/* Non-standard type conversions
				   needed.  */
    }

  /* If no INCOMPATIBLE classification was found, return the worst one
     that was found (if any).  */
  return worst;
}

/* C++: return 1 is NAME is a legitimate name for the destructor of
   type TYPE.  If TYPE does not have a destructor, or if NAME is
   inappropriate for TYPE, an error is signaled.  Parameter TYPE should not yet
   have CHECK_TYPEDEF applied, this function will apply it itself.  */

int
destructor_name_p (const char *name, struct type *type)
{
  if (name[0] == '~')
    {
      const char *dname = type_name_no_tag_or_error (type);
      const char *cp = strchr (dname, '<');
      unsigned int len;

      /* Do not compare the template part for template classes.  */
      if (cp == NULL)
	len = strlen (dname);
      else
	len = cp - dname;
      if (strlen (name + 1) != len || strncmp (dname, name + 1, len) != 0)
	error (_("name of destructor must equal name of class"));
      else
	return 1;
    }
  return 0;
}

/* C++: Given an aggregate type CURTYPE, and a member name NAME,
   return the appropriate member (or the address of the member, if
   WANT_ADDRESS).  This function is used to resolve user expressions
   of the form "DOMAIN::NAME".  For more details on what happens, see
   the comment before value_struct_elt_for_reference.  */

struct value *
value_aggregate_elt (struct type *curtype, char *name,
		     struct type *expect_type, int want_address,
		     enum noside noside)
{
  switch (TYPE_CODE (curtype))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return value_struct_elt_for_reference (curtype, 0, curtype, 
					     name, expect_type,
					     want_address, noside);
    case TYPE_CODE_NAMESPACE:
      return value_namespace_elt (curtype, name, 
				  want_address, noside);
    default:
      internal_error (__FILE__, __LINE__,
		      _("non-aggregate type in value_aggregate_elt"));
    }
}

/* Compares the two method/function types T1 and T2 for "equality" 
   with respect to the methods' parameters.  If the types of the
   two parameter lists are the same, returns 1; 0 otherwise.  This
   comparison may ignore any artificial parameters in T1 if
   SKIP_ARTIFICIAL is non-zero.  This function will ALWAYS skip
   the first artificial parameter in T1, assumed to be a 'this' pointer.

   The type T2 is expected to have come from make_params (in eval.c).  */

static int
compare_parameters (struct type *t1, struct type *t2, int skip_artificial)
{
  int start = 0;

  if (TYPE_NFIELDS (t1) > 0 && TYPE_FIELD_ARTIFICIAL (t1, 0))
    ++start;

  /* If skipping artificial fields, find the first real field
     in T1.  */
  if (skip_artificial)
    {
      while (start < TYPE_NFIELDS (t1)
	     && TYPE_FIELD_ARTIFICIAL (t1, start))
	++start;
    }

  /* Now compare parameters.  */

  /* Special case: a method taking void.  T1 will contain no
     non-artificial fields, and T2 will contain TYPE_CODE_VOID.  */
  if ((TYPE_NFIELDS (t1) - start) == 0 && TYPE_NFIELDS (t2) == 1
      && TYPE_CODE (TYPE_FIELD_TYPE (t2, 0)) == TYPE_CODE_VOID)
    return 1;

  if ((TYPE_NFIELDS (t1) - start) == TYPE_NFIELDS (t2))
    {
      int i;

      for (i = 0; i < TYPE_NFIELDS (t2); ++i)
	{
	  if (compare_ranks (rank_one_type (TYPE_FIELD_TYPE (t1, start + i),
					    TYPE_FIELD_TYPE (t2, i), NULL),
	                     EXACT_MATCH_BADNESS) != 0)
	    return 0;
	}

      return 1;
    }

  return 0;
}

/* C++: Given an aggregate type CURTYPE, and a member name NAME,
   return the address of this member as a "pointer to member" type.
   If INTYPE is non-null, then it will be the type of the member we
   are looking for.  This will help us resolve "pointers to member
   functions".  This function is used to resolve user expressions of
   the form "DOMAIN::NAME".  */

static struct value *
value_struct_elt_for_reference (struct type *domain, int offset,
				struct type *curtype, char *name,
				struct type *intype, 
				int want_address,
				enum noside noside)
{
  struct type *t = curtype;
  int i;
  struct value *v, *result;

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error (_("Internal error: non-aggregate type "
	     "to value_struct_elt_for_reference"));

  for (i = TYPE_NFIELDS (t) - 1; i >= TYPE_N_BASECLASSES (t); i--)
    {
      const char *t_field_name = TYPE_FIELD_NAME (t, i);

      if (t_field_name && strcmp (t_field_name, name) == 0)
	{
	  if (field_is_static (&TYPE_FIELD (t, i)))
	    {
	      v = value_static_field (t, i);
	      if (v == NULL)
		error (_("static field %s has been optimized out"),
		       name);
	      if (want_address)
		v = value_addr (v);
	      return v;
	    }
	  if (TYPE_FIELD_PACKED (t, i))
	    error (_("pointers to bitfield members not allowed"));

	  if (want_address)
	    return value_from_longest
	      (lookup_memberptr_type (TYPE_FIELD_TYPE (t, i), domain),
	       offset + (LONGEST) (TYPE_FIELD_BITPOS (t, i) >> 3));
	  else if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    return allocate_value (TYPE_FIELD_TYPE (t, i));
	  else
	    error (_("Cannot reference non-static field \"%s\""), name);
	}
    }

  /* C++: If it was not found as a data field, then try to return it
     as a pointer to a method.  */

  /* Perform all necessary dereferencing.  */
  while (intype && TYPE_CODE (intype) == TYPE_CODE_PTR)
    intype = TYPE_TARGET_TYPE (intype);

  for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
    {
      const char *t_field_name = TYPE_FN_FIELDLIST_NAME (t, i);
      char dem_opname[64];

      if (strncmp (t_field_name, "__", 2) == 0 
	  || strncmp (t_field_name, "op", 2) == 0 
	  || strncmp (t_field_name, "type", 4) == 0)
	{
	  if (cplus_demangle_opname (t_field_name, 
				     dem_opname, DMGL_ANSI))
	    t_field_name = dem_opname;
	  else if (cplus_demangle_opname (t_field_name, 
					  dem_opname, 0))
	    t_field_name = dem_opname;
	}
      if (t_field_name && strcmp (t_field_name, name) == 0)
	{
	  int j;
	  int len = TYPE_FN_FIELDLIST_LENGTH (t, i);
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	  check_stub_method_group (t, i);

	  if (intype)
	    {
	      for (j = 0; j < len; ++j)
		{
		  if (compare_parameters (TYPE_FN_FIELD_TYPE (f, j), intype, 0)
		      || compare_parameters (TYPE_FN_FIELD_TYPE (f, j),
					     intype, 1))
		    break;
		}

	      if (j == len)
		error (_("no member function matches "
			 "that type instantiation"));
	    }
	  else
	    {
	      int ii;

	      j = -1;
	      for (ii = 0; ii < len; ++ii)
		{
		  /* Skip artificial methods.  This is necessary if,
		     for example, the user wants to "print
		     subclass::subclass" with only one user-defined
		     constructor.  There is no ambiguity in this case.
		     We are careful here to allow artificial methods
		     if they are the unique result.  */
		  if (TYPE_FN_FIELD_ARTIFICIAL (f, ii))
		    {
		      if (j == -1)
			j = ii;
		      continue;
		    }

		  /* Desired method is ambiguous if more than one
		     method is defined.  */
		  if (j != -1 && !TYPE_FN_FIELD_ARTIFICIAL (f, j))
		    error (_("non-unique member `%s' requires "
			     "type instantiation"), name);

		  j = ii;
		}

	      if (j == -1)
		error (_("no matching member function"));
	    }

	  if (TYPE_FN_FIELD_STATIC_P (f, j))
	    {
	      struct symbol *s = 
		lookup_symbol (TYPE_FN_FIELD_PHYSNAME (f, j),
			       0, VAR_DOMAIN, 0);

	      if (s == NULL)
		return NULL;

	      if (want_address)
		return value_addr (read_var_value (s, 0));
	      else
		return read_var_value (s, 0);
	    }

	  if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
	    {
	      if (want_address)
		{
		  result = allocate_value
		    (lookup_methodptr_type (TYPE_FN_FIELD_TYPE (f, j)));
		  cplus_make_method_ptr (value_type (result),
					 value_contents_writeable (result),
					 TYPE_FN_FIELD_VOFFSET (f, j), 1);
		}
	      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
		return allocate_value (TYPE_FN_FIELD_TYPE (f, j));
	      else
		error (_("Cannot reference virtual member function \"%s\""),
		       name);
	    }
	  else
	    {
	      struct symbol *s = 
		lookup_symbol (TYPE_FN_FIELD_PHYSNAME (f, j),
			       0, VAR_DOMAIN, 0);

	      if (s == NULL)
		return NULL;

	      v = read_var_value (s, 0);
	      if (!want_address)
		result = v;
	      else
		{
		  result = allocate_value (lookup_methodptr_type (TYPE_FN_FIELD_TYPE (f, j)));
		  cplus_make_method_ptr (value_type (result),
					 value_contents_writeable (result),
					 value_address (v), 0);
		}
	    }
	  return result;
	}
    }
  for (i = TYPE_N_BASECLASSES (t) - 1; i >= 0; i--)
    {
      struct value *v;
      int base_offset;

      if (BASETYPE_VIA_VIRTUAL (t, i))
	base_offset = 0;
      else
	base_offset = TYPE_BASECLASS_BITPOS (t, i) / 8;
      v = value_struct_elt_for_reference (domain,
					  offset + base_offset,
					  TYPE_BASECLASS (t, i),
					  name, intype, 
					  want_address, noside);
      if (v)
	return v;
    }

  /* As a last chance, pretend that CURTYPE is a namespace, and look
     it up that way; this (frequently) works for types nested inside
     classes.  */

  return value_maybe_namespace_elt (curtype, name, 
				    want_address, noside);
}

/* C++: Return the member NAME of the namespace given by the type
   CURTYPE.  */

static struct value *
value_namespace_elt (const struct type *curtype,
		     char *name, int want_address,
		     enum noside noside)
{
  struct value *retval = value_maybe_namespace_elt (curtype, name,
						    want_address, 
						    noside);

  if (retval == NULL)
    error (_("No symbol \"%s\" in namespace \"%s\"."), 
	   name, TYPE_TAG_NAME (curtype));

  return retval;
}

/* A helper function used by value_namespace_elt and
   value_struct_elt_for_reference.  It looks up NAME inside the
   context CURTYPE; this works if CURTYPE is a namespace or if CURTYPE
   is a class and NAME refers to a type in CURTYPE itself (as opposed
   to, say, some base class of CURTYPE).  */

static struct value *
value_maybe_namespace_elt (const struct type *curtype,
			   char *name, int want_address,
			   enum noside noside)
{
  const char *namespace_name = TYPE_TAG_NAME (curtype);
  struct symbol *sym;
  struct value *result;

  sym = cp_lookup_symbol_namespace (namespace_name, name,
				    get_selected_block (0), VAR_DOMAIN);

  if (sym == NULL)
    {
      char *concatenated_name = alloca (strlen (namespace_name) + 2
					+ strlen (name) + 1);

      sprintf (concatenated_name, "%s::%s", namespace_name, name);
      sym = lookup_static_symbol_aux (concatenated_name, VAR_DOMAIN);
    }

  if (sym == NULL)
    return NULL;
  else if ((noside == EVAL_AVOID_SIDE_EFFECTS)
	   && (SYMBOL_CLASS (sym) == LOC_TYPEDEF))
    result = allocate_value (SYMBOL_TYPE (sym));
  else
    result = value_of_variable (sym, get_selected_block (0));

  if (result && want_address)
    result = value_addr (result);

  return result;
}

/* Given a pointer or a reference value V, find its real (RTTI) type.

   Other parameters FULL, TOP, USING_ENC as with value_rtti_type()
   and refer to the values computed for the object pointed to.  */

struct type *
value_rtti_indirect_type (struct value *v, int *full, 
			  int *top, int *using_enc)
{
  struct value *target;
  struct type *type, *real_type, *target_type;

  type = value_type (v);
  type = check_typedef (type);
  if (TYPE_CODE (type) == TYPE_CODE_REF)
    target = coerce_ref (v);
  else if (TYPE_CODE (type) == TYPE_CODE_PTR)
    target = value_ind (v);
  else
    return NULL;

  real_type = value_rtti_type (target, full, top, using_enc);

  if (real_type)
    {
      /* Copy qualifiers to the referenced object.  */
      target_type = value_type (target);
      real_type = make_cv_type (TYPE_CONST (target_type),
				TYPE_VOLATILE (target_type), real_type, NULL);
      if (TYPE_CODE (type) == TYPE_CODE_REF)
        real_type = lookup_reference_type (real_type);
      else if (TYPE_CODE (type) == TYPE_CODE_PTR)
        real_type = lookup_pointer_type (real_type);
      else
        internal_error (__FILE__, __LINE__, _("Unexpected value type."));

      /* Copy qualifiers to the pointer/reference.  */
      real_type = make_cv_type (TYPE_CONST (type), TYPE_VOLATILE (type),
				real_type, NULL);
    }

  return real_type;
}

/* Given a value pointed to by ARGP, check its real run-time type, and
   if that is different from the enclosing type, create a new value
   using the real run-time type as the enclosing type (and of the same
   type as ARGP) and return it, with the embedded offset adjusted to
   be the correct offset to the enclosed object.  RTYPE is the type,
   and XFULL, XTOP, and XUSING_ENC are the other parameters, computed
   by value_rtti_type().  If these are available, they can be supplied
   and a second call to value_rtti_type() is avoided.  (Pass RTYPE ==
   NULL if they're not available.  */

struct value *
value_full_object (struct value *argp, 
		   struct type *rtype, 
		   int xfull, int xtop,
		   int xusing_enc)
{
  struct type *real_type;
  int full = 0;
  int top = -1;
  int using_enc = 0;
  struct value *new_val;

  if (rtype)
    {
      real_type = rtype;
      full = xfull;
      top = xtop;
      using_enc = xusing_enc;
    }
  else
    real_type = value_rtti_type (argp, &full, &top, &using_enc);

  /* If no RTTI data, or if object is already complete, do nothing.  */
  if (!real_type || real_type == value_enclosing_type (argp))
    return argp;

  /* In a destructor we might see a real type that is a superclass of
     the object's type.  In this case it is better to leave the object
     as-is.  */
  if (full
      && TYPE_LENGTH (real_type) < TYPE_LENGTH (value_enclosing_type (argp)))
    return argp;

  /* If we have the full object, but for some reason the enclosing
     type is wrong, set it.  */
  /* pai: FIXME -- sounds iffy */
  if (full)
    {
      argp = value_copy (argp);
      set_value_enclosing_type (argp, real_type);
      return argp;
    }

  /* Check if object is in memory.  */
  if (VALUE_LVAL (argp) != lval_memory)
    {
      warning (_("Couldn't retrieve complete object of RTTI "
		 "type %s; object may be in register(s)."), 
	       TYPE_NAME (real_type));

      return argp;
    }

  /* All other cases -- retrieve the complete object.  */
  /* Go back by the computed top_offset from the beginning of the
     object, adjusting for the embedded offset of argp if that's what
     value_rtti_type used for its computation.  */
  new_val = value_at_lazy (real_type, value_address (argp) - top +
			   (using_enc ? 0 : value_embedded_offset (argp)));
  deprecated_set_value_type (new_val, value_type (argp));
  set_value_embedded_offset (new_val, (using_enc
				       ? top + value_embedded_offset (argp)
				       : top));
  return new_val;
}


/* Return the value of the local variable, if one exists.  Throw error
   otherwise, such as if the request is made in an inappropriate context.  */

struct value *
value_of_this (const struct language_defn *lang)
{
  struct symbol *sym;
  struct block *b;
  struct frame_info *frame;

  if (!lang->la_name_of_this)
    error (_("no `this' in current language"));

  frame = get_selected_frame (_("no frame selected"));

  b = get_frame_block (frame, NULL);

  sym = lookup_language_this (lang, b);
  if (sym == NULL)
    error (_("current stack frame does not contain a variable named `%s'"),
	   lang->la_name_of_this);

  return read_var_value (sym, frame);
}

/* Return the value of the local variable, if one exists.  Return NULL
   otherwise.  Never throw error.  */

struct value *
value_of_this_silent (const struct language_defn *lang)
{
  struct value *ret = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      ret = value_of_this (lang);
    }

  return ret;
}

/* Create a slice (sub-string, sub-array) of ARRAY, that is LENGTH
   elements long, starting at LOWBOUND.  The result has the same lower
   bound as the original ARRAY.  */

struct value *
value_slice (struct value *array, int lowbound, int length)
{
  struct type *slice_range_type, *slice_type, *range_type;
  LONGEST lowerbound, upperbound;
  struct value *slice;
  struct type *array_type;

  array_type = check_typedef (value_type (array));
  if (TYPE_CODE (array_type) != TYPE_CODE_ARRAY
      && TYPE_CODE (array_type) != TYPE_CODE_STRING)
    error (_("cannot take slice of non-array"));

  range_type = TYPE_INDEX_TYPE (array_type);
  if (get_discrete_bounds (range_type, &lowerbound, &upperbound) < 0)
    error (_("slice from bad array or bitstring"));

  if (lowbound < lowerbound || length < 0
      || lowbound + length - 1 > upperbound)
    error (_("slice out of range"));

  /* FIXME-type-allocation: need a way to free this type when we are
     done with it.  */
  slice_range_type = create_range_type ((struct type *) NULL,
					TYPE_TARGET_TYPE (range_type),
					lowbound, 
					lowbound + length - 1);

    {
      struct type *element_type = TYPE_TARGET_TYPE (array_type);
      LONGEST offset =
	(lowbound - lowerbound) * TYPE_LENGTH (check_typedef (element_type));

      slice_type = create_array_type ((struct type *) NULL, 
				      element_type,
				      slice_range_type);
      TYPE_CODE (slice_type) = TYPE_CODE (array_type);

      if (VALUE_LVAL (array) == lval_memory && value_lazy (array))
	slice = allocate_value_lazy (slice_type);
      else
	{
	  slice = allocate_value (slice_type);
	  value_contents_copy (slice, 0, array, offset,
			       TYPE_LENGTH (slice_type));
	}

      set_value_component_location (slice, array);
      VALUE_FRAME_ID (slice) = VALUE_FRAME_ID (array);
      set_value_offset (slice, value_offset (array) + offset);
    }
  return slice;
}

/* Create a value for a FORTRAN complex number.  Currently most of the
   time values are coerced to COMPLEX*16 (i.e. a complex number
   composed of 2 doubles.  This really should be a smarter routine
   that figures out precision inteligently as opposed to assuming
   doubles.  FIXME: fmb  */

struct value *
value_literal_complex (struct value *arg1, 
		       struct value *arg2,
		       struct type *type)
{
  struct value *val;
  struct type *real_type = TYPE_TARGET_TYPE (type);

  val = allocate_value (type);
  arg1 = value_cast (real_type, arg1);
  arg2 = value_cast (real_type, arg2);

  memcpy (value_contents_raw (val),
	  value_contents (arg1), TYPE_LENGTH (real_type));
  memcpy (value_contents_raw (val) + TYPE_LENGTH (real_type),
	  value_contents (arg2), TYPE_LENGTH (real_type));
  return val;
}

/* Cast a value into the appropriate complex data type.  */

static struct value *
cast_into_complex (struct type *type, struct value *val)
{
  struct type *real_type = TYPE_TARGET_TYPE (type);

  if (TYPE_CODE (value_type (val)) == TYPE_CODE_COMPLEX)
    {
      struct type *val_real_type = TYPE_TARGET_TYPE (value_type (val));
      struct value *re_val = allocate_value (val_real_type);
      struct value *im_val = allocate_value (val_real_type);

      memcpy (value_contents_raw (re_val),
	      value_contents (val), TYPE_LENGTH (val_real_type));
      memcpy (value_contents_raw (im_val),
	      value_contents (val) + TYPE_LENGTH (val_real_type),
	      TYPE_LENGTH (val_real_type));

      return value_literal_complex (re_val, im_val, type);
    }
  else if (TYPE_CODE (value_type (val)) == TYPE_CODE_FLT
	   || TYPE_CODE (value_type (val)) == TYPE_CODE_INT)
    return value_literal_complex (val, 
				  value_zero (real_type, not_lval), 
				  type);
  else
    error (_("cannot cast non-number to complex"));
}

void
_initialize_valops (void)
{
  add_setshow_boolean_cmd ("overload-resolution", class_support,
			   &overload_resolution, _("\
Set overload resolution in evaluating C++ functions."), _("\
Show overload resolution in evaluating C++ functions."), 
			   NULL, NULL,
			   show_overload_resolution,
			   &setlist, &showlist);
  overload_resolution = 1;
}
