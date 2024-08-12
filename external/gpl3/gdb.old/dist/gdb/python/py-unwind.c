/* Python frame unwinder interface.

   Copyright (C) 2015-2023 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame-unwind.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbcmd.h"
#include "language.h"
#include "observable.h"
#include "python-internal.h"
#include "regcache.h"
#include "valprint.h"
#include "user-regs.h"

/* Debugging of Python unwinders.  */

static bool pyuw_debug;

/* Implementation of "show debug py-unwind".  */

static void
show_pyuw_debug (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Python unwinder debugging is %s.\n"), value);
}

/* Print a "py-unwind" debug statement.  */

#define pyuw_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (pyuw_debug, "py-unwind", fmt, ##__VA_ARGS__)

/* Print "py-unwind" enter/exit debug statements.  */

#define PYUW_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (pyuw_debug, "py-unwind")

struct pending_frame_object
{
  PyObject_HEAD

  /* Frame we are unwinding.  */
  frame_info_ptr frame_info;

  /* Its architecture, passed by the sniffer caller.  */
  struct gdbarch *gdbarch;
};

/* Saved registers array item.  */

struct saved_reg
{
  saved_reg (int n, gdbpy_ref<> &&v)
    : number (n),
      value (std::move (v))
  {
  }

  int number;
  gdbpy_ref<> value;
};

/* The data we keep for the PyUnwindInfo: pending_frame, saved registers
   and frame ID.  */

struct unwind_info_object
{
  PyObject_HEAD

  /* gdb.PendingFrame for the frame we are unwinding.  */
  PyObject *pending_frame;

  /* Its ID.  */
  struct frame_id frame_id;

  /* Saved registers array.  */
  std::vector<saved_reg> *saved_regs;
};

/* The data we keep for a frame we can unwind: frame ID and an array of
   (register_number, register_value) pairs.  */

struct cached_frame_info
{
  /* Frame ID.  */
  struct frame_id frame_id;

  /* GDB Architecture.  */
  struct gdbarch *gdbarch;

  /* Length of the `reg' array below.  */
  int reg_count;

  cached_reg_t reg[];
};

extern PyTypeObject pending_frame_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("pending_frame_object");

extern PyTypeObject unwind_info_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("unwind_info_object");

/* Convert gdb.Value instance to inferior's pointer.  Return 1 on success,
   0 on failure.  */

static int
pyuw_value_obj_to_pointer (PyObject *pyo_value, CORE_ADDR *addr)
{
  int rc = 0;
  struct value *value;

  try
    {
      if ((value = value_object_to_value (pyo_value)) != NULL)
	{
	  *addr = unpack_pointer (value_type (value),
				  value_contents (value).data ());
	  rc = 1;
	}
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
    }
  return rc;
}

/* Get attribute from an object and convert it to the inferior's
   pointer value.  Return 1 if attribute exists and its value can be
   converted.  Otherwise, if attribute does not exist or its value is
   None, return 0.  In all other cases set Python error and return
   0.  */

static int
pyuw_object_attribute_to_pointer (PyObject *pyo, const char *attr_name,
				  CORE_ADDR *addr)
{
  int rc = 0;

  if (PyObject_HasAttrString (pyo, attr_name))
    {
      gdbpy_ref<> pyo_value (PyObject_GetAttrString (pyo, attr_name));

      if (pyo_value != NULL && pyo_value != Py_None)
	{
	  rc = pyuw_value_obj_to_pointer (pyo_value.get (), addr);
	  if (!rc)
	    PyErr_Format (
		PyExc_ValueError,
		_("The value of the '%s' attribute is not a pointer."),
		attr_name);
	}
    }
  return rc;
}

/* Called by the Python interpreter to obtain string representation
   of the UnwindInfo object.  */

