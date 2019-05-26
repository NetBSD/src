/* Convert types from GDB to GCC

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "compile-internal.h"
/* An object that maps a gdb type to a gcc type.  */

struct type_map_instance
{
  /* The gdb type.  */

  struct type *type;

  /* The corresponding gcc type handle.  */

  gcc_type gcc_type_handle;
};

/* Hash a type_map_instance.  */

static hashval_t
hash_type_map_instance (const void *p)
{
  const struct type_map_instance *inst = (const struct type_map_instance *) p;

  return htab_hash_pointer (inst->type);
}

/* Check two type_map_instance objects for equality.  */

static int
eq_type_map_instance (const void *a, const void *b)
{
  const struct type_map_instance *insta = (const struct type_map_instance *) a;
  const struct type_map_instance *instb = (const struct type_map_instance *) b;

  return insta->type == instb->type;
}



/* Insert an entry into the type map associated with CONTEXT that maps
   from the gdb type TYPE to the gcc type GCC_TYPE.  It is ok for a
   given type to be inserted more than once, provided that the exact
   same association is made each time.  This simplifies how type
   caching works elsewhere in this file -- see how struct type caching
   is handled.  */

static void
insert_type (struct compile_c_instance *context, struct type *type,
	     gcc_type gcc_type)
{
  struct type_map_instance inst, *add;
  void **slot;

  inst.type = type;
  inst.gcc_type_handle = gcc_type;
  slot = htab_find_slot (context->type_map, &inst, INSERT);

  add = (struct type_map_instance *) *slot;
  /* The type might have already been inserted in order to handle
     recursive types.  */
  if (add != NULL && add->gcc_type_handle != gcc_type)
    error (_("Unexpected type id from GCC, check you use recent enough GCC."));

  if (add == NULL)
    {
      add = XNEW (struct type_map_instance);
      *add = inst;
      *slot = add;
    }
}

/* Convert a pointer type to its gcc representation.  */

static gcc_type
convert_pointer (struct compile_c_instance *context, struct type *type)
{
  gcc_type target = convert_type (context, TYPE_TARGET_TYPE (type));

  return C_CTX (context)->c_ops->build_pointer_type (C_CTX (context),
						     target);
}

/* Convert an array type to its gcc representation.  */

static gcc_type
convert_array (struct compile_c_instance *context, struct type *type)
{
  gcc_type element_type;
  struct type *range = TYPE_INDEX_TYPE (type);

  element_type = convert_type (context, TYPE_TARGET_TYPE (type));

  if (TYPE_LOW_BOUND_KIND (range) != PROP_CONST)
    return C_CTX (context)->c_ops->error (C_CTX (context),
					  _("array type with non-constant"
					    " lower bound is not supported"));
  if (TYPE_LOW_BOUND (range) != 0)
    return C_CTX (context)->c_ops->error (C_CTX (context),
					  _("cannot convert array type with "
					    "non-zero lower bound to C"));

  if (TYPE_HIGH_BOUND_KIND (range) == PROP_LOCEXPR
      || TYPE_HIGH_BOUND_KIND (range) == PROP_LOCLIST)
    {
      gcc_type result;
      char *upper_bound;

      if (TYPE_VECTOR (type))
	return C_CTX (context)->c_ops->error (C_CTX (context),
					      _("variably-sized vector type"
						" is not supported"));

      upper_bound = c_get_range_decl_name (&TYPE_RANGE_DATA (range)->high);
      result = C_CTX (context)->c_ops->build_vla_array_type (C_CTX (context),
							     element_type,
							     upper_bound);
      xfree (upper_bound);
      return result;
    }
  else
    {
      LONGEST low_bound, high_bound, count;

      if (get_array_bounds (type, &low_bound, &high_bound) == 0)
	count = -1;
      else
	{
	  gdb_assert (low_bound == 0); /* Ensured above.  */
	  count = high_bound + 1;
	}

      if (TYPE_VECTOR (type))
	return C_CTX (context)->c_ops->build_vector_type (C_CTX (context),
							  element_type,
							  count);
      return C_CTX (context)->c_ops->build_array_type (C_CTX (context),
						       element_type, count);
    }
}

/* Convert a struct or union type to its gcc representation.  */

