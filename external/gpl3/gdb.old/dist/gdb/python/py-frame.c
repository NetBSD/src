/* Python interface to stack frames

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

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
#include "block.h"
#include "frame.h"
#include "exceptions.h"
#include "symtab.h"
#include "stack.h"
#include "value.h"
#include "python-internal.h"
#include "symfile.h"
#include "objfiles.h"

typedef struct {
  PyObject_HEAD
  struct frame_id frame_id;
  struct gdbarch *gdbarch;

  /* Marks that the FRAME_ID member actually holds the ID of the frame next
     to this, and not this frames' ID itself.  This is a hack to permit Python
     frame objects which represent invalid frames (i.e., the last frame_info
     in a corrupt stack).  The problem arises from the fact that this code
     relies on FRAME_ID to uniquely identify a frame, which is not always true
     for the last "frame" in a corrupt stack (it can have a null ID, or the same
     ID as the  previous frame).  Whenever get_prev_frame returns NULL, we
     record the frame_id of the next frame and set FRAME_ID_IS_NEXT to 1.  */
  int frame_id_is_next;
} frame_object;

/* Require a valid frame.  This must be called inside a TRY_CATCH, or
   another context in which a gdb exception is allowed.  */
#define FRAPY_REQUIRE_VALID(frame_obj, frame)		\
    do {						\
      frame = frame_object_to_frame_info (frame_obj);	\
      if (frame == NULL)				\
	error (_("Frame is invalid."));			\
    } while (0)

/* Returns the frame_info object corresponding to the given Python Frame
   object.  If the frame doesn't exist anymore (the frame id doesn't
   correspond to any frame in the inferior), returns NULL.  */

struct frame_info *
frame_object_to_frame_info (PyObject *obj)
{
  frame_object *frame_obj = (frame_object *) obj;
  struct frame_info *frame;

  frame = frame_find_by_id (frame_obj->frame_id);
  if (frame == NULL)
    return NULL;

  if (frame_obj->frame_id_is_next)
    frame = get_prev_frame (frame);

  return frame;
}

/* Called by the Python interpreter to obtain string representation
   of the object.  */

static PyObject *
frapy_str (PyObject *self)
{
  char *s;
  PyObject *result;
  struct ui_file *strfile;

  strfile = mem_fileopen ();
  fprint_frame_id (strfile, ((frame_object *) self)->frame_id);
  s = ui_file_xstrdup (strfile, NULL);
  result = PyString_FromString (s);
  xfree (s);

  return result;
}

/* Implementation of gdb.Frame.is_valid (self) -> Boolean.
   Returns True if the frame corresponding to the frame_id of this
   object still exists in the inferior.  */