static PyObject *
unwind_infopy_str (PyObject *self)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;
  string_file stb;

  stb.printf ("Frame ID: %s", unwind_info->frame_id.to_string ().c_str ());
  {
    const char *sep = "";
    struct value_print_options opts;

    get_user_print_options (&opts);
    stb.printf ("\nSaved registers: (");
    for (const saved_reg &reg : *unwind_info->saved_regs)
      {
	struct value *value = value_object_to_value (reg.value.get ());

	stb.printf ("%s(%d, ", sep, reg.number);
	if (value != NULL)
	  {
	    try
	      {
		value_print (value, &stb, &opts);
		stb.puts (")");
	      }
	    catch (const gdb_exception &except)
	      {
		GDB_PY_HANDLE_EXCEPTION (except);
	      }
	  }
	else
	  stb.puts ("<BAD>)");
	sep = ", ";
      }
    stb.puts (")");
  }

  return PyUnicode_FromString (stb.c_str ());
}

/* Create UnwindInfo instance for given PendingFrame and frame ID.
   Sets Python error and returns NULL on error.  */

static PyObject *
pyuw_create_unwind_info (PyObject *pyo_pending_frame,
			 struct frame_id frame_id)
{
  unwind_info_object *unwind_info
      = PyObject_New (unwind_info_object, &unwind_info_object_type);

  if (((pending_frame_object *) pyo_pending_frame)->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "Attempting to use stale PendingFrame");
      return NULL;
    }
  unwind_info->frame_id = frame_id;
  Py_INCREF (pyo_pending_frame);
  unwind_info->pending_frame = pyo_pending_frame;
  unwind_info->saved_regs = new std::vector<saved_reg>;
  return (PyObject *) unwind_info;
}

/* The implementation of
   gdb.UnwindInfo.add_saved_register (REG, VALUE) -> None.  */

static PyObject *
unwind_infopy_add_saved_register (PyObject *self, PyObject *args)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;
  pending_frame_object *pending_frame
      = (pending_frame_object *) (unwind_info->pending_frame);
  PyObject *pyo_reg_id;
  PyObject *pyo_reg_value;
  int regnum;

  if (pending_frame->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "UnwindInfo instance refers to a stale PendingFrame");
      return NULL;
    }
  if (!PyArg_UnpackTuple (args, "previous_frame_register", 2, 2,
			  &pyo_reg_id, &pyo_reg_value))
    return NULL;
  if (!gdbpy_parse_register_id (pending_frame->gdbarch, pyo_reg_id, &regnum))
    return nullptr;

  /* If REGNUM identifies a user register then *maybe* we can convert this
     to a real (i.e. non-user) register.  The maybe qualifier is because we
     don't know what user registers each target might add, however, the
     following logic should work for the usual style of user registers,
     where the read function just forwards the register read on to some
     other register with no adjusting the value.  */
  if (regnum >= gdbarch_num_cooked_regs (pending_frame->gdbarch))
    {
      struct value *user_reg_value
	= value_of_user_reg (regnum, pending_frame->frame_info);
      if (VALUE_LVAL (user_reg_value) == lval_register)
	regnum = VALUE_REGNUM (user_reg_value);
      if (regnum >= gdbarch_num_cooked_regs (pending_frame->gdbarch))
	{
	  PyErr_SetString (PyExc_ValueError, "Bad register");
	  return NULL;
	}
    }

  {
    struct value *value;
    size_t data_size;

    if (pyo_reg_value == NULL
      || (value = value_object_to_value (pyo_reg_value)) == NULL)
      {
	PyErr_SetString (PyExc_ValueError, "Bad register value");
	return NULL;
      }
    data_size = register_size (pending_frame->gdbarch, regnum);
    if (data_size != value_type (value)->length ())
      {
	PyErr_Format (
	    PyExc_ValueError,
	    "The value of the register returned by the Python "
	    "sniffer has unexpected size: %u instead of %u.",
	    (unsigned) value_type (value)->length (),
	    (unsigned) data_size);
	return NULL;
      }
  }
  {
    gdbpy_ref<> new_value = gdbpy_ref<>::new_reference (pyo_reg_value);
    bool found = false;
    for (saved_reg &reg : *unwind_info->saved_regs)
      {
	if (regnum == reg.number)
	  {
	    found = true;
	    reg.value = std::move (new_value);
	    break;
	  }
      }
    if (!found)
      unwind_info->saved_regs->emplace_back (regnum, std::move (new_value));
  }
  Py_RETURN_NONE;
}

/* UnwindInfo cleanup.  */

