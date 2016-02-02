/* Scheme interface to values.

   Copyright (C) 2008-2015 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "arch-utils.h"
#include "charset.h"
#include "cp-abi.h"
#include "infcall.h"
#include "symtab.h" /* Needed by language.h.  */
#include "language.h"
#include "valprint.h"
#include "value.h"
#include "guile-internal.h"

/* The <gdb:value> smob.  */

typedef struct _value_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* Doubly linked list of values in values_in_scheme.
     IWBN to use a chained_gdb_smob instead, which is doable, it just requires
     a bit more casting than normal.  */
  struct _value_smob *next;
  struct _value_smob *prev;

  struct value *value;

  /* These are cached here to avoid making multiple copies of them.
     Plus computing the dynamic_type can be a bit expensive.
     We use #f to indicate that the value doesn't exist (e.g. value doesn't
     have an address), so we need another value to indicate that we haven't
     computed the value yet.  For this we use SCM_UNDEFINED.  */
  SCM address;
  SCM type;
  SCM dynamic_type;
} value_smob;

static const char value_smob_name[] = "gdb:value";

/* The tag Guile knows the value smob by.  */
static scm_t_bits value_smob_tag;

/* List of all values which are currently exposed to Scheme. It is
   maintained so that when an objfile is discarded, preserve_values
   can copy the values' types if needed.  */
static value_smob *values_in_scheme;

/* Keywords used by Scheme procedures in this file.  */
static SCM type_keyword;
static SCM encoding_keyword;
static SCM errors_keyword;
static SCM length_keyword;

/* Possible #:errors values.  */
static SCM error_symbol;
static SCM escape_symbol;
static SCM substitute_symbol;

/* Administrivia for value smobs.  */

/* Iterate over all the <gdb:value> objects, calling preserve_one_value on
   each.
   This is the extension_language_ops.preserve_values "method".  */

void
gdbscm_preserve_values (const struct extension_language_defn *extlang,
			struct objfile *objfile, htab_t copied_types)
{
  value_smob *iter;

  for (iter = values_in_scheme; iter; iter = iter->next)
    preserve_one_value (iter->value, objfile, copied_types);
}

/* Helper to add a value_smob to the global list.  */

static void
vlscm_remember_scheme_value (value_smob *v_smob)
{
  v_smob->next = values_in_scheme;
  if (v_smob->next)
    v_smob->next->prev = v_smob;
  v_smob->prev = NULL;
  values_in_scheme = v_smob;
}

/* Helper to remove a value_smob from the global list.  */

static void
vlscm_forget_value_smob (value_smob *v_smob)
{
  /* Remove SELF from the global list.  */
  if (v_smob->prev)
    v_smob->prev->next = v_smob->next;
  else
    {
      gdb_assert (values_in_scheme == v_smob);
      values_in_scheme = v_smob->next;
    }
  if (v_smob->next)
    v_smob->next->prev = v_smob->prev;
}

/* The smob "free" function for <gdb:value>.  */

static size_t
vlscm_free_value_smob (SCM self)
{
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (self);

  vlscm_forget_value_smob (v_smob);
  value_free (v_smob->value);

  return 0;
}

/* The smob "print" function for <gdb:value>.  */

