/* General utility routines for GDB/Python.

   Copyright (C) 2008-2017 Free Software Foundation, Inc.

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
#include "py-ref.h"

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
static gdb::unique_xmalloc_ptr<char>
unicode_to_encoded_string (PyObject *unicode_str, const char *charset)
{
  gdb::unique_xmalloc_ptr<char> result;

  /* Translate string to named charset.  */
  gdbpy_ref<> string (PyUnicode_AsEncodedString (unicode_str, charset, NULL));
  if (string == NULL)
    return NULL;

#ifdef IS_PY3K
  result.reset (xstrdup (PyBytes_AsString (string.get ())));
#else
  result.reset (xstrdup (PyString_AsString (string.get ())));
#endif

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

/* Returns a newly allocated string with the contents of the given
   unicode string object converted to the target's charset.  If an
   error occurs during the conversion, NULL will be returned and a
   python exception will be set.  */
gdb::unique_xmalloc_ptr<char>
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
   the target's charset.  Returns NULL on error, with a python
   exception set.  */
gdb::unique_xmalloc_ptr<char>
python_string_to_target_string (PyObject *obj)
{
  gdbpy_ref<> str (python_string_to_unicode (obj));
  if (str == NULL)
    return NULL;

  return unicode_to_target_string (str.get ());
}

/* Converts a python string (8-bit or unicode) to a target string in the
   target's charset.  Returns NULL on error, with a python exception
   set.

   In Python 3, the returned object is a "bytes" object (not a string).  */
PyObject *
python_string_to_target_python_string (PyObject *obj)
{
  gdbpy_ref<> str (python_string_to_unicode (obj));
  if (str == NULL)
    return NULL;

  return unicode_to_target_python_string (str.get ());
}

/* Converts a python string (8-bit or unicode) to a target string in
   the host's charset.  Returns NULL on error, with a python exception
   set.  */
gdb::unique_xmalloc_ptr<char>
python_string_to_host_string (PyObject *obj)
{
  gdbpy_ref<> str (python_string_to_unicode (obj));
  if (str == NULL)
    return NULL;

  return unicode_to_encoded_string (str.get (), host_charset ());
}

/* Convert a host string to a python string.  */

PyObject *
host_string_to_python_string (const char *str)
{
  return PyString_Decode (str, strlen (str), host_charset (), NULL);
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
   If the result is NULL a python error occurred, the caller must clear it.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_obj_to_string (PyObject *obj)
{
  gdbpy_ref<> str_obj (PyObject_Str (obj));

  if (str_obj != NULL)
    {
      gdb::unique_xmalloc_ptr<char> msg;

#ifdef IS_PY3K
      msg = python_string_to_host_string (str_obj.get ());
#else
      msg.reset (xstrdup (PyString_AsString (str_obj.get ())));
#endif

      return msg;
    }

  return NULL;
}

/* Return the string representation of the exception represented by
   TYPE, VALUE which is assumed to have been obtained with PyErr_Fetch,
   i.e., the error indicator is currently clear.
   If the result is NULL a python error occurred, the caller must clear it.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_exception_to_string (PyObject *ptype, PyObject *pvalue)
{
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
    return gdbpy_obj_to_string (pvalue);
  else
    return gdbpy_obj_to_string (ptype);
}

/* Convert a GDB exception to the appropriate Python exception.

   This sets the Python error indicator.  */

void
gdbpy_convert_exception (struct gdb_exception exception)
{
  PyObject *exc_class;

  if (exception.reason == RETURN_QUIT)
    exc_class = PyExc_KeyboardInterrupt;
  else if (exception.error == MEMORY_ERROR)
    exc_class = gdbpy_gdb_memory_error;
  else
    exc_class = gdbpy_gdb_error;

  PyErr_Format (exc_class, "%s", exception.message);
}

/* Converts OBJ to a CORE_ADDR value.

   Returns 0 on success or -1 on failure, with a Python exception set.
*/

int
get_addr_from_python (PyObject *obj, CORE_ADDR *addr)
{
  if (gdbpy_is_value_object (obj))
    {

      TRY
	{
	  *addr = value_as_address (value_object_to_value (obj));
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	  GDB_PY_SET_HANDLE_EXCEPTION (except);
	}
      END_CATCH
    }
  else
    {
      gdbpy_ref<> num (PyNumber_Long (obj));
      gdb_py_ulongest val;

      if (num == NULL)
	return -1;

      val = gdb_py_long_as_ulongest (num.get ());
      if (PyErr_Occurred ())
	return -1;

      if (sizeof (val) > sizeof (CORE_ADDR) && ((CORE_ADDR) val) != val)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Overflow converting to address."));
	  return -1;
	}

      *addr = val;
    }

  return 0;
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
  PyTypeObject *type_obj = (PyTypeObject *) closure;
  char *raw_ptr;

  raw_ptr = (char *) self + type_obj->tp_dictoffset;
  result = * (PyObject **) raw_ptr;

  Py_INCREF (result);
  return result;
}

/* Like PyModule_AddObject, but does not steal a reference to
   OBJECT.  */

int
gdb_pymodule_addobject (PyObject *module, const char *name, PyObject *object)
{
  int result;

  Py_INCREF (object);
  /* Python 2.4 did not have a 'const' here.  */
  result = PyModule_AddObject (module, (char *) name, object);
  if (result < 0)
    Py_DECREF (object);
  return result;
}