static void
unwind_infopy_dealloc (PyObject *self)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;

  Py_XDECREF (unwind_info->pending_frame);
  delete unwind_info->saved_regs;
  Py_TYPE (self)->tp_free (self);
}

/* Called by the Python interpreter to obtain string representation
   of the PendingFrame object.  */

static PyObject *
pending_framepy_str (PyObject *self)
{
  frame_info_ptr frame = ((pending_frame_object *) self)->frame_info;
  const char *sp_str = NULL;
  const char *pc_str = NULL;

  if (frame == NULL)
    return PyUnicode_FromString ("Stale PendingFrame instance");
  try
    {
      sp_str = core_addr_to_string_nz (get_frame_sp (frame));
      pc_str = core_addr_to_string_nz (get_frame_pc (frame));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_FromFormat ("SP=%s,PC=%s", sp_str, pc_str);
}

/* Implementation of gdb.PendingFrame.read_register (self, reg) -> gdb.Value.
   Returns the value of register REG as gdb.Value instance.  */

static PyObject *
pending_framepy_read_register (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;
  struct value *val = NULL;
  int regnum;
  PyObject *pyo_reg_id;

  if (pending_frame->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "Attempting to read register from stale PendingFrame");
      return NULL;
    }
  if (!PyArg_UnpackTuple (args, "read_register", 1, 1, &pyo_reg_id))
    return NULL;
  if (!gdbpy_parse_register_id (pending_frame->gdbarch, pyo_reg_id, &regnum))
    return nullptr;

  try
    {
      /* Fetch the value associated with a register, whether it's
	 a real register or a so called "user" register, like "pc",
	 which maps to a real register.  In the past,
	 get_frame_register_value() was used here, which did not
	 handle the user register case.  */
      val = value_of_register (regnum, pending_frame->frame_info);
      if (val == NULL)
	PyErr_Format (PyExc_ValueError,
		      "Cannot read register %d from frame.",
		      regnum);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return val == NULL ? NULL : value_to_value_object (val);
}

/* Implementation of
   PendingFrame.create_unwind_info (self, frameId) -> UnwindInfo.  */

static PyObject *
pending_framepy_create_unwind_info (PyObject *self, PyObject *args)
{
  PyObject *pyo_frame_id;
  CORE_ADDR sp;
  CORE_ADDR pc;
  CORE_ADDR special;

  if (!PyArg_ParseTuple (args, "O:create_unwind_info", &pyo_frame_id))
      return NULL;
  if (!pyuw_object_attribute_to_pointer (pyo_frame_id, "sp", &sp))
    {
      PyErr_SetString (PyExc_ValueError,
		       _("frame_id should have 'sp' attribute."));
      return NULL;
    }

  /* The logic of building frame_id depending on the attributes of
     the frame_id object:
     Has     Has    Has           Function to call
     'sp'?   'pc'?  'special'?
     ------|------|--------------|-------------------------
     Y       N      *             frame_id_build_wild (sp)
     Y       Y      N             frame_id_build (sp, pc)
     Y       Y      Y             frame_id_build_special (sp, pc, special)
  */
  if (!pyuw_object_attribute_to_pointer (pyo_frame_id, "pc", &pc))
    return pyuw_create_unwind_info (self, frame_id_build_wild (sp));
  if (!pyuw_object_attribute_to_pointer (pyo_frame_id, "special", &special))
    return pyuw_create_unwind_info (self, frame_id_build (sp, pc));
  else
    return pyuw_create_unwind_info (self,
				    frame_id_build_special (sp, pc, special));
}

/* Implementation of PendingFrame.architecture (self) -> gdb.Architecture.  */

static PyObject *
pending_framepy_architecture (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  if (pending_frame->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "Attempting to read register from stale PendingFrame");
      return NULL;
    }
  return gdbarch_to_arch_object (pending_frame->gdbarch);
}

/* Implementation of PendingFrame.level (self) -> Integer.  */

static PyObject *
pending_framepy_level (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  if (pending_frame->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "Attempting to read stack level from stale PendingFrame");
      return NULL;
    }
  int level = frame_relative_level (pending_frame->frame_info);
  return gdb_py_object_from_longest (level).release ();
}

/* frame_unwind.this_id method.  */