static int
vlscm_print_value_smob (SCM self, SCM port, scm_print_state *pstate)
{
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (self);
  char *s = NULL;
  struct value_print_options opts;
  volatile struct gdb_exception except;

  if (pstate->writingp)
    gdbscm_printf (port, "#<%s ", value_smob_name);

  get_user_print_options (&opts);
  opts.deref_ref = 0;

  /* pstate->writingp = zero if invoked by display/~A, and nonzero if
     invoked by write/~S.  What to do here may need to evolve.
     IWBN if we could pass an argument to format that would we could use
     instead of writingp.  */
  opts.raw = !!pstate->writingp;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct ui_file *stb = mem_fileopen ();
      struct cleanup *old_chain = make_cleanup_ui_file_delete (stb);

      common_val_print (v_smob->value, stb, 0, &opts, current_language);
      s = ui_file_xstrdup (stb, NULL);

      do_cleanups (old_chain);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (s != NULL)
    {
      scm_puts (s, port);
      xfree (s);
    }

  if (pstate->writingp)
    scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* The smob "equalp" function for <gdb:value>.  */

static SCM
vlscm_equal_p_value_smob (SCM v1, SCM v2)
{
  const value_smob *v1_smob = (value_smob *) SCM_SMOB_DATA (v1);
  const value_smob *v2_smob = (value_smob *) SCM_SMOB_DATA (v2);
  int result = 0;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      result = value_equal (v1_smob->value, v2_smob->value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return scm_from_bool (result);
}

/* Low level routine to create a <gdb:value> object.  */

static SCM
vlscm_make_value_smob (void)
{
  value_smob *v_smob = (value_smob *)
    scm_gc_malloc (sizeof (value_smob), value_smob_name);
  SCM v_scm;

  /* These must be filled in by the caller.  */
  v_smob->value = NULL;
  v_smob->prev = NULL;
  v_smob->next = NULL;

  /* These are lazily computed.  */
  v_smob->address = SCM_UNDEFINED;
  v_smob->type = SCM_UNDEFINED;
  v_smob->dynamic_type = SCM_UNDEFINED;

  v_scm = scm_new_smob (value_smob_tag, (scm_t_bits) v_smob);
  gdbscm_init_gsmob (&v_smob->base);

  return v_scm;
}

/* Return non-zero if SCM is a <gdb:value> object.  */

int
vlscm_is_value (SCM scm)
{
  return SCM_SMOB_PREDICATE (value_smob_tag, scm);
}

/* (value? object) -> boolean */

static SCM
gdbscm_value_p (SCM scm)
{
  return scm_from_bool (vlscm_is_value (scm));
}

/* Create a new <gdb:value> object that encapsulates VALUE.
   The value is released from the all_values chain so its lifetime is not
   bound to the execution of a command.  */

SCM
vlscm_scm_from_value (struct value *value)
{
  /* N.B. It's important to not cause any side-effects until we know the
     conversion worked.  */
  SCM v_scm = vlscm_make_value_smob ();
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);

  v_smob->value = value;
  release_value_or_incref (value);
  vlscm_remember_scheme_value (v_smob);

  return v_scm;
}

/* Returns the <gdb:value> object in SELF.
   Throws an exception if SELF is not a <gdb:value> object.  */

static SCM
vlscm_get_value_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (vlscm_is_value (self), self, arg_pos, func_name,
		   value_smob_name);

  return self;
}

/* Returns a pointer to the value smob of SELF.
   Throws an exception if SELF is not a <gdb:value> object.  */

static value_smob *
vlscm_get_value_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM v_scm = vlscm_get_value_arg_unsafe (self, arg_pos, func_name);
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);

  return v_smob;
}

/* Return the value field of V_SCM, an object of type <gdb:value>.
   This exists so that we don't have to export the struct's contents.  */

struct value *
vlscm_scm_to_value (SCM v_scm)
{
  value_smob *v_smob;

  gdb_assert (vlscm_is_value (v_scm));
  v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);
  return v_smob->value;
}

/* Value methods.  */

/* (make-value x [#:type type]) -> <gdb:value> */

static SCM
gdbscm_make_value (SCM x, SCM rest)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const struct language_defn *language = current_language;
  const SCM keywords[] = { type_keyword, SCM_BOOL_F };
  int type_arg_pos = -1;
  SCM type_scm = SCM_UNDEFINED;
  SCM except_scm, result;
  type_smob *t_smob;
  struct type *type = NULL;
  struct value *value;
  struct cleanup *cleanups;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#O", rest,
			      &type_arg_pos, &type_scm);

  if (type_arg_pos > 0)
    {
      t_smob = tyscm_get_type_smob_arg_unsafe (type_scm, type_arg_pos,
					       FUNC_NAME);
      type = tyscm_type_smob_type (t_smob);
    }

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  value = vlscm_convert_typed_value_from_scheme (FUNC_NAME, SCM_ARG1, x,
						 type_arg_pos, type_scm, type,
						 &except_scm,
						 gdbarch, language);
  if (value == NULL)
    {
      do_cleanups (cleanups);
      gdbscm_throw (except_scm);
    }

  result = vlscm_scm_from_value (value);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);
  return result;
}

/* (make-lazy-value <gdb:type> address) -> <gdb:value> */

static SCM
gdbscm_make_lazy_value (SCM type_scm, SCM address_scm)
{
  type_smob *t_smob;
  struct type *type;
  ULONGEST address;
  struct value *value = NULL;
  SCM result;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  t_smob = tyscm_get_type_smob_arg_unsafe (type_scm, SCM_ARG1, FUNC_NAME);
  type = tyscm_type_smob_type (t_smob);

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, NULL, "U",
			      address_scm, &address);

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  /* There's no (current) need to wrap this in a TRY_CATCH, but for consistency
     and future-proofing we do.  */
  TRY_CATCH (except, RETURN_MASK_ALL)
  {
    value = value_from_contents_and_address (type, NULL, address);
  }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  result = vlscm_scm_from_value (value);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);
  return result;
}

