/* General utility routines for GDB/Python.

   Copyright (C) 2008-2013 Free Software Foundation, Inc.

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
#include "charset.h"
#include "value.h"
#include "python-internal.h"


/* This is a cleanup function which decrements the refcount on a
   Python object.  */

static void
py_decref (void *p)
{
  PyObject *py = p;

  /* Note that we need the extra braces in this 'if' to avoid a
     warning from gcc.  */
  if (py)
    {
      Py_DECREF (py);
    }
}

/* Return a new cleanup which will decrement the Python object's
   refcount when run.  */

struct cleanup *
make_cleanup_py_decref (PyObject *py)
{
  return make_cleanup (py_decref, (void *) py);
}

/* Converts a Python 8-bit string to a unicode string object.  Assumes the
   8-bit string is in the host charset.  If an error occurs during conversion,
   returns NULL with a python exception set.

   As an added bonus, the functions accepts a unicode string and returns it
   right away, so callers don't need to check which kind of string they've
   got.  In Python 3, all strings are Unicode so this case is always the 
   one that applies.

   If the given object is not one of the mentioned string types, NULL is
   returned, with the TypeError python exception set.  */
PyObject *
python_string_to_unicode (PyObject *obj)
{
  PyObject *unicode_str;

  /* If obj is already a unicode string, just return it.
     I wish life was always that simple...  */
  if (PyUnicode_Check (obj))
    {
      unicode_str = obj;
      Py_INCREF (obj);
    }
#ifndef IS_PY3K
  else if (PyString_Check (obj))
    unicode_str = PyUnicode_FromEncodedObject (obj, host_charset (), NULL);
#endif
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Expected a string or unicode object."));
      unicode_str = NULL;
    }

  return unicode_str;
}

/* Returns a newly allocated string with the contents of the given unicode
   string object converted to CHARSET.  If an error occurs during the
   conversion, NULL will be returned and a python exception will be set.

   The caller is responsible for xfree'ing the string.  */
static char *
unicode_to_encoded_string (PyObject *unicode_str, const char *charset)
{
  char *result;
  PyObject *string;

  /* Translate string to named charset.  */
  string = PyUnicode_AsEncodedString (unicode_str, charset, NULL);
  if (string == NULL)
    return NULL;

#ifdef IS_PY3K
  result = xstrdup (PyBytes_AsString (string));
#else
  result = xstrdup (PyString_AsString (string));
#endif

  Py_DECREF (string);

  return result;
}

/* Returns a PyObject with the contents of the given unicode string
   object converted to a named charset.  If an error occurs during
   the conversion, NULL will be returned and a python exception will
   be set.  */
static PyObject *
unicode_to_encoded_python_string (PyObject *unicode_str, const char *charset)
{
  /* Translate string to named charset.  */
  return PyUnicode_AsEncodedString (unicode_str, charset, NULL);
}

/* Returns a newly allocated string with the contents of the given unicode
   string object converted to the target's charset.  If an error occurs during
   the conversion, NULL will be returned and a python exception will be set.

   The caller is responsible for xfree'ing the string.  */
char *
unicode_to_target_string (PyObject *unicode_str)
{
  return unicode_to_encoded_string (unicode_str,
				    target_charset (python_gdbarch));
}

/* Returns a PyObject with the contents of the given unicode string
   object converted to the target's charset.  If an error occurs
   during the conversion, NULL will be returned and a python exception
   will be set.  */
static PyObject *
unicode_to_target_python_string (PyObject *unicode_str)
{
  return unicode_to_encoded_python_string (unicode_str,
					   target_charset (python_gdbarch));
}

/* Converts a python string (8-bit or unicode) to a target string in
   the target's charset.  Returns NULL on error, with a python exception set.

   The caller is responsible for xfree'ing the string.  */
char *
python_string_to_target_string (PyObject *obj)
{
  PyObject *str;
  char *result;

  str = python_string_to_unicode (obj);
  if (str == NULL)
    return NULL;

  result = unicode_to_target_string (str);
  Py_DECREF (str);
  return result;
}

/* Converts a python string (8-bit or unicode) to a target string in the
   target's charset.  Returns NULL on error, with a python exception
   set.

   In Python 3, the returned object is a "bytes" object (not a string).  */
PyObject *
python_string_to_target_python_string (PyObject *obj)
{
  PyObject *str;
  PyObject *result;

  str = python_string_to_unicode (obj);
  if (str == NULL)
    return NULL;

  result = unicode_to_target_python_string (str);
  Py_DECREF (str);
  return result;
}

/* Converts a python string (8-bit or unicode) to a target string in
   the host's charset.  Returns NULL on error, with a python exception set.

   The caller is responsible for xfree'ing the string.  */
char *
python_string_to_host_string (PyObject *obj)
{
  PyObject *str;
  char *result;

  str = python_string_to_unicode (obj);
  if (str == NULL)
    return NULL;

  result = unicode_to_encoded_string (str, host_charset ()); 
  Py_DECREF (str);
  return result;
}

/* Return true if OBJ is a Python string or unicode object, false
   otherwise.  */

int
gdbpy_is_string (PyObject *obj)
{
#ifdef IS_PY3K
  return PyUnicode_Check (obj);
#else
  return PyString_Check (obj) || PyUnicode_Check (obj);
#endif
}

/* Return the string representation of OBJ, i.e., str (obj).
   Space for the result is malloc'd, the caller must free.
   If the result is NULL a python error occurred, the caller must clear it.  */