static void
pyuw_this_id (frame_info_ptr this_frame, void **cache_ptr,
	      struct frame_id *this_id)
{
  *this_id = ((cached_frame_info *) *cache_ptr)->frame_id;
  pyuw_debug_printf ("frame_id: %s", this_id->to_string ().c_str ());
}

/* frame_unwind.prev_register.  */

static struct value *
pyuw_prev_register (frame_info_ptr this_frame, void **cache_ptr,
		    int regnum)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;

  cached_frame_info *cached_frame = (cached_frame_info *) *cache_ptr;
  cached_reg_t *reg_info = cached_frame->reg;
  cached_reg_t *reg_info_end = reg_info + cached_frame->reg_count;

  pyuw_debug_printf ("frame=%d, reg=%d",
		     frame_relative_level (this_frame), regnum);
  for (; reg_info < reg_info_end; ++reg_info)
    {
      if (regnum == reg_info->num)
	return frame_unwind_got_bytes (this_frame, regnum, reg_info->data);
    }

  return frame_unwind_got_optimized (this_frame, regnum);
}

/* Frame sniffer dispatch.  */

static int
pyuw_sniffer (const struct frame_unwind *self, frame_info_ptr this_frame,
	      void **cache_ptr)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;

  struct gdbarch *gdbarch = (struct gdbarch *) (self->unwind_data);
  cached_frame_info *cached_frame;

  gdbpy_enter enter_py (gdbarch);

  pyuw_debug_printf ("frame=%d, sp=%s, pc=%s",
		     frame_relative_level (this_frame),
		     paddress (gdbarch, get_frame_sp (this_frame)),
		     paddress (gdbarch, get_frame_pc (this_frame)));

  /* Create PendingFrame instance to pass to sniffers.  */
  pending_frame_object *pfo = PyObject_New (pending_frame_object,
					    &pending_frame_object_type);
  gdbpy_ref<> pyo_pending_frame ((PyObject *) pfo);
  if (pyo_pending_frame == NULL)
    {
      gdbpy_print_stack ();
      return 0;
    }
  pfo->gdbarch = gdbarch;
  scoped_restore invalidate_frame = make_scoped_restore (&pfo->frame_info,
							 this_frame);

  /* Run unwinders.  */
  if (gdb_python_module == NULL
      || ! PyObject_HasAttrString (gdb_python_module, "_execute_unwinders"))
    {
      PyErr_SetString (PyExc_NameError,
		       "Installation error: gdb._execute_unwinders function "
		       "is missing");
      gdbpy_print_stack ();
      return 0;
    }
  gdbpy_ref<> pyo_execute (PyObject_GetAttrString (gdb_python_module,
						   "_execute_unwinders"));
  if (pyo_execute == nullptr)
    {
      gdbpy_print_stack ();
      return 0;
    }

  /* A (gdb.UnwindInfo, str) tuple, or None.  */
  gdbpy_ref<> pyo_execute_ret
    (PyObject_CallFunctionObjArgs (pyo_execute.get (),
				   pyo_pending_frame.get (), NULL));
  if (pyo_execute_ret == nullptr)
    {
      /* If the unwinder is cancelled due to a Ctrl-C, then propagate
	 the Ctrl-C as a GDB exception instead of swallowing it.  */
      gdbpy_print_stack_or_quit ();
      return 0;
    }
  if (pyo_execute_ret == Py_None)
    return 0;

  /* Verify the return value of _execute_unwinders is a tuple of size 2.  */
  gdb_assert (PyTuple_Check (pyo_execute_ret.get ()));
  gdb_assert (PyTuple_GET_SIZE (pyo_execute_ret.get ()) == 2);

  if (pyuw_debug)
    {
      PyObject *pyo_unwinder_name = PyTuple_GET_ITEM (pyo_execute_ret.get (), 1);
      gdb::unique_xmalloc_ptr<char> name
	= python_string_to_host_string (pyo_unwinder_name);

      /* This could happen if the user passed something else than a string
	 as the unwinder's name.  */
      if (name == nullptr)
	{
	  gdbpy_print_stack ();
	  name = make_unique_xstrdup ("<failed to get unwinder name>");
	}

      pyuw_debug_printf ("frame claimed by unwinder %s", name.get ());
    }

  /* Received UnwindInfo, cache data.  */
  PyObject *pyo_unwind_info = PyTuple_GET_ITEM (pyo_execute_ret.get (), 0);
  if (PyObject_IsInstance (pyo_unwind_info,
			   (PyObject *) &unwind_info_object_type) <= 0)
    error (_("A Unwinder should return gdb.UnwindInfo instance."));

  {
    unwind_info_object *unwind_info =
      (unwind_info_object *) pyo_unwind_info;
    int reg_count = unwind_info->saved_regs->size ();

    cached_frame
      = ((cached_frame_info *)
	 xmalloc (sizeof (*cached_frame)
		  + reg_count * sizeof (cached_frame->reg[0])));
    cached_frame->gdbarch = gdbarch;
    cached_frame->frame_id = unwind_info->frame_id;
    cached_frame->reg_count = reg_count;

    /* Populate registers array.  */
    for (int i = 0; i < unwind_info->saved_regs->size (); ++i)
      {
	saved_reg *reg = &(*unwind_info->saved_regs)[i];

	struct value *value = value_object_to_value (reg->value.get ());
	size_t data_size = register_size (gdbarch, reg->number);

	cached_frame->reg[i].num = reg->number;

	/* `value' validation was done before, just assert.  */
	gdb_assert (value != NULL);
	gdb_assert (data_size == value_type (value)->length ());

	cached_frame->reg[i].data = (gdb_byte *) xmalloc (data_size);
	memcpy (cached_frame->reg[i].data,
		value_contents (value).data (), data_size);
      }
  }

  *cache_ptr = cached_frame;
  return 1;
}

