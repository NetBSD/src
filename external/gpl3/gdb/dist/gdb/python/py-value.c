/* Python interface to values.

   Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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
#include "gdb_assert.h"
#include "charset.h"
#include "value.h"
#include "exceptions.h"
#include "language.h"
#include "dfp.h"
#include "valprint.h"
#include "infcall.h"
#include "expression.h"
#include "cp-abi.h"

#ifdef HAVE_PYTHON

#include "python-internal.h"

/* Even though Python scalar types directly map to host types, we use
   target types here to remain consistent with the values system in
   GDB (which uses target arithmetic).  */

/* Python's integer type corresponds to C's long type.  */
#define builtin_type_pyint builtin_type (python_gdbarch)->builtin_long

/* Python's float type corresponds to C's double type.  */
#define builtin_type_pyfloat builtin_type (python_gdbarch)->builtin_double

/* Python's long type corresponds to C's long long type.  */
#define builtin_type_pylong builtin_type (python_gdbarch)->builtin_long_long

/* Python's long type corresponds to C's long long type.  Unsigned version.  */
#define builtin_type_upylong builtin_type \
  (python_gdbarch)->builtin_unsigned_long_long

#define builtin_type_pybool \
  language_bool_type (python_language, python_gdbarch)

#define builtin_type_pychar \
  language_string_char_type (python_language, python_gdbarch)

typedef struct value_object {
  PyObject_HEAD
  struct value_object *next;
  struct value_object *prev;
  struct value *value;
  PyObject *address;
  PyObject *type;
  PyObject *dynamic_type;
} value_object;

/* List of all values which are currently exposed to Python. It is
   maintained so that when an objfile is discarded, preserve_values
   can copy the values' types if needed.  */
/* This variable is unnecessarily initialized to NULL in order to
   work around a linker bug on MacOS.  */
static value_object *values_in_python = NULL;

/* Called by the Python interpreter when deallocating a value object.  */
static void
valpy_dealloc (PyObject *obj)
{
  value_object *self = (value_object *) obj;

  /* Remove SELF from the global list.  */
  if (self->prev)
    self->prev->next = self->next;
  else
    {
      gdb_assert (values_in_python == self);
      values_in_python = self->next;
    }
  if (self->next)
    self->next->prev = self->prev;

  value_free (self->value);

  if (self->address)
    /* Use braces to appease gcc warning.  *sigh*  */
    {
      Py_DECREF (self->address);
    }

  if (self->type)
    {
      Py_DECREF (self->type);
    }

  Py_XDECREF (self->dynamic_type);

  self->ob_type->tp_free (self);
}

/* Helper to push a Value object on the global list.  */
static void
note_value (value_object *value_obj)
{
  value_obj->next = values_in_python;
  if (value_obj->next)
    value_obj->next->prev = value_obj;
  value_obj->prev = NULL;
  values_in_python = value_obj;
}

/* Called when a new gdb.Value object needs to be allocated.  Returns NULL on
   error, with a python exception set.  */
static PyObject *
valpy_new (PyTypeObject *subtype, PyObject *args, PyObject *keywords)
{
  struct value *value = NULL;   /* Initialize to appease gcc warning.  */
  value_object *value_obj;

  if (PyTuple_Size (args) != 1)
    {
      PyErr_SetString (PyExc_TypeError, _("Value object creation takes only "
					  "1 argument"));
      return NULL;
    }

  value_obj = (value_object *) subtype->tp_alloc (subtype, 1);
  if (value_obj == NULL)
    {
      PyErr_SetString (PyExc_MemoryError, _("Could not allocate memory to "
					    "create Value object."));
      return NULL;
    }

  value = convert_value_from_python (PyTuple_GetItem (args, 0));
  if (value == NULL)
    {
      subtype->tp_free (value_obj);
      return NULL;
    }

  value_obj->value = value;
  value_incref (value);
  value_obj->address = NULL;
  value_obj->type = NULL;
  value_obj->dynamic_type = NULL;
  note_value (value_obj);

  return (PyObject *) value_obj;
}

/* Iterate over all the Value objects, calling preserve_one_value on
   each.  */
void
preserve_python_values (struct objfile *objfile, htab_t copied_types)
{
  value_object *iter;

  for (iter = values_in_python; iter; iter = iter->next)
    preserve_one_value (iter->value, objfile, copied_types);
}