/* (value-optimized-out? <gdb:value>) -> boolean */

static SCM
gdbscm_value_optimized_out_p (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  int opt = 0;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      opt = value_optimized_out (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return scm_from_bool (opt);
}

/* (value-address <gdb:value>) -> integer
   Returns #f if the value doesn't have one.  */

static SCM
gdbscm_value_address (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  if (SCM_UNBNDP (v_smob->address))
    {
      struct value *res_val = NULL;
      struct cleanup *cleanup
	= make_cleanup_value_free_to_mark (value_mark ());
      SCM address;
      volatile struct gdb_exception except;

      TRY_CATCH (except, RETURN_MASK_ALL)
	{
	  res_val = value_addr (value);
	}
      if (except.reason < 0)
	address = SCM_BOOL_F;
      else
	address = vlscm_scm_from_value (res_val);

      do_cleanups (cleanup);

      if (gdbscm_is_exception (address))
	gdbscm_throw (address);

      v_smob->address = address;
    }

  return v_smob->address;
}

/* (value-dereference <gdb:value>) -> <gdb:value>
   Given a value of a pointer type, apply the C unary * operator to it.  */

static SCM
gdbscm_value_dereference (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  SCM result;
  struct value *res_val = NULL;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_val = value_ind (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-referenced-value <gdb:value>) -> <gdb:value>
   Given a value of a reference type, return the value referenced.
   The difference between this function and gdbscm_value_dereference is that
   the latter applies * unary operator to a value, which need not always
   result in the value referenced.
   For example, for a value which is a reference to an 'int' pointer ('int *'),
   gdbscm_value_dereference will result in a value of type 'int' while
   gdbscm_value_referenced_value will result in a value of type 'int *'.  */

static SCM
gdbscm_value_referenced_value (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  SCM result;
  struct value *res_val = NULL;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      switch (TYPE_CODE (check_typedef (value_type (value))))
        {
        case TYPE_CODE_PTR:
          res_val = value_ind (value);
          break;
        case TYPE_CODE_REF:
          res_val = coerce_ref (value);
          break;
        default:
          error (_("Trying to get the referenced value from a value which is"
		   " neither a pointer nor a reference"));
        }
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-type <gdb:value>) -> <gdb:type> */

static SCM
gdbscm_value_type (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  if (SCM_UNBNDP (v_smob->type))
    v_smob->type = tyscm_scm_from_type (value_type (value));

  return v_smob->type;
}

/* (value-dynamic-type <gdb:value>) -> <gdb:type> */

static SCM
gdbscm_value_dynamic_type (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type = NULL;
  volatile struct gdb_exception except;

  if (! SCM_UNBNDP (v_smob->type))
    return v_smob->dynamic_type;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct cleanup *cleanup
	= make_cleanup_value_free_to_mark (value_mark ());

      type = value_type (value);
      CHECK_TYPEDEF (type);

      if (((TYPE_CODE (type) == TYPE_CODE_PTR)
	   || (TYPE_CODE (type) == TYPE_CODE_REF))
	  && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_STRUCT))
	{
	  struct value *target;
	  int was_pointer = TYPE_CODE (type) == TYPE_CODE_PTR;

	  if (was_pointer)
	    target = value_ind (value);
	  else
	    target = coerce_ref (value);
	  type = value_rtti_type (target, NULL, NULL, NULL);

	  if (type)
	    {
	      if (was_pointer)
		type = lookup_pointer_type (type);
	      else
		type = lookup_reference_type (type);
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
	type = value_rtti_type (value, NULL, NULL, NULL);
      else
	{
	  /* Re-use object's static type.  */
	  type = NULL;
	}

      do_cleanups (cleanup);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (type == NULL)
    v_smob->dynamic_type = gdbscm_value_type (self);
  else
    v_smob->dynamic_type = tyscm_scm_from_type (type);

  return v_smob->dynamic_type;
}

/* A helper function that implements the various cast operators.  */

static SCM
vlscm_do_cast (SCM self, SCM type_scm, enum exp_opcode op,
	       const char *func_name)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (type_scm, SCM_ARG2, FUNC_NAME);
  struct type *type = tyscm_type_smob_type (t_smob);
  SCM result;
  struct value *res_val = NULL;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (op == UNOP_DYNAMIC_CAST)
	res_val = value_dynamic_cast (type, value);
      else if (op == UNOP_REINTERPRET_CAST)
	res_val = value_reinterpret_cast (type, value);
      else
	{
	  gdb_assert (op == UNOP_CAST);
	  res_val = value_cast (type, value);
	}
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  gdb_assert (res_val != NULL);
  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_CAST, FUNC_NAME);
}

/* (value-dynamic-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_dynamic_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_DYNAMIC_CAST, FUNC_NAME);
}

/* (value-reinterpret-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_reinterpret_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_REINTERPRET_CAST, FUNC_NAME);
}

/* (value-field <gdb:value> string) -> <gdb:value>
   Given string name of an element inside structure, return its <gdb:value>
   object.  */

static SCM
gdbscm_value_field (SCM self, SCM field_scm)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  char *field = NULL;
  struct value *res_val = NULL;
  SCM result;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  SCM_ASSERT_TYPE (scm_is_string (field_scm), field_scm, SCM_ARG2, FUNC_NAME,
		   _("string"));

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  field = gdbscm_scm_to_c_string (field_scm);
  make_cleanup (xfree, field);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *tmp = value;

      res_val = value_struct_elt (&tmp, NULL, field, NULL, NULL);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  gdb_assert (res_val != NULL);
  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-subscript <gdb:value> integer|<gdb:value>) -> <gdb:value>
   Return the specified value in an array.  */