/* Frame cache release shim.  */

static void
pyuw_dealloc_cache (frame_info *this_frame, void *cache)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;
  cached_frame_info *cached_frame = (cached_frame_info *) cache;

  for (int i = 0; i < cached_frame->reg_count; i++)
    xfree (cached_frame->reg[i].data);

  xfree (cache);
}

struct pyuw_gdbarch_data_type
{
  /* Has the unwinder shim been prepended? */
  int unwinder_registered = 0;
};

static const registry<gdbarch>::key<pyuw_gdbarch_data_type> pyuw_gdbarch_data;

/* New inferior architecture callback: register the Python unwinders
   intermediary.  */

static void
pyuw_on_new_gdbarch (struct gdbarch *newarch)
{
  struct pyuw_gdbarch_data_type *data = pyuw_gdbarch_data.get (newarch);
  if (data == nullptr)
    data= pyuw_gdbarch_data.emplace (newarch);

  if (!data->unwinder_registered)
    {
      struct frame_unwind *unwinder
	  = GDBARCH_OBSTACK_ZALLOC (newarch, struct frame_unwind);

      unwinder->name = "python";
      unwinder->type = NORMAL_FRAME;
      unwinder->stop_reason = default_frame_unwind_stop_reason;
      unwinder->this_id = pyuw_this_id;
      unwinder->prev_register = pyuw_prev_register;
      unwinder->unwind_data = (const struct frame_data *) newarch;
      unwinder->sniffer = pyuw_sniffer;
      unwinder->dealloc_cache = pyuw_dealloc_cache;
      frame_unwind_prepend_unwinder (newarch, unwinder);
      data->unwinder_registered = 1;
    }
}

void _initialize_py_unwind ();
void
_initialize_py_unwind ()
{
  add_setshow_boolean_cmd
      ("py-unwind", class_maintenance, &pyuw_debug,
	_("Set Python unwinder debugging."),
	_("Show Python unwinder debugging."),
	_("When on, Python unwinder debugging is enabled."),
	NULL,
	show_pyuw_debug,
	&setdebuglist, &showdebuglist);
}

/* Initialize unwind machinery.  */

int
gdbpy_initialize_unwind (void)
{
  gdb::observers::architecture_changed.attach (pyuw_on_new_gdbarch,
					       "py-unwind");

  if (PyType_Ready (&pending_frame_object_type) < 0)
    return -1;
  int rc = gdb_pymodule_addobject (gdb_module, "PendingFrame",
				   (PyObject *) &pending_frame_object_type);
  if (rc != 0)
    return rc;

  if (PyType_Ready (&unwind_info_object_type) < 0)
    return -1;
  return gdb_pymodule_addobject (gdb_module, "UnwindInfo",
      (PyObject *) &unwind_info_object_type);
}