static gcc_type
convert_struct_or_union (struct compile_c_instance *context, struct type *type)
{
  int i;
  gcc_type result;

  /* First we create the resulting type and enter it into our hash
     table.  This lets recursive types work.  */
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    result = C_CTX (context)->c_ops->build_record_type (C_CTX (context));
  else
    {
      gdb_assert (TYPE_CODE (type) == TYPE_CODE_UNION);
      result = C_CTX (context)->c_ops->build_union_type (C_CTX (context));
    }
  insert_type (context, type, result);

  for (i = 0; i < TYPE_NFIELDS (type); ++i)
    {
      gcc_type field_type;
      unsigned long bitsize = TYPE_FIELD_BITSIZE (type, i);

      field_type = convert_type (context, TYPE_FIELD_TYPE (type, i));
      if (bitsize == 0)
	bitsize = 8 * TYPE_LENGTH (TYPE_FIELD_TYPE (type, i));
      C_CTX (context)->c_ops->build_add_field (C_CTX (context), result,
					       TYPE_FIELD_NAME (type, i),
					       field_type,
					       bitsize,
					       TYPE_FIELD_BITPOS (type, i));
    }

  C_CTX (context)->c_ops->finish_record_or_union (C_CTX (context), result,
						  TYPE_LENGTH (type));
  return result;
}

/* Convert an enum type to its gcc representation.  */

static gcc_type
convert_enum (struct compile_c_instance *context, struct type *type)
{
  gcc_type int_type, result;
  int i;
  struct gcc_c_context *ctx = C_CTX (context);

  int_type = ctx->c_ops->int_type (ctx,
				   TYPE_UNSIGNED (type),
				   TYPE_LENGTH (type));

  result = ctx->c_ops->build_enum_type (ctx, int_type);
  for (i = 0; i < TYPE_NFIELDS (type); ++i)
    {
      ctx->c_ops->build_add_enum_constant (ctx,
					   result,
					   TYPE_FIELD_NAME (type, i),
					   TYPE_FIELD_ENUMVAL (type, i));
    }

  ctx->c_ops->finish_enum_type (ctx, result);

  return result;
}

/* Convert a function type to its gcc representation.  */

static gcc_type
convert_func (struct compile_c_instance *context, struct type *type)
{
  int i;
  gcc_type result, return_type;
  struct gcc_type_array array;
  int is_varargs = TYPE_VARARGS (type) || !TYPE_PROTOTYPED (type);

  /* This approach means we can't make self-referential function
     types.  Those are impossible in C, though.  */
  return_type = convert_type (context, TYPE_TARGET_TYPE (type));

  array.n_elements = TYPE_NFIELDS (type);
  array.elements = XNEWVEC (gcc_type, TYPE_NFIELDS (type));
  for (i = 0; i < TYPE_NFIELDS (type); ++i)
    array.elements[i] = convert_type (context, TYPE_FIELD_TYPE (type, i));

  result = C_CTX (context)->c_ops->build_function_type (C_CTX (context),
							return_type,
							&array, is_varargs);
  xfree (array.elements);

  return result;
}

/* Convert an integer type to its gcc representation.  */

static gcc_type
convert_int (struct compile_c_instance *context, struct type *type)
{
  return C_CTX (context)->c_ops->int_type (C_CTX (context),
					   TYPE_UNSIGNED (type),
					   TYPE_LENGTH (type));
}

/* Convert a floating-point type to its gcc representation.  */

static gcc_type
convert_float (struct compile_c_instance *context, struct type *type)
{
  return C_CTX (context)->c_ops->float_type (C_CTX (context),
					     TYPE_LENGTH (type));
}

/* Convert the 'void' type to its gcc representation.  */

static gcc_type
convert_void (struct compile_c_instance *context, struct type *type)
{
  return C_CTX (context)->c_ops->void_type (C_CTX (context));
}

/* Convert a boolean type to its gcc representation.  */

static gcc_type
convert_bool (struct compile_c_instance *context, struct type *type)
{
  return C_CTX (context)->c_ops->bool_type (C_CTX (context));
}

/* Convert a qualified type to its gcc representation.  */