static SCM
gdbscm_value_subscript (SCM self, SCM index_scm)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct value *index = NULL;
  struct value *res_val = NULL;
  struct type *type = value_type (value);
  struct gdbarch *gdbarch;
  SCM result, except_scm;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  SCM_ASSERT (type != NULL, self, SCM_ARG2, FUNC_NAME);
  gdbarch = get_type_arch (type);

  cleanups = make_cleanup_value_free_to_mark (value_mark ());

  index = vlscm_convert_value_from_scheme (FUNC_NAME, SCM_ARG2, index_scm,
					   &except_scm,
					   gdbarch, current_language);
  if (index == NULL)
    {
      do_cleanups (cleanups);
      gdbscm_throw (except_scm);
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *tmp = value;

      /* Assume we are attempting an array access, and let the value code
	 throw an exception if the index has an invalid type.
	 Check the value's type is something that can be accessed via
	 a subscript.  */
      tmp = coerce_ref (tmp);
      type = check_typedef (value_type (tmp));
      if (TYPE_CODE (type) != TYPE_CODE_ARRAY
	  && TYPE_CODE (type) != TYPE_CODE_PTR)
	error (_("Cannot subscript requested type"));

      res_val = value_subscript (tmp, value_as_long (index));
   }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  gdb_assert (res_val != NULL);
  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-call <gdb:value> arg-list) -> <gdb:value>
   Perform an inferior function call on the value.  */

static SCM
gdbscm_value_call (SCM self, SCM args)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *function = v_smob->value;
  struct value *mark = value_mark ();
  struct type *ftype = NULL;
  long args_count;
  struct value **vargs = NULL;
  SCM result = SCM_BOOL_F;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      ftype = check_typedef (value_type (function));
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  SCM_ASSERT_TYPE (TYPE_CODE (ftype) == TYPE_CODE_FUNC, self,
		   SCM_ARG1, FUNC_NAME,
		   _("function (value of TYPE_CODE_FUNC)"));

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (args)), args,
		   SCM_ARG2, FUNC_NAME, _("list"));

  args_count = scm_ilength (args);
  if (args_count > 0)
    {
      struct gdbarch *gdbarch = get_current_arch ();
      const struct language_defn *language = current_language;
      SCM except_scm;
      long i;

      vargs = alloca (sizeof (struct value *) * args_count);
      for (i = 0; i < args_count; i++)
	{
	  SCM arg = scm_car (args);

	  vargs[i] = vlscm_convert_value_from_scheme (FUNC_NAME,
						      GDBSCM_ARG_NONE, arg,
						      &except_scm,
						      gdbarch, language);
	  if (vargs[i] == NULL)
	    gdbscm_throw (except_scm);

	  args = scm_cdr (args);
	}
      gdb_assert (gdbscm_is_true (scm_null_p (args)));
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct cleanup *cleanup = make_cleanup_value_free_to_mark (mark);
      struct value *return_value;

      return_value = call_function_by_hand (function, args_count, vargs);
      result = vlscm_scm_from_value (return_value);
      do_cleanups (cleanup);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value->bytevector <gdb:value>) -> bytevector */