/* Given a value of a pointer type, apply the C unary * operator to it.  */
static PyObject *
valpy_dereference (PyObject *self, PyObject *args)
{
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_val = value_ind (((value_object *) self)->value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (res_val);
}

/* Return "&value".  */
static PyObject *
valpy_get_address (PyObject *self, void *closure)
{
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */
  value_object *val_obj = (value_object *) self;
  volatile struct gdb_exception except;

  if (!val_obj->address)
    {
      TRY_CATCH (except, RETURN_MASK_ALL)
	{
	  res_val = value_addr (val_obj->value);
	}
      if (except.reason < 0)
	{
	  val_obj->address = Py_None;
	  Py_INCREF (Py_None);
	}
      else
	val_obj->address = value_to_value_object (res_val);
    }

  Py_INCREF (val_obj->address);

  return val_obj->address;
}

/* Return type of the value.  */
static PyObject *
valpy_get_type (PyObject *self, void *closure)
{
  value_object *obj = (value_object *) self;

  if (!obj->type)
    {
      obj->type = type_to_type_object (value_type (obj->value));
      if (!obj->type)
	return NULL;
    }
  Py_INCREF (obj->type);
  return obj->type;
}

/* Return dynamic type of the value.  */

static PyObject *
valpy_get_dynamic_type (PyObject *self, void *closure)
{
  value_object *obj = (value_object *) self;
  volatile struct gdb_exception except;
  struct type *type = NULL;

  if (obj->dynamic_type != NULL)
    {
      Py_INCREF (obj->dynamic_type);
      return obj->dynamic_type;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *val = obj->value;

      type = value_type (val);
      CHECK_TYPEDEF (type);

      if (((TYPE_CODE (type) == TYPE_CODE_PTR)
	   || (TYPE_CODE (type) == TYPE_CODE_REF))
	  && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_CLASS))
	{
	  struct value *target;
	  int was_pointer = TYPE_CODE (type) == TYPE_CODE_PTR;

	  target = value_ind (val);
	  type = value_rtti_type (target, NULL, NULL, NULL);

	  if (type)
	    {
	      if (was_pointer)
		type = lookup_pointer_type (type);
	      else
		type = lookup_reference_type (type);
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_CLASS)
	type = value_rtti_type (val, NULL, NULL, NULL);
      else
	{
	  /* Re-use object's static type.  */
	  type = NULL;
	}
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (type == NULL)
    {
      /* Ensure that the TYPE field is ready.  */
      if (!valpy_get_type (self, NULL))
	return NULL;
      /* We don't need to incref here, because valpy_get_type already
	 did it for us.  */
      obj->dynamic_type = obj->type;
    }
  else
    obj->dynamic_type = type_to_type_object (type);

  Py_INCREF (obj->dynamic_type);
  return obj->dynamic_type;
}

/* Implementation of gdb.Value.lazy_string ([encoding] [, length]) ->
   string.  Return a PyObject representing a lazy_string_object type.
   A lazy string is a pointer to a string with an optional encoding and
   length.  If ENCODING is not given, encoding is set to None.  If an
   ENCODING is provided the encoding parameter is set to ENCODING, but
   the string is not encoded.  If LENGTH is provided then the length
   parameter is set to LENGTH, otherwise length will be set to -1 (first
   null of appropriate with).  */
static PyObject *
valpy_lazy_string (PyObject *self, PyObject *args, PyObject *kw)
{
  gdb_py_longest length = -1;
  struct value *value = ((value_object *) self)->value;
  const char *user_encoding = NULL;
  static char *keywords[] = { "encoding", "length", NULL };
  PyObject *str_obj;

  if (!PyArg_ParseTupleAndKeywords (args, kw, "|s" GDB_PY_LL_ARG, keywords,
				    &user_encoding, &length))
    return NULL;

  if (TYPE_CODE (value_type (value)) == TYPE_CODE_PTR)
    value = value_ind (value);

  str_obj = gdbpy_create_lazy_string_object (value_address (value), length,
					     user_encoding,
					     value_type (value));

  return (PyObject *) str_obj;
}

/* Implementation of gdb.Value.string ([encoding] [, errors]
   [, length]) -> string.  Return Unicode string with value contents.
   If ENCODING is not given, the string is assumed to be encoded in
   the target's charset.  If LENGTH is provided, only fetch string to
   the length provided.  */

static PyObject *
valpy_string (PyObject *self, PyObject *args, PyObject *kw)
{
  int length = -1;
  gdb_byte *buffer;
  struct value *value = ((value_object *) self)->value;
  volatile struct gdb_exception except;
  PyObject *unicode;
  const char *encoding = NULL;
  const char *errors = NULL;
  const char *user_encoding = NULL;
  const char *la_encoding = NULL;
  struct type *char_type;
  static char *keywords[] = { "encoding", "errors", "length", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, kw, "|ssi", keywords,
				    &user_encoding, &errors, &length))
    return NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      LA_GET_STRING (value, &buffer, &length, &char_type, &la_encoding);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  encoding = (user_encoding && *user_encoding) ? user_encoding : la_encoding;
  unicode = PyUnicode_Decode (buffer, length * TYPE_LENGTH (char_type),
			      encoding, errors);
  xfree (buffer);

  return unicode;
}

/* A helper function that implements the various cast operators.  */

static PyObject *
valpy_do_cast (PyObject *self, PyObject *args, enum exp_opcode op)
{
  PyObject *type_obj;
  struct type *type;
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  if (! PyArg_ParseTuple (args, "O", &type_obj))
    return NULL;

  type = type_object_to_type (type_obj);
  if (! type)
    {
      PyErr_SetString (PyExc_RuntimeError, 
		       _("Argument must be a type."));
      return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *val = ((value_object *) self)->value;

      if (op == UNOP_DYNAMIC_CAST)
	res_val = value_dynamic_cast (type, val);
      else if (op == UNOP_REINTERPRET_CAST)
	res_val = value_reinterpret_cast (type, val);
      else
	{
	  gdb_assert (op == UNOP_CAST);
	  res_val = value_cast (type, val);
	}
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (res_val);
}

/* Implementation of the "cast" method.  */

static PyObject *
valpy_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_CAST);
}