static gcc_type
convert_qualified (struct compile_c_instance *context, struct type *type)
{
  struct type *unqual = make_unqualified_type (type);
  gcc_type unqual_converted;
  gcc_qualifiers_flags quals = 0;

  unqual_converted = convert_type (context, unqual);

  if (TYPE_CONST (type))
    quals |= GCC_QUALIFIER_CONST;
  if (TYPE_VOLATILE (type))
    quals |= GCC_QUALIFIER_VOLATILE;
  if (TYPE_RESTRICT (type))
    quals |= GCC_QUALIFIER_RESTRICT;

  return C_CTX (context)->c_ops->build_qualified_type (C_CTX (context),
						       unqual_converted,
						       quals);
}

/* Convert a complex type to its gcc representation.  */

static gcc_type
convert_complex (struct compile_c_instance *context, struct type *type)
{
  gcc_type base = convert_type (context, TYPE_TARGET_TYPE (type));

  return C_CTX (context)->c_ops->build_complex_type (C_CTX (context), base);
}

/* A helper function which knows how to convert most types from their
   gdb representation to the corresponding gcc form.  This examines
   the TYPE and dispatches to the appropriate conversion function.  It
   returns the gcc type.  */

static gcc_type
convert_type_basic (struct compile_c_instance *context, struct type *type)
{
  /* If we are converting a qualified type, first convert the
     unqualified type and then apply the qualifiers.  */
  if ((TYPE_INSTANCE_FLAGS (type) & (TYPE_INSTANCE_FLAG_CONST
				     | TYPE_INSTANCE_FLAG_VOLATILE
				     | TYPE_INSTANCE_FLAG_RESTRICT)) != 0)
    return convert_qualified (context, type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      return convert_pointer (context, type);

    case TYPE_CODE_ARRAY:
      return convert_array (context, type);

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return convert_struct_or_union (context, type);

    case TYPE_CODE_ENUM:
      return convert_enum (context, type);

    case TYPE_CODE_FUNC:
      return convert_func (context, type);

    case TYPE_CODE_INT:
      return convert_int (context, type);

    case TYPE_CODE_FLT:
      return convert_float (context, type);

    case TYPE_CODE_VOID:
      return convert_void (context, type);

    case TYPE_CODE_BOOL:
      return convert_bool (context, type);

    case TYPE_CODE_COMPLEX:
      return convert_complex (context, type);
    }

  return C_CTX (context)->c_ops->error (C_CTX (context),
					_("cannot convert gdb type "
					  "to gcc type"));
}

/* See compile-internal.h.  */

gcc_type
convert_type (struct compile_c_instance *context, struct type *type)
{
  struct type_map_instance inst, *found;
  gcc_type result;

  /* We don't ever have to deal with typedefs in this code, because
     those are only needed as symbols by the C compiler.  */
  type = check_typedef (type);

  inst.type = type;
  found = (struct type_map_instance *) htab_find (context->type_map, &inst);
  if (found != NULL)
    return found->gcc_type_handle;

  result = convert_type_basic (context, type);
  insert_type (context, type, result);
  return result;
}



/* Delete the compiler instance C.  */

static void
delete_instance (struct compile_instance *c)
{
  struct compile_c_instance *context = (struct compile_c_instance *) c;

  context->base.fe->ops->destroy (context->base.fe);
  htab_delete (context->type_map);
  if (context->symbol_err_map != NULL)
    htab_delete (context->symbol_err_map);
  xfree (context);
}

/* See compile-internal.h.  */

struct compile_instance *
new_compile_instance (struct gcc_c_context *fe)
{
  struct compile_c_instance *result = XCNEW (struct compile_c_instance);

  result->base.fe = &fe->base;
  result->base.destroy = delete_instance;
  result->base.gcc_target_options = ("-std=gnu11"
				     /* Otherwise the .o file may need
					"_Unwind_Resume" and
					"__gcc_personality_v0".  */
				     " -fno-exceptions");

  result->type_map = htab_create_alloc (10, hash_type_map_instance,
					eq_type_map_instance,
					xfree, xcalloc, xfree);

  fe->c_ops->set_callbacks (fe, gcc_convert_symbol,
			    gcc_symbol_address, result);

  return &result->base;
}