static SCM
gdbscm_value_to_bytevector (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  size_t length = 0;
  const gdb_byte *contents = NULL;
  SCM bv;
  volatile struct gdb_exception except;

  type = value_type (value);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      CHECK_TYPEDEF (type);
      length = TYPE_LENGTH (type);
      contents = value_contents (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  bv = scm_c_make_bytevector (length);
  memcpy (SCM_BYTEVECTOR_CONTENTS (bv), contents, length);

  return bv;
}

/* Helper function to determine if a type is "int-like".  */

static int
is_intlike (struct type *type, int ptr_ok)
{
  return (TYPE_CODE (type) == TYPE_CODE_INT
	  || TYPE_CODE (type) == TYPE_CODE_ENUM
	  || TYPE_CODE (type) == TYPE_CODE_BOOL
	  || TYPE_CODE (type) == TYPE_CODE_CHAR
	  || (ptr_ok && TYPE_CODE (type) == TYPE_CODE_PTR));
}

/* (value->bool <gdb:value>) -> boolean
   Throws an error if the value is not integer-like.  */

static SCM
gdbscm_value_to_bool (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  LONGEST l = 0;
  volatile struct gdb_exception except;

  type = value_type (value);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      CHECK_TYPEDEF (type);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  SCM_ASSERT_TYPE (is_intlike (type, 1), self, SCM_ARG1, FUNC_NAME,
		   _("integer-like gdb value"));

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (TYPE_CODE (type) == TYPE_CODE_PTR)
	l = value_as_address (value);
      else
	l = value_as_long (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return scm_from_bool (l != 0);
}

/* (value->integer <gdb:value>) -> integer
   Throws an error if the value is not integer-like.  */

static SCM
gdbscm_value_to_integer (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  LONGEST l = 0;
  volatile struct gdb_exception except;

  type = value_type (value);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      CHECK_TYPEDEF (type);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  SCM_ASSERT_TYPE (is_intlike (type, 1), self, SCM_ARG1, FUNC_NAME,
		   _("integer-like gdb value"));

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (TYPE_CODE (type) == TYPE_CODE_PTR)
	l = value_as_address (value);
      else
	l = value_as_long (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (TYPE_UNSIGNED (type))
    return gdbscm_scm_from_ulongest (l);
  else
    return gdbscm_scm_from_longest (l);
}

/* (value->real <gdb:value>) -> real
   Throws an error if the value is not a number.  */

static SCM
gdbscm_value_to_real (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  DOUBLEST d = 0;
  volatile struct gdb_exception except;

  type = value_type (value);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      CHECK_TYPEDEF (type);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  SCM_ASSERT_TYPE (is_intlike (type, 0) || TYPE_CODE (type) == TYPE_CODE_FLT,
		   self, SCM_ARG1, FUNC_NAME, _("number"));

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      d = value_as_double (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  /* TODO: Is there a better way to check if the value fits?  */
  if (d != (double) d)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _("number can't be converted to a double"));

  return scm_from_double (d);
}

/* (value->string <gdb:value>
       [#:encoding encoding]
       [#:errors #f | 'error | 'substitute]
       [#:length length])
     -> string
   Return Unicode string with value's contents, which must be a string.

   If ENCODING is not given, the string is assumed to be encoded in
   the target's charset.

   ERRORS is one of #f, 'error or 'substitute.
   An error setting of #f means use the default, which is Guile's
   %default-port-conversion-strategy when using Guile >= 2.0.6, or 'error if
   using an earlier version of Guile.  Earlier versions do not properly
   support obtaining the default port conversion strategy.
   If the default is not one of 'error or 'substitute, 'substitute is used.
   An error setting of "error" causes an exception to be thrown if there's
   a decoding error.  An error setting of "substitute" causes invalid
   characters to be replaced with "?".

   If LENGTH is provided, only fetch string to the length provided.
   LENGTH must be a Scheme integer, it can't be a <gdb:value> integer.  */

static SCM
gdbscm_value_to_string (SCM self, SCM rest)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  const SCM keywords[] = {
    encoding_keyword, errors_keyword, length_keyword, SCM_BOOL_F
  };
  int encoding_arg_pos = -1, errors_arg_pos = -1, length_arg_pos = -1;
  char *encoding = NULL;
  SCM errors = SCM_BOOL_F;
  int length = -1;
  gdb_byte *buffer = NULL;
  const char *la_encoding = NULL;
  struct type *char_type = NULL;
  SCM result;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#sOi", rest,
			      &encoding_arg_pos, &encoding,
			      &errors_arg_pos, &errors,
			      &length_arg_pos, &length);

  cleanups = make_cleanup (xfree, encoding);

  if (errors_arg_pos > 0
      && errors != SCM_BOOL_F
      && !scm_is_eq (errors, error_symbol)
      && !scm_is_eq (errors, substitute_symbol))
    {
      SCM excp
	= gdbscm_make_out_of_range_error (FUNC_NAME, errors_arg_pos, errors,
					  _("invalid error kind"));

      do_cleanups (cleanups);
      gdbscm_throw (excp);
    }
  if (errors == SCM_BOOL_F)
    {
      /* N.B. scm_port_conversion_strategy in Guile versions prior to 2.0.6
	 will throw a Scheme error when passed #f.  */
      if (gdbscm_guile_version_is_at_least (2, 0, 6))
	errors = scm_port_conversion_strategy (SCM_BOOL_F);
      else
	errors = error_symbol;
    }
  /* We don't assume anything about the result of scm_port_conversion_strategy.
     From this point on, if errors is not 'errors, use 'substitute.  */

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      LA_GET_STRING (value, &buffer, &length, &char_type, &la_encoding);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  /* If errors is "error" scm_from_stringn may throw a Scheme exception.
     Make sure we don't leak.  This is done via scm_dynwind_begin, et.al.  */
  discard_cleanups (cleanups);

  scm_dynwind_begin (0);

  gdbscm_dynwind_xfree (encoding);
  gdbscm_dynwind_xfree (buffer);

  result = scm_from_stringn ((const char *) buffer,
			     length * TYPE_LENGTH (char_type),
			     (encoding != NULL && *encoding != '\0'
			      ? encoding
			      : la_encoding),
			     scm_is_eq (errors, error_symbol)
			     ? SCM_FAILED_CONVERSION_ERROR
			     : SCM_FAILED_CONVERSION_QUESTION_MARK);

  scm_dynwind_end ();

  return result;
}

/* (value->lazy-string <gdb:value> [#:encoding encoding] [#:length length])
     -> <gdb:lazy-string>
   Return a Scheme object representing a lazy_string_object type.
   A lazy string is a pointer to a string with an optional encoding and length.
   If ENCODING is not given, the target's charset is used.
   If LENGTH is provided then the length parameter is set to LENGTH, otherwise
   length will be set to -1 (first null of appropriate with).
   LENGTH must be a Scheme integer, it can't be a <gdb:value> integer.  */

static SCM
gdbscm_value_to_lazy_string (SCM self, SCM rest)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  const SCM keywords[] = { encoding_keyword, length_keyword, SCM_BOOL_F };
  int encoding_arg_pos = -1, length_arg_pos = -1;
  char *encoding = NULL;
  int length = -1;
  SCM result = SCM_BOOL_F; /* -Wall */
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#si", rest,
			      &encoding_arg_pos, &encoding,
			      &length_arg_pos, &length);

  cleanups = make_cleanup (xfree, encoding);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct cleanup *inner_cleanup
	= make_cleanup_value_free_to_mark (value_mark ());

      if (TYPE_CODE (value_type (value)) == TYPE_CODE_PTR)
	value = value_ind (value);

      result = lsscm_make_lazy_string (value_address (value), length,
				       encoding, value_type (value));

      do_cleanups (inner_cleanup);
    }
  do_cleanups (cleanups);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-lazy? <gdb:value>) -> boolean */