char *
gdbpy_obj_to_string (PyObject *obj)
{
  PyObject *str_obj = PyObject_Str (obj);

  if (str_obj != NULL)
    {
#ifdef IS_PY3K
      char *msg = python_string_to_host_string (str_obj);
#else
      char *msg = xstrdup (PyString_AsString (str_obj));
#endif

      Py_DECREF (str_obj);
      return msg;
    }

  return NULL;
}

/* Return the string representation of the exception represented by
   TYPE, VALUE which is assumed to have been obtained with PyErr_Fetch,
   i.e., the error indicator is currently clear.
   Space for the result is malloc'd, the caller must free.
   If the result is NULL a python error occurred, the caller must clear it.  */

char *
gdbpy_exception_to_string (PyObject *ptype, PyObject *pvalue)
{
  char *str;

  /* There are a few cases to consider.
     For example:
     pvalue is a string when PyErr_SetString is used.
     pvalue is not a string when raise "foo" is used, instead it is None
     and ptype is "foo".
     So the algorithm we use is to print `str (pvalue)' if it's not
     None, otherwise we print `str (ptype)'.
     Using str (aka PyObject_Str) will fetch the error message from
     gdb.GdbError ("message").  */

  if (pvalue && pvalue != Py_None)
    str = gdbpy_obj_to_string (pvalue);
  else
    str = gdbpy_obj_to_string (ptype);

  return str;
}

/* Convert a GDB exception to the appropriate Python exception.
   
   This sets the Python error indicator, and returns NULL.  */

PyObject *
gdbpy_convert_exception (struct gdb_exception exception)
{
  PyObject *exc_class;

  if (exception.reason == RETURN_QUIT)
    exc_class = PyExc_KeyboardInterrupt;
  else if (exception.error == MEMORY_ERROR)
    exc_class = gdbpy_gdb_memory_error;
  else
    exc_class = gdbpy_gdb_error;

  return PyErr_Format (exc_class, "%s", exception.message);
}

/* Converts OBJ to a CORE_ADDR value.

   Returns 1 on success or 0 on failure, with a Python exception set.  This
   function can also throw GDB exceptions.
*/

int
get_addr_from_python (PyObject *obj, CORE_ADDR *addr)
{
  if (gdbpy_is_value_object (obj))
    *addr = value_as_address (value_object_to_value (obj));
  else
    {
      PyObject *num = PyNumber_Long (obj);
      gdb_py_ulongest val;

      if (num == NULL)
	return 0;

      val = gdb_py_long_as_ulongest (num);
      Py_XDECREF (num);
      if (PyErr_Occurred ())
	return 0;

      if (sizeof (val) > sizeof (CORE_ADDR) && ((CORE_ADDR) val) != val)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Overflow converting to address."));
	  return 0;
	}

      *addr = val;
    }

  return 1;
}

/* Convert a LONGEST to the appropriate Python object -- either an
   integer object or a long object, depending on its value.  */

PyObject *
gdb_py_object_from_longest (LONGEST l)
{
#ifdef IS_PY3K
  if (sizeof (l) > sizeof (long))
    return PyLong_FromLongLong (l);
  return PyLong_FromLong (l);
#else
#ifdef HAVE_LONG_LONG		/* Defined by Python.  */
  /* If we have 'long long', and the value overflows a 'long', use a
     Python Long; otherwise use a Python Int.  */
  if (sizeof (l) > sizeof (long)
      && (l > PyInt_GetMax () || l < (- (LONGEST) PyInt_GetMax ()) - 1))
    return PyLong_FromLongLong (l);
#endif
  return PyInt_FromLong (l);
#endif
}

/* Convert a ULONGEST to the appropriate Python object -- either an
   integer object or a long object, depending on its value.  */

PyObject *
gdb_py_object_from_ulongest (ULONGEST l)
{
#ifdef IS_PY3K
  if (sizeof (l) > sizeof (unsigned long))
    return PyLong_FromUnsignedLongLong (l);
  return PyLong_FromUnsignedLong (l);
#else
#ifdef HAVE_LONG_LONG		/* Defined by Python.  */
  /* If we have 'long long', and the value overflows a 'long', use a
     Python Long; otherwise use a Python Int.  */
  if (sizeof (l) > sizeof (unsigned long) && l > PyInt_GetMax ())
    return PyLong_FromUnsignedLongLong (l);
#endif

  if (l > PyInt_GetMax ())
    return PyLong_FromUnsignedLong (l);

  return PyInt_FromLong (l);
#endif
}

/* Like PyInt_AsLong, but returns 0 on failure, 1 on success, and puts
   the value into an out parameter.  */

int
gdb_py_int_as_long (PyObject *obj, long *result)
{
  *result = PyInt_AsLong (obj);
  return ! (*result == -1 && PyErr_Occurred ());
}



/* Generic implementation of the __dict__ attribute for objects that
   have a dictionary.  The CLOSURE argument should be the type object.
   This only handles positive values for tp_dictoffset.  */

PyObject *
gdb_py_generic_dict (PyObject *self, void *closure)
{
  PyObject *result;
  PyTypeObject *type_obj = closure;
  char *raw_ptr;

  raw_ptr = (char *) self + type_obj->tp_dictoffset;
  result = * (PyObject **) raw_ptr;

  Py_INCREF (result);
  return result;
}