/* Implementation of the "dynamic_cast" method.  */

static PyObject *
valpy_dynamic_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_DYNAMIC_CAST);
}

/* Implementation of the "reinterpret_cast" method.  */

static PyObject *
valpy_reinterpret_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_REINTERPRET_CAST);
}

static Py_ssize_t
valpy_length (PyObject *self)
{
  /* We don't support getting the number of elements in a struct / class.  */
  PyErr_SetString (PyExc_NotImplementedError,
		   _("Invalid operation on gdb.Value."));
  return -1;
}

/* Given string name of an element inside structure, return its value
   object.  Returns NULL on error, with a python exception set.  */
static PyObject *
valpy_getitem (PyObject *self, PyObject *key)
{
  value_object *self_value = (value_object *) self;
  char *field = NULL;
  struct value *res_val = NULL;
  volatile struct gdb_exception except;

  if (gdbpy_is_string (key))
    {  
      field = python_string_to_host_string (key);
      if (field == NULL)
	return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *tmp = self_value->value;

      if (field)
	res_val = value_struct_elt (&tmp, NULL, field, 0, NULL);
      else
	{
	  /* Assume we are attempting an array access, and let the
	     value code throw an exception if the index has an invalid
	     type.  */
	  struct value *idx = convert_value_from_python (key);

	  if (idx != NULL)
	    {
	      /* Check the value's type is something that can be accessed via
		 a subscript.  */
	      struct type *type;

	      tmp = coerce_ref (tmp);
	      type = check_typedef (value_type (tmp));
	      if (TYPE_CODE (type) != TYPE_CODE_ARRAY
		  && TYPE_CODE (type) != TYPE_CODE_PTR)
		  error (_("Cannot subscript requested type."));
	      else
		res_val = value_subscript (tmp, value_as_long (idx));
	    }
	}
    }

  xfree (field);
  GDB_PY_HANDLE_EXCEPTION (except);

  return res_val ? value_to_value_object (res_val) : NULL;
}

static int
valpy_setitem (PyObject *self, PyObject *key, PyObject *value)
{
  PyErr_Format (PyExc_NotImplementedError,
		_("Setting of struct elements is not currently supported."));
  return -1;
}

/* Called by the Python interpreter to perform an inferior function
   call on the value.  Returns NULL on error, with a python exception set.  */