static SCM
gdbscm_value_lazy_p (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return scm_from_bool (value_lazy (value));
}

/* (value-fetch-lazy! <gdb:value>) -> unspecified */

static SCM
gdbscm_value_fetch_lazy_x (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (value_lazy (value))
	value_fetch_lazy (value);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return SCM_UNSPECIFIED;
}

/* (value-print <gdb:value>) -> string */

static SCM
gdbscm_value_print (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct value_print_options opts;
  char *s = NULL;
  SCM result;
  volatile struct gdb_exception except;

  get_user_print_options (&opts);
  opts.deref_ref = 0;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct ui_file *stb = mem_fileopen ();
      struct cleanup *old_chain = make_cleanup_ui_file_delete (stb);

      common_val_print (value, stb, 0, &opts, current_language);
      s = ui_file_xstrdup (stb, NULL);

      do_cleanups (old_chain);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  /* Use SCM_FAILED_CONVERSION_QUESTION_MARK to ensure this doesn't
     throw an error if the encoding fails.
     IWBN to use scm_take_locale_string here, but we'd have to temporarily
     override the default port conversion handler because contrary to
     documentation it doesn't necessarily free the input string.  */
  result = scm_from_stringn (s, strlen (s), host_charset (),
			     SCM_FAILED_CONVERSION_QUESTION_MARK);
  xfree (s);

  return result;
}