static PyObject *
frapy_is_valid (PyObject *self, PyObject *args)
{
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frame_object_to_frame_info (self);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (frame == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Frame.name (self) -> String.
   Returns the name of the function corresponding to this frame.  */

static PyObject *
frapy_name (PyObject *self, PyObject *args)
{
  struct frame_info *frame;
  char *name = NULL;
  enum language lang;
  PyObject *result;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      find_frame_funname (frame, &name, &lang, NULL);
    }

  if (except.reason < 0)
    xfree (name);

  GDB_PY_HANDLE_EXCEPTION (except);

  if (name)
    {
      result = PyUnicode_Decode (name, strlen (name), host_charset (), NULL);
      xfree (name);
    }
  else
    {
      result = Py_None;
      Py_INCREF (Py_None);
    }

  return result;
}

/* Implementation of gdb.Frame.type (self) -> Integer.
   Returns the frame type, namely one of the gdb.*_FRAME constants.  */

static PyObject *
frapy_type (PyObject *self, PyObject *args)
{
  struct frame_info *frame;
  enum frame_type type = NORMAL_FRAME;/* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      type = get_frame_type (frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return PyInt_FromLong (type);
}

/* Implementation of gdb.Frame.architecture (self) -> gdb.Architecture.
   Returns the frame's architecture as a gdb.Architecture object.  */

static PyObject *
frapy_arch (PyObject *self, PyObject *args)
{
  struct frame_info *frame = NULL;    /* Initialize to appease gcc warning.  */
  frame_object *obj = (frame_object *) self;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return gdbarch_to_arch_object (obj->gdbarch);
}

/* Implementation of gdb.Frame.unwind_stop_reason (self) -> Integer.
   Returns one of the gdb.FRAME_UNWIND_* constants.  */

static PyObject *
frapy_unwind_stop_reason (PyObject *self, PyObject *args)
{
  struct frame_info *frame = NULL;    /* Initialize to appease gcc warning.  */
  volatile struct gdb_exception except;
  enum unwind_stop_reason stop_reason;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  stop_reason = get_frame_unwind_stop_reason (frame);

  return PyInt_FromLong (stop_reason);
}

/* Implementation of gdb.Frame.pc (self) -> Long.
   Returns the frame's resume address.  */

static PyObject *
frapy_pc (PyObject *self, PyObject *args)
{
  CORE_ADDR pc = 0;	      /* Initialize to appease gcc warning.  */
  struct frame_info *frame;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      pc = get_frame_pc (frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return gdb_py_long_from_ulongest (pc);
}

/* Implementation of gdb.Frame.block (self) -> gdb.Block.
   Returns the frame's code block.  */

static PyObject *
frapy_block (PyObject *self, PyObject *args)
{
  struct frame_info *frame;
  struct block *block = NULL, *fn_block;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);
      block = get_frame_block (frame, NULL);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  for (fn_block = block;
       fn_block != NULL && BLOCK_FUNCTION (fn_block) == NULL;
       fn_block = BLOCK_SUPERBLOCK (fn_block))
    ;

  if (block == NULL || fn_block == NULL || BLOCK_FUNCTION (fn_block) == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Cannot locate block for frame."));
      return NULL;
    }

  if (block)
    {
      struct symtab *symt;

      symt = SYMBOL_SYMTAB (BLOCK_FUNCTION (fn_block));
      return block_to_block_object (block, symt->objfile);
    }

  Py_RETURN_NONE;
}


/* Implementation of gdb.Frame.function (self) -> gdb.Symbol.
   Returns the symbol for the function corresponding to this frame.  */

static PyObject *
frapy_function (PyObject *self, PyObject *args)
{
  struct symbol *sym = NULL;
  struct frame_info *frame;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      sym = find_pc_function (get_frame_address_in_block (frame));
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (sym)
    return symbol_to_symbol_object (sym);

  Py_RETURN_NONE;
}

/* Convert a frame_info struct to a Python Frame object.
   Sets a Python exception and returns NULL on error.  */

PyObject *
frame_info_to_frame_object (struct frame_info *frame)
{
  frame_object *frame_obj;
  volatile struct gdb_exception except;

  frame_obj = PyObject_New (frame_object, &frame_object_type);
  if (frame_obj == NULL)
    return NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {

      /* Try to get the previous frame, to determine if this is the last frame
	 in a corrupt stack.  If so, we need to store the frame_id of the next
	 frame and not of this one (which is possibly invalid).  */
      if (get_prev_frame (frame) == NULL
	  && get_frame_unwind_stop_reason (frame) != UNWIND_NO_REASON
	  && get_next_frame (frame) != NULL)
	{
	  frame_obj->frame_id = get_frame_id (get_next_frame (frame));
	  frame_obj->frame_id_is_next = 1;
	}
      else
	{
	  frame_obj->frame_id = get_frame_id (frame);
	  frame_obj->frame_id_is_next = 0;
	}
      frame_obj->gdbarch = get_frame_arch (frame);
    }
  if (except.reason < 0)
    {
      Py_DECREF (frame_obj);
      gdbpy_convert_exception (except);
      return NULL;
    }
  return (PyObject *) frame_obj;
}

/* Implementation of gdb.Frame.older (self) -> gdb.Frame.
   Returns the frame immediately older (outer) to this frame, or None if
   there isn't one.  */

static PyObject *
frapy_older (PyObject *self, PyObject *args)
{
  struct frame_info *frame, *prev = NULL;
  volatile struct gdb_exception except;
  PyObject *prev_obj = NULL;   /* Initialize to appease gcc warning.  */

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      prev = get_prev_frame (frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (prev)
    prev_obj = (PyObject *) frame_info_to_frame_object (prev);
  else
    {
      Py_INCREF (Py_None);
      prev_obj = Py_None;
    }

  return prev_obj;
}

/* Implementation of gdb.Frame.newer (self) -> gdb.Frame.
   Returns the frame immediately newer (inner) to this frame, or None if
   there isn't one.  */

static PyObject *
frapy_newer (PyObject *self, PyObject *args)
{
  struct frame_info *frame, *next = NULL;
  volatile struct gdb_exception except;
  PyObject *next_obj = NULL;   /* Initialize to appease gcc warning.  */

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      next = get_next_frame (frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (next)
    next_obj = (PyObject *) frame_info_to_frame_object (next);
  else
    {
      Py_INCREF (Py_None);
      next_obj = Py_None;
    }

  return next_obj;
}

/* Implementation of gdb.Frame.find_sal (self) -> gdb.Symtab_and_line.
   Returns the frame's symtab and line.  */

static PyObject *
frapy_find_sal (PyObject *self, PyObject *args)
{
  struct frame_info *frame;
  struct symtab_and_line sal;
  volatile struct gdb_exception except;
  PyObject *sal_obj = NULL;   /* Initialize to appease gcc warning.  */

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      find_frame_sal (frame, &sal);
      sal_obj = symtab_and_line_to_sal_object (sal);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return sal_obj;
}

/* Implementation of gdb.Frame.read_var_value (self, variable,
   [block]) -> gdb.Value.  If the optional block argument is provided
   start the search from that block, otherwise search from the frame's
   current block (determined by examining the resume address of the
   frame).  The variable argument must be a string or an instance of a
   gdb.Symbol.  The block argument must be an instance of gdb.Block.  Returns
   NULL on error, with a python exception set.  */
static PyObject *
frapy_read_var (PyObject *self, PyObject *args)
{
  struct frame_info *frame;
  PyObject *sym_obj, *block_obj = NULL;
  struct symbol *var = NULL;	/* gcc-4.3.2 false warning.  */
  struct value *val = NULL;
  volatile struct gdb_exception except;

  if (!PyArg_ParseTuple (args, "O|O", &sym_obj, &block_obj))
    return NULL;

  if (PyObject_TypeCheck (sym_obj, &symbol_object_type))
    var = symbol_object_to_symbol (sym_obj);
  else if (gdbpy_is_string (sym_obj))
    {
      char *var_name;
      const struct block *block = NULL;
      struct cleanup *cleanup;
      volatile struct gdb_exception except;

      var_name = python_string_to_target_string (sym_obj);
      if (!var_name)
	return NULL;
      cleanup = make_cleanup (xfree, var_name);

      if (block_obj)
	{
	  block = block_object_to_block (block_obj);
	  if (!block)
	    {
	      PyErr_SetString (PyExc_RuntimeError,
			       _("Second argument must be block."));
	      do_cleanups (cleanup);
	      return NULL;
	    }
	}

      TRY_CATCH (except, RETURN_MASK_ALL)
	{
	  FRAPY_REQUIRE_VALID (self, frame);

	  if (!block)
	    block = get_frame_block (frame, NULL);
	  var = lookup_symbol (var_name, block, VAR_DOMAIN, NULL);
	}
      if (except.reason < 0)
	{
	  do_cleanups (cleanup);
	  gdbpy_convert_exception (except);
	  return NULL;
	}

      if (!var)
	{
	  PyErr_Format (PyExc_ValueError,
			_("Variable '%s' not found."), var_name);
	  do_cleanups (cleanup);

	  return NULL;
	}

      do_cleanups (cleanup);
    }
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Argument must be a symbol or string."));
      return NULL;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, frame);

      val = read_var_value (var, frame);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (val);
}

/* Select this frame.  */

static PyObject *
frapy_select (PyObject *self, PyObject *args)
{
  struct frame_info *fi;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      FRAPY_REQUIRE_VALID (self, fi);

      select_frame (fi);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  Py_RETURN_NONE;
}

/* Implementation of gdb.newest_frame () -> gdb.Frame.
   Returns the newest frame object.  */

PyObject *
gdbpy_newest_frame (PyObject *self, PyObject *args)
{
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = get_current_frame ();
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return frame_info_to_frame_object (frame);
}

/* Implementation of gdb.selected_frame () -> gdb.Frame.
   Returns the selected frame object.  */

PyObject *
gdbpy_selected_frame (PyObject *self, PyObject *args)
{
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = get_selected_frame ("No frame is currently selected.");
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return frame_info_to_frame_object (frame);
}

/* Implementation of gdb.stop_reason_string (Integer) -> String.
   Return a string explaining the unwind stop reason.  */

PyObject *
gdbpy_frame_stop_reason_string (PyObject *self, PyObject *args)
{
  int reason;
  const char *str;

  if (!PyArg_ParseTuple (args, "i", &reason))
    return NULL;

  if (reason < UNWIND_FIRST || reason > UNWIND_LAST)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Invalid frame stop reason."));
      return NULL;
    }

  str = frame_stop_reason_string (reason);
  return PyUnicode_Decode (str, strlen (str), host_charset (), NULL);
}

/* Implements the equality comparison for Frame objects.
   All other comparison operators will throw a TypeError Python exception,
   as they aren't valid for frames.  */

static PyObject *
frapy_richcompare (PyObject *self, PyObject *other, int op)
{
  int result;

  if (!PyObject_TypeCheck (other, &frame_object_type)
      || (op != Py_EQ && op != Py_NE))
    {
      Py_INCREF (Py_NotImplemented);
      return Py_NotImplemented;
    }

  if (frame_id_eq (((frame_object *) self)->frame_id,
		   ((frame_object *) other)->frame_id))
    result = Py_EQ;
  else
    result = Py_NE;

  if (op == result)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Sets up the Frame API in the gdb module.  */

int
gdbpy_initialize_frames (void)
{
  frame_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&frame_object_type) < 0)
    return -1;

  /* Note: These would probably be best exposed as class attributes of
     Frame, but I don't know how to do it except by messing with the
     type's dictionary.  That seems too messy.  */
  if (PyModule_AddIntConstant (gdb_module, "NORMAL_FRAME", NORMAL_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "DUMMY_FRAME", DUMMY_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "INLINE_FRAME", INLINE_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "TAILCALL_FRAME",
				  TAILCALL_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "SIGTRAMP_FRAME",
				  SIGTRAMP_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "ARCH_FRAME", ARCH_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "SENTINEL_FRAME",
				  SENTINEL_FRAME) < 0)
    return -1;

#define SET(name, description) \
  if (PyModule_AddIntConstant (gdb_module, "FRAME_"#name, name) < 0) \
    return -1;
#include "unwind_stop_reasons.def"
#undef SET

  return gdb_pymodule_addobject (gdb_module, "Frame",
				 (PyObject *) &frame_object_type);
}



static PyMethodDef frame_object_methods[] = {
  { "is_valid", frapy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this frame is valid, false if not." },
  { "name", frapy_name, METH_NOARGS,
    "name () -> String.\n\
Return the function name of the frame, or None if it can't be determined." },
  { "type", frapy_type, METH_NOARGS,
    "type () -> Integer.\n\
Return the type of the frame." },
  { "architecture", frapy_arch, METH_NOARGS,
    "architecture () -> gdb.Architecture.\n\
Return the architecture of the frame." },
  { "unwind_stop_reason", frapy_unwind_stop_reason, METH_NOARGS,
    "unwind_stop_reason () -> Integer.\n\
Return the reason why it's not possible to find frames older than this." },
  { "pc", frapy_pc, METH_NOARGS,
    "pc () -> Long.\n\
Return the frame's resume address." },
  { "block", frapy_block, METH_NOARGS,
    "block () -> gdb.Block.\n\
Return the frame's code block." },
  { "function", frapy_function, METH_NOARGS,
    "function () -> gdb.Symbol.\n\
Returns the symbol for the function corresponding to this frame." },
  { "older", frapy_older, METH_NOARGS,
    "older () -> gdb.Frame.\n\
Return the frame that called this frame." },
  { "newer", frapy_newer, METH_NOARGS,
    "newer () -> gdb.Frame.\n\
Return the frame called by this frame." },
  { "find_sal", frapy_find_sal, METH_NOARGS,
    "find_sal () -> gdb.Symtab_and_line.\n\
Return the frame's symtab and line." },
  { "read_var", frapy_read_var, METH_VARARGS,
    "read_var (variable) -> gdb.Value.\n\
Return the value of the variable in this frame." },
  { "select", frapy_select, METH_NOARGS,
    "Select this frame as the user's current frame." },
  {NULL}  /* Sentinel */
};

PyTypeObject frame_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Frame",			  /* tp_name */
  sizeof (frame_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  0,				  /* tp_dealloc */
  0,				  /* tp_print */
  0,				  /* tp_getattr */
  0,				  /* tp_setattr */
  0,				  /* tp_compare */
  0,				  /* tp_repr */
  0,				  /* tp_as_number */
  0,				  /* tp_as_sequence */
  0,				  /* tp_as_mapping */
  0,				  /* tp_hash  */
  0,				  /* tp_call */
  frapy_str,			  /* tp_str */
  0,				  /* tp_getattro */
  0,				  /* tp_setattro */
  0,				  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,		  /* tp_flags */
  "GDB frame object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  frapy_richcompare,		  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  frame_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
};