static PyObject *
valpy_call (PyObject *self, PyObject *args, PyObject *keywords)
{
  struct value *return_value = NULL;
  Py_ssize_t args_count;
  volatile struct gdb_exception except;
  struct value *function = ((value_object *) self)->value;
  struct value **vargs = NULL;
  struct type *ftype = check_typedef (value_type (function));

  if (TYPE_CODE (ftype) != TYPE_CODE_FUNC)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Value is not callable (not TYPE_CODE_FUNC)."));
      return NULL;
    }

  args_count = PyTuple_Size (args);
  if (args_count > 0)
    {
      int i;

      vargs = alloca (sizeof (struct value *) * args_count);
      for (i = 0; i < args_count; i++)
	{
	  PyObject *item = PyTuple_GetItem (args, i);

	  if (item == NULL)
	    return NULL;

	  vargs[i] = convert_value_from_python (item);
	  if (vargs[i] == NULL)
	    return NULL;
	}
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      return_value = call_function_by_hand (function, args_count, vargs);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (return_value);
}

/* Called by the Python interpreter to obtain string representation
   of the object.  */
static PyObject *
valpy_str (PyObject *self)
{
  char *s = NULL;
  struct ui_file *stb;
  struct cleanup *old_chain;
  PyObject *result;
  struct value_print_options opts;
  volatile struct gdb_exception except;

  get_user_print_options (&opts);
  opts.deref_ref = 0;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      common_val_print (((value_object *) self)->value, stb, 0,
			&opts, python_language);
      s = ui_file_xstrdup (stb, NULL);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  do_cleanups (old_chain);

  result = PyUnicode_Decode (s, strlen (s), host_charset (), NULL);
  xfree (s);

  return result;
}

/* Implements gdb.Value.is_optimized_out.  */
static PyObject *
valpy_get_is_optimized_out (PyObject *self, void *closure)
{
  struct value *value = ((value_object *) self)->value;

  if (value_optimized_out (value))
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Calculate and return the address of the PyObject as the value of
   the builtin __hash__ call.  */
static long 
valpy_hash (PyObject *self)
{
  return (long) (intptr_t) self;
}

enum valpy_opcode
{
  VALPY_ADD,
  VALPY_SUB,
  VALPY_MUL,
  VALPY_DIV,
  VALPY_REM,
  VALPY_POW,
  VALPY_LSH,
  VALPY_RSH,
  VALPY_BITAND,
  VALPY_BITOR,
  VALPY_BITXOR
};

/* If TYPE is a reference, return the target; otherwise return TYPE.  */
#define STRIP_REFERENCE(TYPE) \
  ((TYPE_CODE (TYPE) == TYPE_CODE_REF) ? (TYPE_TARGET_TYPE (TYPE)) : (TYPE))

/* Returns a value object which is the result of applying the operation
   specified by OPCODE to the given arguments.  Returns NULL on error, with
   a python exception set.  */
static PyObject *
valpy_binop (enum valpy_opcode opcode, PyObject *self, PyObject *other)
{
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      struct value *arg1, *arg2;

      /* If the gdb.Value object is the second operand, then it will be passed
	 to us as the OTHER argument, and SELF will be an entirely different
	 kind of object, altogether.  Because of this, we can't assume self is
	 a gdb.Value object and need to convert it from python as well.  */
      arg1 = convert_value_from_python (self);
      if (arg1 == NULL)
	break;

      arg2 = convert_value_from_python (other);
      if (arg2 == NULL)
	break;

      switch (opcode)
	{
	case VALPY_ADD:
	  {
	    struct type *ltype = value_type (arg1);
	    struct type *rtype = value_type (arg2);

	    CHECK_TYPEDEF (ltype);
	    ltype = STRIP_REFERENCE (ltype);
	    CHECK_TYPEDEF (rtype);
	    rtype = STRIP_REFERENCE (rtype);

	    if (TYPE_CODE (ltype) == TYPE_CODE_PTR
		&& is_integral_type (rtype))
	      res_val = value_ptradd (arg1, value_as_long (arg2));
	    else if (TYPE_CODE (rtype) == TYPE_CODE_PTR
		     && is_integral_type (ltype))
	      res_val = value_ptradd (arg2, value_as_long (arg1));
	    else
	      res_val = value_binop (arg1, arg2, BINOP_ADD);
	  }
	  break;
	case VALPY_SUB:
	  {
	    struct type *ltype = value_type (arg1);
	    struct type *rtype = value_type (arg2);

	    CHECK_TYPEDEF (ltype);
	    ltype = STRIP_REFERENCE (ltype);
	    CHECK_TYPEDEF (rtype);
	    rtype = STRIP_REFERENCE (rtype);

	    if (TYPE_CODE (ltype) == TYPE_CODE_PTR
		&& TYPE_CODE (rtype) == TYPE_CODE_PTR)
	      /* A ptrdiff_t for the target would be preferable here.  */
	      res_val = value_from_longest (builtin_type_pyint,
					    value_ptrdiff (arg1, arg2));
	    else if (TYPE_CODE (ltype) == TYPE_CODE_PTR
		     && is_integral_type (rtype))
	      res_val = value_ptradd (arg1, - value_as_long (arg2));
	    else
	      res_val = value_binop (arg1, arg2, BINOP_SUB);
	  }
	  break;
	case VALPY_MUL:
	  res_val = value_binop (arg1, arg2, BINOP_MUL);
	  break;
	case VALPY_DIV:
	  res_val = value_binop (arg1, arg2, BINOP_DIV);
	  break;
	case VALPY_REM:
	  res_val = value_binop (arg1, arg2, BINOP_REM);
	  break;
	case VALPY_POW:
	  res_val = value_binop (arg1, arg2, BINOP_EXP);
	  break;
	case VALPY_LSH:
	  res_val = value_binop (arg1, arg2, BINOP_LSH);
	  break;
	case VALPY_RSH:
	  res_val = value_binop (arg1, arg2, BINOP_RSH);
	  break;
	case VALPY_BITAND:
	  res_val = value_binop (arg1, arg2, BINOP_BITWISE_AND);
	  break;
	case VALPY_BITOR:
	  res_val = value_binop (arg1, arg2, BINOP_BITWISE_IOR);
	  break;
	case VALPY_BITXOR:
	  res_val = value_binop (arg1, arg2, BINOP_BITWISE_XOR);
	  break;
	}
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return res_val ? value_to_value_object (res_val) : NULL;
}

static PyObject *
valpy_add (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_ADD, self, other);
}