/* (parse-and-eval string) -> <gdb:value>
   Parse a string and evaluate the string as an expression.  */

static SCM
gdbscm_parse_and_eval (SCM expr_scm)
{
  char *expr_str;
  struct value *res_val = NULL;
  SCM result;
  struct cleanup *cleanups;
  volatile struct gdb_exception except;

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "s",
			      expr_scm, &expr_str);

  cleanups = make_cleanup_value_free_to_mark (value_mark ());
  make_cleanup (xfree, expr_str);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_val = parse_and_eval (expr_str);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS (except, cleanups);

  gdb_assert (res_val != NULL);
  result = vlscm_scm_from_value (res_val);

  do_cleanups (cleanups);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (history-ref integer) -> <gdb:value>
   Return the specified value from GDB's value history.  */

static SCM
gdbscm_history_ref (SCM index)
{
  int i;
  struct value *res_val = NULL; /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "i", index, &i);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_val = access_value_history (i);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return vlscm_scm_from_value (res_val);
}

/* (history-append! <gdb:value>) -> index
   Append VALUE to GDB's value history.  Return its index in the history.  */

static SCM
gdbscm_history_append_x (SCM value)
{
  int res_index = -1;
  struct value *v;
  value_smob *v_smob;
  volatile struct gdb_exception except;

  v_smob = vlscm_get_value_smob_arg_unsafe (value, SCM_ARG1, FUNC_NAME);
  v = v_smob->value;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_index = record_latest_value (v);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return scm_from_int (res_index);
}

/* Initialize the Scheme value code.  */