static PyMethodDef pending_frame_object_methods[] =
{
  { "read_register", pending_framepy_read_register, METH_VARARGS,
    "read_register (REG) -> gdb.Value\n"
    "Return the value of the REG in the frame." },
  { "create_unwind_info",
    pending_framepy_create_unwind_info, METH_VARARGS,
    "create_unwind_info (FRAME_ID) -> gdb.UnwindInfo\n"
    "Construct UnwindInfo for this PendingFrame, using FRAME_ID\n"
    "to identify it." },
  { "architecture",
    pending_framepy_architecture, METH_NOARGS,
    "architecture () -> gdb.Architecture\n"
    "The architecture for this PendingFrame." },
  { "level", pending_framepy_level, METH_NOARGS,
    "The stack level of this frame." },
  {NULL}  /* Sentinel */
};

PyTypeObject pending_frame_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.PendingFrame",             /* tp_name */
  sizeof (pending_frame_object),  /* tp_basicsize */
  0,                              /* tp_itemsize */
  0,                              /* tp_dealloc */
  0,                              /* tp_print */
  0,                              /* tp_getattr */
  0,                              /* tp_setattr */
  0,                              /* tp_compare */
  0,                              /* tp_repr */
  0,                              /* tp_as_number */
  0,                              /* tp_as_sequence */
  0,                              /* tp_as_mapping */
  0,                              /* tp_hash  */
  0,                              /* tp_call */
  pending_framepy_str,            /* tp_str */
  0,                              /* tp_getattro */
  0,                              /* tp_setattro */
  0,                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,             /* tp_flags */
  "GDB PendingFrame object",      /* tp_doc */
  0,                              /* tp_traverse */
  0,                              /* tp_clear */
  0,                              /* tp_richcompare */
  0,                              /* tp_weaklistoffset */
  0,                              /* tp_iter */
  0,                              /* tp_iternext */
  pending_frame_object_methods,   /* tp_methods */
  0,                              /* tp_members */
  0,                              /* tp_getset */
  0,                              /* tp_base */
  0,                              /* tp_dict */
  0,                              /* tp_descr_get */
  0,                              /* tp_descr_set */
  0,                              /* tp_dictoffset */
  0,                              /* tp_init */
  0,                              /* tp_alloc */
};

static PyMethodDef unwind_info_object_methods[] =
{
  { "add_saved_register",
    unwind_infopy_add_saved_register, METH_VARARGS,
    "add_saved_register (REG, VALUE) -> None\n"
    "Set the value of the REG in the previous frame to VALUE." },
  { NULL }  /* Sentinel */
};

PyTypeObject unwind_info_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.UnwindInfo",               /* tp_name */
  sizeof (unwind_info_object),    /* tp_basicsize */
  0,                              /* tp_itemsize */
  unwind_infopy_dealloc,          /* tp_dealloc */
  0,                              /* tp_print */
  0,                              /* tp_getattr */
  0,                              /* tp_setattr */
  0,                              /* tp_compare */
  0,                              /* tp_repr */
  0,                              /* tp_as_number */
  0,                              /* tp_as_sequence */
  0,                              /* tp_as_mapping */
  0,                              /* tp_hash  */
  0,                              /* tp_call */
  unwind_infopy_str,              /* tp_str */
  0,                              /* tp_getattro */
  0,                              /* tp_setattro */
  0,                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
  "GDB UnwindInfo object",        /* tp_doc */
  0,                              /* tp_traverse */
  0,                              /* tp_clear */
  0,                              /* tp_richcompare */
  0,                              /* tp_weaklistoffset */
  0,                              /* tp_iter */
  0,                              /* tp_iternext */
  unwind_info_object_methods,     /* tp_methods */
  0,                              /* tp_members */
  0,                              /* tp_getset */
  0,                              /* tp_base */
  0,                              /* tp_dict */
  0,                              /* tp_descr_get */
  0,                              /* tp_descr_set */
  0,                              /* tp_dictoffset */
  0,                              /* tp_init */
  0,                              /* tp_alloc */
};