static PyObject *
valpy_subtract (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_SUB, self, other);
}

static PyObject *
valpy_multiply (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_MUL, self, other);
}

static PyObject *
valpy_divide (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_DIV, self, other);
}

static PyObject *
valpy_remainder (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_REM, self, other);
}

static PyObject *
valpy_power (PyObject *self, PyObject *other, PyObject *unused)
{
  /* We don't support the ternary form of pow.  I don't know how to express
     that, so let's just throw NotImplementedError to at least do something
     about it.  */
  if (unused != Py_None)
    {
      PyErr_SetString (PyExc_NotImplementedError,
		       "Invalid operation on gdb.Value.");
      return NULL;
    }

  return valpy_binop (VALPY_POW, self, other);
}

static PyObject *
valpy_negative (PyObject *self)
{
  struct value *val = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      val = value_neg (((value_object *) self)->value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (val);
}

static PyObject *
valpy_positive (PyObject *self)
{
  return value_to_value_object (((value_object *) self)->value);
}

static PyObject *
valpy_absolute (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;

  if (value_less (value, value_zero (value_type (value), not_lval)))
    return valpy_negative (self);
  else
    return valpy_positive (self);
}

/* Implements boolean evaluation of gdb.Value.  */
static int
valpy_nonzero (PyObject *self)
{
  value_object *self_value = (value_object *) self;
  struct type *type;

  type = check_typedef (value_type (self_value->value));

  if (is_integral_type (type) || TYPE_CODE (type) == TYPE_CODE_PTR)
    return !!value_as_long (self_value->value);
  else if (TYPE_CODE (type) == TYPE_CODE_FLT)
    return value_as_double (self_value->value) != 0;
  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    return !decimal_is_zero (value_contents (self_value->value),
			     TYPE_LENGTH (type),
			     gdbarch_byte_order (get_type_arch (type)));
  else
    /* All other values are True.  */
    return 1;
}

/* Implements ~ for value objects.  */
static PyObject *
valpy_invert (PyObject *self)
{
  struct value *val = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      val = value_complement (((value_object *) self)->value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (val);
}

/* Implements left shift for value objects.  */
static PyObject *
valpy_lsh (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_LSH, self, other);
}

/* Implements right shift for value objects.  */
static PyObject *
valpy_rsh (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_RSH, self, other);
}