static const scheme_function value_functions[] =
{
  { "value?", 1, 0, 0, gdbscm_value_p,
    "\
Return #t if the object is a <gdb:value> object." },

  { "make-value", 1, 0, 1, gdbscm_make_value,
    "\
Create a <gdb:value> representing object.\n\
Typically this is used to convert numbers and strings to\n\
<gdb:value> objects.\n\
\n\
  Arguments: object [#:type <gdb:type>]" },

  { "value-optimized-out?", 1, 0, 0, gdbscm_value_optimized_out_p,
    "\
Return #t if the value has been optimizd out." },

  { "value-address", 1, 0, 0, gdbscm_value_address,
    "\
Return the address of the value." },

  { "value-type", 1, 0, 0, gdbscm_value_type,
    "\
Return the type of the value." },

  { "value-dynamic-type", 1, 0, 0, gdbscm_value_dynamic_type,
    "\
Return the dynamic type of the value." },

  { "value-cast", 2, 0, 0, gdbscm_value_cast,
    "\
Cast the value to the supplied type.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-dynamic-cast", 2, 0, 0, gdbscm_value_dynamic_cast,
    "\
Cast the value to the supplied type, as if by the C++\n\
dynamic_cast operator.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-reinterpret-cast", 2, 0, 0, gdbscm_value_reinterpret_cast,
    "\
Cast the value to the supplied type, as if by the C++\n\
reinterpret_cast operator.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-dereference", 1, 0, 0, gdbscm_value_dereference,
    "\
Return the result of applying the C unary * operator to the value." },

  { "value-referenced-value", 1, 0, 0, gdbscm_value_referenced_value,
    "\
Given a value of a reference type, return the value referenced.\n\
The difference between this function and value-dereference is that\n\
the latter applies * unary operator to a value, which need not always\n\
result in the value referenced.\n\
For example, for a value which is a reference to an 'int' pointer ('int *'),\n\
value-dereference will result in a value of type 'int' while\n\
value-referenced-value will result in a value of type 'int *'." },

  { "value-field", 2, 0, 0, gdbscm_value_field,
    "\
Return the specified field of the value.\n\
\n\
  Arguments: <gdb:value> string" },

  { "value-subscript", 2, 0, 0, gdbscm_value_subscript,
    "\
Return the value of the array at the specified index.\n\
\n\
  Arguments: <gdb:value> integer" },

  { "value-call", 2, 0, 0, gdbscm_value_call,
    "\
Perform an inferior function call taking the value as a pointer to the\n\
function to call.\n\
Each element of the argument list must be a <gdb:value> object or an object\n\
that can be converted to one.\n\
The result is the value returned by the function.\n\
\n\
  Arguments: <gdb:value> arg-list" },

  { "value->bool", 1, 0, 0, gdbscm_value_to_bool,
    "\
Return the Scheme boolean representing the GDB value.\n\
The value must be \"integer like\".  Pointers are ok." },

  { "value->integer", 1, 0, 0, gdbscm_value_to_integer,
    "\
Return the Scheme integer representing the GDB value.\n\
The value must be \"integer like\".  Pointers are ok." },

  { "value->real", 1, 0, 0, gdbscm_value_to_real,
    "\
Return the Scheme real number representing the GDB value.\n\
The value must be a number." },

  { "value->bytevector", 1, 0, 0, gdbscm_value_to_bytevector,
    "\
Return a Scheme bytevector with the raw contents of the GDB value.\n\
No transformation, endian or otherwise, is performed." },

  { "value->string", 1, 0, 1, gdbscm_value_to_string,
    "\
Return the Unicode string of the value's contents.\n\
If ENCODING is not given, the string is assumed to be encoded in\n\
the target's charset.\n\
An error setting \"error\" causes an exception to be thrown if there's\n\
a decoding error.  An error setting of \"substitute\" causes invalid\n\
characters to be replaced with \"?\".  The default is \"error\".\n\
If LENGTH is provided, only fetch string to the length provided.\n\
\n\
  Arguments: <gdb:value>\n\
             [#:encoding encoding] [#:errors \"error\"|\"substitute\"]\n\
             [#:length length]" },

  { "value->lazy-string", 1, 0, 1, gdbscm_value_to_lazy_string,
    "\
Return a Scheme object representing a lazily fetched Unicode string\n\
of the value's contents.\n\
If ENCODING is not given, the string is assumed to be encoded in\n\
the target's charset.\n\
If LENGTH is provided, only fetch string to the length provided.\n\
\n\
  Arguments: <gdb:value> [#:encoding encoding] [#:length length]" },

  { "value-lazy?", 1, 0, 0, gdbscm_value_lazy_p,
    "\
Return #t if the value is lazy (not fetched yet from the inferior).\n\
A lazy value is fetched when needed, or when the value-fetch-lazy! function\n\
is called." },

  { "make-lazy-value", 2, 0, 0, gdbscm_make_lazy_value,
    "\
Create a <gdb:value> that will be lazily fetched from the target.\n\
\n\
  Arguments: <gdb:type> address" },

  { "value-fetch-lazy!", 1, 0, 0, gdbscm_value_fetch_lazy_x,
    "\
Fetch the value from the inferior, if it was lazy.\n\
The result is \"unspecified\"." },

  { "value-print", 1, 0, 0, gdbscm_value_print,
    "\
Return the string representation (print form) of the value." },

  { "parse-and-eval", 1, 0, 0, gdbscm_parse_and_eval,
    "\
Evaluates string in gdb and returns the result as a <gdb:value> object." },

  { "history-ref", 1, 0, 0, gdbscm_history_ref,
    "\
Return the specified value from GDB's value history." },

  { "history-append!", 1, 0, 0, gdbscm_history_append_x,
    "\
Append the specified value onto GDB's value history." },

  END_FUNCTIONS
};

void
gdbscm_initialize_values (void)
{
  value_smob_tag = gdbscm_make_smob_type (value_smob_name,
					  sizeof (value_smob));
  scm_set_smob_free (value_smob_tag, vlscm_free_value_smob);
  scm_set_smob_print (value_smob_tag, vlscm_print_value_smob);
  scm_set_smob_equalp (value_smob_tag, vlscm_equal_p_value_smob);

  gdbscm_define_functions (value_functions, 1);

  type_keyword = scm_from_latin1_keyword ("type");
  encoding_keyword = scm_from_latin1_keyword ("encoding");
  errors_keyword = scm_from_latin1_keyword ("errors");
  length_keyword = scm_from_latin1_keyword ("length");

  error_symbol = scm_from_latin1_symbol ("error");
  escape_symbol = scm_from_latin1_symbol ("escape");
  substitute_symbol = scm_from_latin1_symbol ("substitute");
}
