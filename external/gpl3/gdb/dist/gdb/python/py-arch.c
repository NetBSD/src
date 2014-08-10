/* Python interface to architecture

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "arch-utils.h"
#include "disasm.h"
#include "python-internal.h"

typedef struct arch_object_type_object {
  PyObject_HEAD
  struct gdbarch *gdbarch;
} arch_object;

static struct gdbarch_data *arch_object_data = NULL;

/* Require a valid Architecture.  */
#define ARCHPY_REQUIRE_VALID(arch_obj, arch)			\
  do {								\
    arch = arch_object_to_gdbarch (arch_obj);			\
    if (arch == NULL)						\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Architecture is invalid."));	\
	return NULL;						\
      }								\
  } while (0)

static PyTypeObject arch_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("arch_object");

/* Associates an arch_object with GDBARCH as gdbarch_data via the gdbarch
   post init registration mechanism (gdbarch_data_register_post_init).  */

static void *
arch_object_data_init (struct gdbarch *gdbarch)
{
  arch_object *arch_obj = PyObject_New (arch_object, &arch_object_type);

  if (arch_obj == NULL)
    return NULL;

  arch_obj->gdbarch = gdbarch;

  return (void *) arch_obj;
}

/* Returns the struct gdbarch value corresponding to the given Python
   architecture object OBJ.  */

struct gdbarch *
arch_object_to_gdbarch (PyObject *obj)
{
  arch_object *py_arch = (arch_object *) obj;

  return py_arch->gdbarch;
}

/* Returns the Python architecture object corresponding to GDBARCH.
   Returns a new reference to the arch_object associated as data with
   GDBARCH.  */

PyObject *
gdbarch_to_arch_object (struct gdbarch *gdbarch)
{
  PyObject *new_ref = (PyObject *) gdbarch_data (gdbarch, arch_object_data);

  /* new_ref could be NULL if registration of arch_object with GDBARCH failed
     in arch_object_data_init.  */
  Py_XINCREF (new_ref);

  return new_ref;
}

/* Implementation of gdb.Architecture.name (self) -> String.
   Returns the name of the architecture as a string value.  */

static PyObject *
archpy_name (PyObject *self, PyObject *args)
{
  struct gdbarch *gdbarch = NULL;
  const char *name;
  PyObject *py_name;

  ARCHPY_REQUIRE_VALID (self, gdbarch);

  name = (gdbarch_bfd_arch_info (gdbarch))->printable_name;
  py_name = PyString_FromString (name);

  return py_name;
}

/* Implementation of
   gdb.Architecture.disassemble (self, start_pc [, end_pc [,count]]) -> List.
   Returns a list of instructions in a memory address range.  Each instruction
   in the list is a Python dict object.
*/

static PyObject *
archpy_disassemble (PyObject *self, PyObject *args, PyObject *kw)
{
  static char *keywords[] = { "start_pc", "end_pc", "count", NULL };
  CORE_ADDR start, end = 0;
  CORE_ADDR pc;
  gdb_py_ulongest start_temp;
  long count = 0, i;
  PyObject *result_list, *end_obj = NULL, *count_obj = NULL;
  struct gdbarch *gdbarch = NULL;

  ARCHPY_REQUIRE_VALID (self, gdbarch);

  if (!PyArg_ParseTupleAndKeywords (args, kw, GDB_PY_LLU_ARG "|OO", keywords,
                                    &start_temp, &end_obj, &count_obj))
    return NULL;

  start = start_temp;
  if (end_obj)
    {
      /* Make a long logic check first.  In Python 3.x, internally,
	 all integers are represented as longs.  In Python 2.x, there
	 is still a differentiation internally between a PyInt and a
	 PyLong.  Explicitly do this long check conversion first. In
	 GDB, for Python 3.x, we #ifdef PyInt = PyLong.  This check has
	 to be done first to ensure we do not lose information in the
	 conversion process.  */
      if (PyLong_Check (end_obj))
        end = PyLong_AsUnsignedLongLong (end_obj);
      else if (PyInt_Check (end_obj))
        /* If the end_pc value is specified without a trailing 'L', end_obj will
           be an integer and not a long integer.  */
        end = PyInt_AsLong (end_obj);
      else
        {
          Py_DECREF (end_obj);
          Py_XDECREF (count_obj);
          PyErr_SetString (PyExc_TypeError,
                           _("Argument 'end_pc' should be a (long) integer."));

          return NULL;
        }

      if (end < start)
        {
          Py_DECREF (end_obj);
          Py_XDECREF (count_obj);
          PyErr_SetString (PyExc_ValueError,
                           _("Argument 'end_pc' should be greater than or "
                             "equal to the argument 'start_pc'."));

          return NULL;
        }
    }
  if (count_obj)
    {
      count = PyInt_AsLong (count_obj);
      if (PyErr_Occurred () || count < 0)
        {
          Py_DECREF (count_obj);
          Py_XDECREF (end_obj);
          PyErr_SetString (PyExc_TypeError,
                           _("Argument 'count' should be an non-negative "
                             "integer."));

          return NULL;
        }
    }

  result_list = PyList_New (0);
  if (result_list == NULL)
    return NULL;

  for (pc = start, i = 0;
       /* All args are specified.  */
       (end_obj && count_obj && pc <= end && i < count)
       /* end_pc is specified, but no count.  */
       || (end_obj && count_obj == NULL && pc <= end)
       /* end_pc is not specified, but a count is.  */
       || (end_obj == NULL && count_obj && i < count)
       /* Both end_pc and count are not specified.  */
       || (end_obj == NULL && count_obj == NULL && pc == start);)
    {
      int insn_len = 0;
      char *as = NULL;
      struct ui_file *memfile = mem_fileopen ();
      PyObject *insn_dict = PyDict_New ();
      volatile struct gdb_exception except;

      if (insn_dict == NULL)
        {
          Py_DECREF (result_list);
          ui_file_delete (memfile);

          return NULL;
        }
      if (PyList_Append (result_list, insn_dict))
        {
          Py_DECREF (result_list);
          Py_DECREF (insn_dict);
          ui_file_delete (memfile);

          return NULL;  /* PyList_Append Sets the exception.  */
        }

      TRY_CATCH (except, RETURN_MASK_ALL)
        {
          insn_len = gdb_print_insn (gdbarch, pc, memfile, NULL);
        }
      if (except.reason < 0)
        {
          Py_DECREF (result_list);
          ui_file_delete (memfile);

	  gdbpy_convert_exception (except);
	  return NULL;
        }

      as = ui_file_xstrdup (memfile, NULL);
      if (PyDict_SetItemString (insn_dict, "addr",
                                gdb_py_long_from_ulongest (pc))
          || PyDict_SetItemString (insn_dict, "asm",
                                   PyString_FromString (*as ? as : "<unknown>"))
          || PyDict_SetItemString (insn_dict, "length",
                                   PyInt_FromLong (insn_len)))
        {
          Py_DECREF (result_list);

          ui_file_delete (memfile);
          xfree (as);

          return NULL;
        }

      pc += insn_len;
      i++;
      ui_file_delete (memfile);
      xfree (as);
    }

  return result_list;
}

/* Initializes the Architecture class in the gdb module.  */

int
gdbpy_initialize_arch (void)
{
  arch_object_data = gdbarch_data_register_post_init (arch_object_data_init);
  arch_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&arch_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Architecture",
				 (PyObject *) &arch_object_type);
}

static PyMethodDef arch_object_methods [] = {
  { "name", archpy_name, METH_NOARGS,
    "name () -> String.\n\
Return the name of the architecture as a string value." },
  { "disassemble", (PyCFunction) archpy_disassemble,
    METH_VARARGS | METH_KEYWORDS,
    "disassemble (start_pc [, end_pc [, count]]) -> List.\n\
Return a list of at most COUNT disassembled instructions from START_PC to\n\
END_PC." },
  {NULL}  /* Sentinel */
};

static PyTypeObject arch_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Architecture",                 /* tp_name */
  sizeof (arch_object),               /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash  */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  "GDB architecture object",          /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  arch_object_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
};