/* Implements bitwise and for value objects.  */
static PyObject *
valpy_and (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITAND, self, other);
}

/* Implements bitwise or for value objects.  */
static PyObject *
valpy_or (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITOR, self, other);
}

/* Implements bitwise xor for value objects.  */
static PyObject *
valpy_xor (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITXOR, self, other);
}

/* Implements comparison operations for value objects.  Returns NULL on error,
   with a python exception set.  */
static PyObject *
valpy_richcompare (PyObject *self, PyObject *other, int op)
{
  int result = 0;
  struct value *value_other;
  volatile struct gdb_exception except;

  if (other == Py_None)
    /* Comparing with None is special.  From what I can tell, in Python
       None is smaller than anything else.  */
    switch (op) {
      case Py_LT:
      case Py_LE:
      case Py_EQ:
	Py_RETURN_FALSE;
      case Py_NE:
      case Py_GT:
      case Py_GE:
	Py_RETURN_TRUE;
      default:
	/* Can't happen.  */
	PyErr_SetString (PyExc_NotImplementedError,
			 _("Invalid operation on gdb.Value."));
	return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      value_other = convert_value_from_python (other);
      if (value_other == NULL)
	{
	  result = -1;
	  break;
	}

      switch (op) {
        case Py_LT:
	  result = value_less (((value_object *) self)->value, value_other);
	  break;
	case Py_LE:
	  result = value_less (((value_object *) self)->value, value_other)
	    || value_equal (((value_object *) self)->value, value_other);
	  break;
	case Py_EQ:
	  result = value_equal (((value_object *) self)->value, value_other);
	  break;
	case Py_NE:
	  result = !value_equal (((value_object *) self)->value, value_other);
	  break;
        case Py_GT:
	  result = value_less (value_other, ((value_object *) self)->value);
	  break;
	case Py_GE:
	  result = value_less (value_other, ((value_object *) self)->value)
	    || value_equal (((value_object *) self)->value, value_other);
	  break;
	default:
	  /* Can't happen.  */
	  PyErr_SetString (PyExc_NotImplementedError,
			   _("Invalid operation on gdb.Value."));
	  result = -1;
	  break;
      }
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  /* In this case, the Python exception has already been set.  */
  if (result < 0)
    return NULL;

  if (result == 1)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Helper function to determine if a type is "int-like".  */
static int
is_intlike (struct type *type, int ptr_ok)
{
  CHECK_TYPEDEF (type);
  return (TYPE_CODE (type) == TYPE_CODE_INT
	  || TYPE_CODE (type) == TYPE_CODE_ENUM
	  || TYPE_CODE (type) == TYPE_CODE_BOOL
	  || TYPE_CODE (type) == TYPE_CODE_CHAR
	  || (ptr_ok && TYPE_CODE (type) == TYPE_CODE_PTR));
}

/* Implements conversion to int.  */
static PyObject *
valpy_int (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  LONGEST l = 0;
  volatile struct gdb_exception except;

  CHECK_TYPEDEF (type);
  if (!is_intlike (type, 0))
    {
      PyErr_SetString (PyExc_RuntimeError, 
		       _("Cannot convert value to int."));
      return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      l = value_as_long (value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return gdb_py_object_from_longest (l);
}

/* Implements conversion to long.  */
static PyObject *
valpy_long (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  LONGEST l = 0;
  volatile struct gdb_exception except;

  if (!is_intlike (type, 1))
    {
      PyErr_SetString (PyExc_RuntimeError, 
		       _("Cannot convert value to long."));
      return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      l = value_as_long (value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return gdb_py_long_from_longest (l);
}

/* Implements conversion to float.  */
static PyObject *
valpy_float (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  double d = 0;
  volatile struct gdb_exception except;

  CHECK_TYPEDEF (type);
  if (TYPE_CODE (type) != TYPE_CODE_FLT)
    {
      PyErr_SetString (PyExc_RuntimeError, 
		       _("Cannot convert value to float."));
      return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      d = value_as_double (value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return PyFloat_FromDouble (d);
}

/* Returns an object for a value which is released from the all_values chain,
   so its lifetime is not bound to the execution of a command.  */
PyObject *
value_to_value_object (struct value *val)
{
  value_object *val_obj;

  val_obj = PyObject_New (value_object, &value_object_type);
  if (val_obj != NULL)
    {
      val_obj->value = val;
      value_incref (val);
      val_obj->address = NULL;
      val_obj->type = NULL;
      val_obj->dynamic_type = NULL;
      note_value (val_obj);
    }

  return (PyObject *) val_obj;
}

/* Returns a borrowed reference to the struct value corresponding to
   the given value object.  */
struct value *
value_object_to_value (PyObject *self)
{
  value_object *real;

  if (! PyObject_TypeCheck (self, &value_object_type))
    return NULL;
  real = (value_object *) self;
  return real->value;
}

/* Try to convert a Python value to a gdb value.  If the value cannot
   be converted, set a Python exception and return NULL.  Returns a
   reference to a new value on the all_values chain.  */

struct value *
convert_value_from_python (PyObject *obj)
{
  struct value *value = NULL; /* -Wall */
  struct cleanup *old;
  volatile struct gdb_exception except;
  int cmp;

  gdb_assert (obj != NULL);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (PyBool_Check (obj)) 
	{
	  cmp = PyObject_IsTrue (obj);
	  if (cmp >= 0)
	    value = value_from_longest (builtin_type_pybool, cmp);
	}
      else if (PyInt_Check (obj))
	{
	  long l = PyInt_AsLong (obj);

	  if (! PyErr_Occurred ())
	    value = value_from_longest (builtin_type_pyint, l);
	}
      else if (PyLong_Check (obj))
	{
	  LONGEST l = PyLong_AsLongLong (obj);

	  if (PyErr_Occurred ())
	    {
	      /* If the error was an overflow, we can try converting to
	         ULONGEST instead.  */
	      if (PyErr_ExceptionMatches (PyExc_OverflowError))
		{
		  PyObject *etype, *evalue, *etraceback, *zero;

		  PyErr_Fetch (&etype, &evalue, &etraceback);
		  zero = PyInt_FromLong (0);

		  /* Check whether obj is positive.  */
		  if (PyObject_RichCompareBool (obj, zero, Py_GT) > 0)
		    {
		      ULONGEST ul;

		      ul = PyLong_AsUnsignedLongLong (obj);
		      if (! PyErr_Occurred ())
			value = value_from_ulongest (builtin_type_upylong, ul);
		    }
		  else
		    /* There's nothing we can do.  */
		    PyErr_Restore (etype, evalue, etraceback);

		  Py_DECREF (zero);
		}
	    }
	  else
	    value = value_from_longest (builtin_type_pylong, l);
	}
      else if (PyFloat_Check (obj))
	{
	  double d = PyFloat_AsDouble (obj);

	  if (! PyErr_Occurred ())
	    value = value_from_double (builtin_type_pyfloat, d);
	}
      else if (gdbpy_is_string (obj))
	{
	  char *s;

	  s = python_string_to_target_string (obj);
	  if (s != NULL)
	    {
	      old = make_cleanup (xfree, s);
	      value = value_cstring (s, strlen (s), builtin_type_pychar);
	      do_cleanups (old);
	    }
	}
      else if (PyObject_TypeCheck (obj, &value_object_type))
	value = value_copy (((value_object *) obj)->value);
      else if (gdbpy_is_lazy_string (obj))
	{
	  PyObject *result;

	  result = PyObject_CallMethodObjArgs (obj, gdbpy_value_cst,  NULL);
	  value = value_copy (((value_object *) result)->value);
	}
      else
	PyErr_Format (PyExc_TypeError,
		      _("Could not convert Python object: %s."),
		      PyString_AsString (PyObject_Str (obj)));
    }
  if (except.reason < 0)
    {
      PyErr_Format (except.reason == RETURN_QUIT
		    ? PyExc_KeyboardInterrupt : PyExc_RuntimeError,
		    "%s", except.message);
      return NULL;
    }

  return value;
}

/* Returns value object in the ARGth position in GDB's history.  */
PyObject *
gdbpy_history (PyObject *self, PyObject *args)
{
  int i;
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  if (!PyArg_ParseTuple (args, "i", &i))
    return NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      res_val = access_value_history (i);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (res_val);
}

/* Returns 1 in OBJ is a gdb.Value object, 0 otherwise.  */

int
gdbpy_is_value_object (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &value_object_type);
}

void
gdbpy_initialize_values (void)
{
  if (PyType_Ready (&value_object_type) < 0)
    return;

  Py_INCREF (&value_object_type);
  PyModule_AddObject (gdb_module, "Value", (PyObject *) &value_object_type);

  values_in_python = NULL;
}



static PyGetSetDef value_object_getset[] = {
  { "address", valpy_get_address, NULL, "The address of the value.",
    NULL },
  { "is_optimized_out", valpy_get_is_optimized_out, NULL,
    "Boolean telling whether the value is optimized "
    "out (i.e., not available).",
    NULL },
  { "type", valpy_get_type, NULL, "Type of the value.", NULL },
  { "dynamic_type", valpy_get_dynamic_type, NULL,
    "Dynamic type of the value.", NULL },
  {NULL}  /* Sentinel */
};

static PyMethodDef value_object_methods[] = {
  { "cast", valpy_cast, METH_VARARGS, "Cast the value to the supplied type." },
  { "dynamic_cast", valpy_dynamic_cast, METH_VARARGS,
    "dynamic_cast (gdb.Type) -> gdb.Value\n\
Cast the value to the supplied type, as if by the C++ dynamic_cast operator."
  },
  { "reinterpret_cast", valpy_reinterpret_cast, METH_VARARGS,
    "reinterpret_cast (gdb.Type) -> gdb.Value\n\
Cast the value to the supplied type, as if by the C++\n\
reinterpret_cast operator."
  },
  { "dereference", valpy_dereference, METH_NOARGS, "Dereferences the value." },
  { "lazy_string", (PyCFunction) valpy_lazy_string,
    METH_VARARGS | METH_KEYWORDS,
    "lazy_string ([encoding]  [, length]) -> lazy_string\n\
Return a lazy string representation of the value." },
  { "string", (PyCFunction) valpy_string, METH_VARARGS | METH_KEYWORDS,
    "string ([encoding] [, errors] [, length]) -> string\n\
Return Unicode string representation of the value." },
  {NULL}  /* Sentinel */
};

static PyNumberMethods value_object_as_number = {
  valpy_add,
  valpy_subtract,
  valpy_multiply,
  valpy_divide,
  valpy_remainder,
  NULL,			      /* nb_divmod */
  valpy_power,		      /* nb_power */
  valpy_negative,	      /* nb_negative */
  valpy_positive,	      /* nb_positive */
  valpy_absolute,	      /* nb_absolute */
  valpy_nonzero,	      /* nb_nonzero */
  valpy_invert,		      /* nb_invert */
  valpy_lsh,		      /* nb_lshift */
  valpy_rsh,		      /* nb_rshift */
  valpy_and,		      /* nb_and */
  valpy_xor,		      /* nb_xor */
  valpy_or,		      /* nb_or */
  NULL,			      /* nb_coerce */
  valpy_int,		      /* nb_int */
  valpy_long,		      /* nb_long */
  valpy_float,		      /* nb_float */
  NULL,			      /* nb_oct */
  NULL			      /* nb_hex */
};

static PyMappingMethods value_object_as_mapping = {
  valpy_length,
  valpy_getitem,
  valpy_setitem
};

PyTypeObject value_object_type = {
  PyObject_HEAD_INIT (NULL)
  0,				  /*ob_size*/
  "gdb.Value",			  /*tp_name*/
  sizeof (value_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  valpy_dealloc,		  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  &value_object_as_number,	  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  &value_object_as_mapping,	  /*tp_as_mapping*/
  valpy_hash,		          /*tp_hash*/
  valpy_call,	                  /*tp_call*/
  valpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES
  | Py_TPFLAGS_BASETYPE,	  /*tp_flags*/
  "GDB value object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  valpy_richcompare,		  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  value_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  value_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  valpy_new			  /* tp_new */
};

#else

void
preserve_python_values (struct objfile *objfile, htab_t copied_types)
{
  /* Nothing.  */
}

#endif /* HAVE_PYTHON */
