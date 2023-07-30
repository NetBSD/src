/* MI Command Set for GDB, the GNU debugger.

   Copyright (C) 2019-2023 Free Software Foundation, Inc.

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

/* GDB/MI commands implemented in Python.  */

#include "defs.h"
#include "python-internal.h"
#include "arch-utils.h"
#include "charset.h"
#include "language.h"
#include "mi/mi-cmds.h"
#include "mi/mi-parse.h"
#include "cli/cli-cmds.h"
#include <string>

/* Debugging of Python MI commands.  */

static bool pymicmd_debug;

/* Implementation of "show debug py-micmd".  */

static void
show_pymicmd_debug (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Python MI command debugging is %s.\n"), value);
}

/* Print a "py-micmd" debug statement.  */

#define pymicmd_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (pymicmd_debug, "py-micmd", fmt, ##__VA_ARGS__)

/* Print a "py-micmd" enter/exit debug statements.  */

#define PYMICMD_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (pymicmd_debug, "py-micmd")

struct mi_command_py;

/* Representation of a Python gdb.MICommand object.  */

struct micmdpy_object
{
  PyObject_HEAD

  /* The object representing this command in the MI command table.  This
     pointer can be nullptr if the command is not currently installed into
     the MI command table (see gdb.MICommand.installed property).  */
  struct mi_command_py *mi_command;

  /* The string representing the name of this command, without the leading
     dash.  This string is never nullptr once the Python object has been
     initialised.

     The memory for this string was allocated with malloc, and needs to be
     deallocated with free when the Python object is deallocated.

     When the MI_COMMAND field is not nullptr, then the mi_command_py
     object's name will point back to this string.  */
  char *mi_command_name;
};

/* The MI command implemented in Python.  */

struct mi_command_py : public mi_command
{
  /* Constructs a new mi_command_py object.  NAME is command name without
     leading dash.  OBJECT is a reference to a Python object implementing
     the command.  This object must inherit from gdb.MICommand and must
     implement the invoke method.  */

  mi_command_py (const char *name, micmdpy_object *object)
    : mi_command (name, nullptr),
      m_pyobj (gdbpy_ref<micmdpy_object>::new_reference (object))
  {
    pymicmd_debug_printf ("this = %p", this);
    m_pyobj->mi_command = this;
  }

  ~mi_command_py ()
  {
    /* The Python object representing a MI command contains a pointer back
       to this c++ object.  We can safely set this pointer back to nullptr
       now, to indicate the Python object no longer references a valid c++
       object.

       However, the Python object also holds the storage for our name
       string.  We can't clear that here as our parent's destructor might
       still want to reference that string.  Instead we rely on the Python
       object deallocator to free that memory, and reset the pointer.  */
    m_pyobj->mi_command = nullptr;

    pymicmd_debug_printf ("this = %p", this);
  };

  /* Validate that CMD_OBJ, a non-nullptr pointer, is installed into the MI
     command table correctly.  This function looks up the command in the MI
     command table and checks that the object we get back references
     CMD_OBJ.  This function is only intended for calling within a
     gdb_assert.  This function performs many assertions internally, and
     then always returns true.  */
  static void validate_installation (micmdpy_object *cmd_obj);

  /* Update M_PYOBJ to NEW_PYOBJ.  The pointer from M_PYOBJ that points
     back to this object is swapped with the pointer in NEW_PYOBJ, which
     must be nullptr, so that NEW_PYOBJ now points back to this object.
     Additionally our parent's name string is stored in M_PYOBJ, so we
     swap the name string with NEW_PYOBJ.

     Before this call M_PYOBJ is the Python object representing this MI
     command object.  After this call has completed, NEW_PYOBJ now
     represents this MI command object.  */
  void swap_python_object (micmdpy_object *new_pyobj)
  {
    /* Current object has a backlink, new object doesn't have a backlink.  */
    gdb_assert (m_pyobj->mi_command != nullptr);
    gdb_assert (new_pyobj->mi_command == nullptr);

    /* Clear the current M_PYOBJ's backlink, set NEW_PYOBJ's backlink.  */
    std::swap (new_pyobj->mi_command, m_pyobj->mi_command);

    /* Both object have names.  */
    gdb_assert (m_pyobj->mi_command_name != nullptr);
    gdb_assert (new_pyobj->mi_command_name != nullptr);

    /* mi_command::m_name is the string owned by the current object.  */
    gdb_assert (m_pyobj->mi_command_name == this->name ());

    /* The name in mi_command::m_name is owned by the current object.  Rather
       than changing the value of mi_command::m_name (which is not accessible
       from here) to point to the name owned by the new object, swap the names
       of the two objects, since we know they are identical strings.  */
    gdb_assert (strcmp (new_pyobj->mi_command_name,
			m_pyobj->mi_command_name) == 0);
    std::swap (new_pyobj->mi_command_name, m_pyobj->mi_command_name);

    /* Take a reference to the new object, drop the reference to the current
       object.  */
    m_pyobj = gdbpy_ref<micmdpy_object>::new_reference (new_pyobj);
  }

  /* Called when the MI command is invoked.  */
  virtual void invoke(struct mi_parse *parse) const override;

private:
  /* The Python object representing this MI command.  */
  gdbpy_ref<micmdpy_object> m_pyobj;
};

using mi_command_py_up = std::unique_ptr<mi_command_py>;

extern PyTypeObject micmdpy_object_type
	CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("micmdpy_object");

/* Holds a Python object containing the string 'invoke'.  */

static PyObject *invoke_cst;

/* Convert KEY_OBJ into a string that can be used as a field name in MI
   output.  KEY_OBJ must be a Python string object, and must only contain
   characters suitable for use as an MI field name.

   If KEY_OBJ is not a string, or if KEY_OBJ contains invalid characters,
   then an error is thrown.  Otherwise, KEY_OBJ is converted to a string
   and returned.  */

static gdb::unique_xmalloc_ptr<char>
py_object_to_mi_key (PyObject *key_obj)
{
  /* The key must be a string.  */
  if (!PyUnicode_Check (key_obj))
    {
      gdbpy_ref<> key_repr (PyObject_Repr (key_obj));
      gdb::unique_xmalloc_ptr<char> key_repr_string;
      if (key_repr != nullptr)
	key_repr_string = python_string_to_target_string (key_repr.get ());
      if (key_repr_string == nullptr)
	gdbpy_handle_exception ();

      gdbpy_error (_("non-string object used as key: %s"),
		   key_repr_string.get ());
    }

  gdb::unique_xmalloc_ptr<char> key_string
    = python_string_to_target_string (key_obj);
  if (key_string == nullptr)
    gdbpy_handle_exception ();

  /* Predicate function, returns true if NAME is a valid field name for use
     in MI result output, otherwise, returns false.  */
  auto is_valid_key_name = [] (const char *name) -> bool
  {
    gdb_assert (name != nullptr);

    if (*name == '\0' || !isalpha (*name))
      return false;

    for (; *name != '\0'; ++name)
      if (!isalnum (*name) && *name != '_' && *name != '-')
	return false;

    return true;
  };

  if (!is_valid_key_name (key_string.get ()))
    {
      if (*key_string.get () == '\0')
	gdbpy_error (_("Invalid empty key in MI result"));
      else
	gdbpy_error (_("Invalid key in MI result: %s"), key_string.get ());
    }

  return key_string;
}

/* Serialize RESULT and print it in MI format to the current_uiout.
   FIELD_NAME is used as the name of this result field.

   RESULT can be a dictionary, a sequence, an iterator, or an object that
   can be converted to a string, these are converted to the matching MI
   output format (dictionaries as tuples, sequences and iterators as lists,
   and strings as named fields).

   If anything goes wrong while formatting the output then an error is
   thrown.

   This function is the recursive inner core of serialize_mi_result, and
   should only be called from that function.  */

static void
serialize_mi_result_1 (PyObject *result, const char *field_name)
{
  struct ui_out *uiout = current_uiout;

  if (PyDict_Check (result))
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      ui_out_emit_tuple tuple_emitter (uiout, field_name);
      while (PyDict_Next (result, &pos, &key, &value))
	{
	  gdb::unique_xmalloc_ptr<char> key_string
	    (py_object_to_mi_key (key));
	  serialize_mi_result_1 (value, key_string.get ());
	}
    }
  else if (PySequence_Check (result) && !PyUnicode_Check (result))
    {
      ui_out_emit_list list_emitter (uiout, field_name);
      Py_ssize_t len = PySequence_Size (result);
      if (len == -1)
	gdbpy_handle_exception ();
      for (Py_ssize_t i = 0; i < len; ++i)
	{
          gdbpy_ref<> item (PySequence_ITEM (result, i));
	  if (item == nullptr)
	    gdbpy_handle_exception ();
	  serialize_mi_result_1 (item.get (), nullptr);
	}
    }
  else if (PyIter_Check (result))
    {
      gdbpy_ref<> item;
      ui_out_emit_list list_emitter (uiout, field_name);
      while (true)
	{
	  item.reset (PyIter_Next (result));
	  if (item == nullptr)
	    {
	      if (PyErr_Occurred () != nullptr)
		gdbpy_handle_exception ();
	      break;
	    }
	  serialize_mi_result_1 (item.get (), nullptr);
	}
    }
  else
    {
      gdb::unique_xmalloc_ptr<char> string (gdbpy_obj_to_string (result));
      if (string == nullptr)
	gdbpy_handle_exception ();
      uiout->field_string (field_name, string.get ());
    }
}

/* Serialize RESULT and print it in MI format to the current_uiout.

   This function handles the top-level result initially returned from the
   invoke method of the Python command implementation.  At the top-level
   the result must be a dictionary.  The values within this dictionary can
   be a wider range of types.  Handling the values of the top-level
   dictionary is done by serialize_mi_result_1, see that function for more
   details.

   If anything goes wrong while parsing and printing the MI output then an
   error is thrown.  */

static void
serialize_mi_result (PyObject *result)
{
  /* At the top-level, the result must be a dictionary.  */

  if (!PyDict_Check (result))
    gdbpy_error (_("Result from invoke must be a dictionary"));

  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next (result, &pos, &key, &value))
    {
      gdb::unique_xmalloc_ptr<char> key_string
	(py_object_to_mi_key (key));
      serialize_mi_result_1 (value, key_string.get ());
    }
}

/* Called when the MI command is invoked.  PARSE contains the parsed
   command line arguments from the user.  */

void
mi_command_py::invoke (struct mi_parse *parse) const
{
  PYMICMD_SCOPED_DEBUG_ENTER_EXIT;

  pymicmd_debug_printf ("this = %p, name = %s", this, name ());

  mi_parse_argv (parse->args, parse);

  if (parse->argv == nullptr)
    error (_("Problem parsing arguments: %s %s"), parse->command, parse->args);


  gdbpy_enter enter_py;

  /* Place all the arguments into a list which we pass as a single argument
     to the MI command's invoke method.  */
  gdbpy_ref<> argobj (PyList_New (parse->argc));
  if (argobj == nullptr)
    gdbpy_handle_exception ();

  for (int i = 0; i < parse->argc; ++i)
    {
      gdbpy_ref<> str (PyUnicode_Decode (parse->argv[i],
					 strlen (parse->argv[i]),
					 host_charset (), nullptr));
      if (PyList_SetItem (argobj.get (), i, str.release ()) < 0)
	gdbpy_handle_exception ();
    }

  gdb_assert (this->m_pyobj != nullptr);
  gdb_assert (PyErr_Occurred () == nullptr);
  gdbpy_ref<> result
    (PyObject_CallMethodObjArgs ((PyObject *) this->m_pyobj.get (), invoke_cst,
				 argobj.get (), nullptr));
  if (result == nullptr)
    gdbpy_handle_exception ();

  if (result != Py_None)
    serialize_mi_result (result.get ());
}

/* See declaration above.  */

void
mi_command_py::validate_installation (micmdpy_object *cmd_obj)
{
  gdb_assert (cmd_obj != nullptr);
  mi_command_py *cmd = cmd_obj->mi_command;
  gdb_assert (cmd != nullptr);
  const char *name = cmd_obj->mi_command_name;
  gdb_assert (name != nullptr);
  gdb_assert (name == cmd->name ());
  mi_command *mi_cmd = mi_cmd_lookup (name);
  gdb_assert (mi_cmd == cmd);
  gdb_assert (cmd->m_pyobj == cmd_obj);
}

/* Return CMD as an mi_command_py if it is a Python MI command, else
   nullptr.  */

static mi_command_py *
as_mi_command_py (mi_command *cmd)
{
  return dynamic_cast<mi_command_py *> (cmd);
}

/* Uninstall OBJ, making the MI command represented by OBJ unavailable for
   use by the user.  On success 0 is returned, otherwise -1 is returned
   and a Python exception will be set.  */

static int
micmdpy_uninstall_command (micmdpy_object *obj)
{
  PYMICMD_SCOPED_DEBUG_ENTER_EXIT;

  gdb_assert (obj->mi_command != nullptr);
  gdb_assert (obj->mi_command_name != nullptr);

  pymicmd_debug_printf ("name = %s", obj->mi_command_name);

  /* Remove the command from the internal MI table of commands.  This will
     cause the mi_command_py object to be deleted, which will clear the
     backlink in OBJ.  */
  bool removed = remove_mi_cmd_entry (obj->mi_command->name ());
  gdb_assert (removed);
  gdb_assert (obj->mi_command == nullptr);

  return 0;
}

/* Install OBJ as a usable MI command.  Return 0 on success, and -1 on
   error, in which case, a Python error will have been set.

   After successful completion the command name associated with OBJ will
   be installed in the MI command table (so it can be found if the user
   enters that command name), additionally, OBJ will have been added to
   the gdb._mi_commands dictionary (using the command name as its key),
   this will ensure that OBJ remains live even if the user gives up all
   references.  */

static int
micmdpy_install_command (micmdpy_object *obj)
{
  PYMICMD_SCOPED_DEBUG_ENTER_EXIT;

  gdb_assert (obj->mi_command == nullptr);
  gdb_assert (obj->mi_command_name != nullptr);

  pymicmd_debug_printf ("name = %s", obj->mi_command_name);

  /* Look up this command name in MI_COMMANDS, a command with this name may
     already exist.  */
  mi_command *cmd = mi_cmd_lookup (obj->mi_command_name);
  mi_command_py *cmd_py = as_mi_command_py (cmd);

  if (cmd != nullptr && cmd_py == nullptr)
    {
      /* There is already an MI command registered with that name, and it's not
         a Python one.  Forbid replacing a non-Python MI command.  */
      PyErr_SetString (PyExc_RuntimeError,
		       _("unable to add command, name is already in use"));
      return -1;
    }

  if (cmd_py != nullptr)
    {
      /* There is already a Python MI command registered with that name, swap
         in the new gdb.MICommand implementation.  */
      cmd_py->swap_python_object (obj);
    }
  else
    {
      /* There's no MI command registered with that name at all, create one.  */
      mi_command_py_up mi_cmd (new mi_command_py (obj->mi_command_name, obj));

      /* Add the command to the gdb internal MI command table.  */
      bool result = insert_mi_cmd_entry (std::move (mi_cmd));
      gdb_assert (result);
    }

  return 0;
}

/* Implement gdb.MICommand.__init__.  The init method takes the name of
   the MI command as the first argument, which must be a string, starting
   with a single dash.  */

static int
micmdpy_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
  PYMICMD_SCOPED_DEBUG_ENTER_EXIT;

  micmdpy_object *cmd = (micmdpy_object *) self;

  static const char *keywords[] = { "name", nullptr };
  const char *name;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "s", keywords,
					&name))
    return -1;

  /* Validate command name */
  const int name_len = strlen (name);
  if (name_len == 0)
    {
      PyErr_SetString (PyExc_ValueError, _("MI command name is empty."));
      return -1;
    }
  else if ((name_len < 2) || (name[0] != '-') || !isalnum (name[1]))
    {
      PyErr_SetString (PyExc_ValueError,
		       _("MI command name does not start with '-'"
			 " followed by at least one letter or digit."));
      return -1;
    }
  else
    {
      for (int i = 2; i < name_len; i++)
	{
	  if (!isalnum (name[i]) && name[i] != '-')
	    {
	      PyErr_Format
		(PyExc_ValueError,
		 _("MI command name contains invalid character: %c."),
		 name[i]);
	      return -1;
	    }
	}

      /* Skip over the leading dash.  For the rest of this function the
	 dash is not important.  */
      ++name;
    }

  /* If this object already has a name set, then this object has been
     initialized before.  We handle this case a little differently.  */
  if (cmd->mi_command_name != nullptr)
    {
      /* First, we don't allow the user to change the MI command name.
	 Supporting this would be tricky as we would need to delete the
	 mi_command_py from the MI command table, however, the user might
	 be trying to perform this reinitialization from within the very
	 command we're about to delete... it all gets very messy.

	 So, for now at least, we don't allow this.  This doesn't seem like
	 an excessive restriction.  */
      if (strcmp (cmd->mi_command_name, name) != 0)
	{
	  PyErr_SetString
	    (PyExc_ValueError,
	     _("can't reinitialize object with a different command name"));
	  return -1;
	}

      /* If there's already an object registered with the MI command table,
	 then we're done.  That object must be a mi_command_py, which
	 should reference back to this micmdpy_object.  */
      if (cmd->mi_command != nullptr)
	{
	  mi_command_py::validate_installation (cmd);
	  return 0;
	}
    }
  else
    cmd->mi_command_name = xstrdup (name);

  /* Now we can install this mi_command_py in the MI command table.  */
  return micmdpy_install_command (cmd);
}

/* Called when a gdb.MICommand object is deallocated.  */

static void
micmdpy_dealloc (PyObject *obj)
{
  PYMICMD_SCOPED_DEBUG_ENTER_EXIT;

  micmdpy_object *cmd = (micmdpy_object *) obj;

  /* If the Python object failed to initialize, then the name field might
     be nullptr.  */
  pymicmd_debug_printf ("obj = %p, name = %s", cmd,
			(cmd->mi_command_name == nullptr
			 ? "(null)" : cmd->mi_command_name));

  /* As the mi_command_py object holds a reference to the micmdpy_object,
     the only way the dealloc function can be called is if the mi_command_py
     object has been deleted, in which case the following assert will
     hold.  */
  gdb_assert (cmd->mi_command == nullptr);

  /* Free the memory that holds the command name.  */
  xfree (cmd->mi_command_name);
  cmd->mi_command_name = nullptr;

  /* Finally, free the memory for this Python object.  */
  Py_TYPE (obj)->tp_free (obj);
}

/* Python initialization for the MI commands components.  */

int
gdbpy_initialize_micommands ()
{
  micmdpy_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&micmdpy_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "MICommand",
			      (PyObject *) &micmdpy_object_type)
      < 0)
    return -1;

  invoke_cst = PyUnicode_FromString ("invoke");
  if (invoke_cst == nullptr)
    return -1;

  return 0;
}

void
gdbpy_finalize_micommands ()
{
  /* mi_command_py objects hold references to micmdpy_object objects.  They must
     be dropped before the Python interpreter is finalized.  Do so by removing
     those MI command entries, thus deleting the mi_command_py objects.  */
  remove_mi_cmd_entries ([] (mi_command *cmd)
    {
      return as_mi_command_py (cmd) != nullptr;
    });
}

/* Get the gdb.MICommand.name attribute, returns a string, the name of this
   MI command.  */

static PyObject *
micmdpy_get_name (PyObject *self, void *closure)
{
  struct micmdpy_object *micmd_obj = (struct micmdpy_object *) self;

  gdb_assert (micmd_obj->mi_command_name != nullptr);
  std::string name_str = string_printf ("-%s", micmd_obj->mi_command_name);
  return PyUnicode_FromString (name_str.c_str ());
}

/* Get the gdb.MICommand.installed property.  Returns true if this MI
   command is installed into the MI command table, otherwise returns
   false.  */

static PyObject *
micmdpy_get_installed (PyObject *self, void *closure)
{
  struct micmdpy_object *micmd_obj = (struct micmdpy_object *) self;

  if (micmd_obj->mi_command == nullptr)
    Py_RETURN_FALSE;
  Py_RETURN_TRUE;
}

/* Set the gdb.MICommand.installed property.  The property can be set to
   either true or false.  Setting the property to true will cause the
   command to be installed into the MI command table (if it isn't
   already), while setting this property to false will cause the command
   to be removed from the MI command table (if it is present).  */

static int
micmdpy_set_installed (PyObject *self, PyObject *newvalue, void *closure)
{
  struct micmdpy_object *micmd_obj = (struct micmdpy_object *) self;

  bool installed_p = PyObject_IsTrue (newvalue);
  if (installed_p == (micmd_obj->mi_command != nullptr))
    return 0;

  if (installed_p)
    return micmdpy_install_command (micmd_obj);
  else
    return micmdpy_uninstall_command (micmd_obj);
}

/* The gdb.MICommand properties.   */

static gdb_PyGetSetDef micmdpy_object_getset[] = {
  { "name", micmdpy_get_name, nullptr, "The command's name.", nullptr },
  { "installed", micmdpy_get_installed, micmdpy_set_installed,
    "Is this command installed for use.", nullptr },
  { nullptr }	/* Sentinel.  */
};

/* The gdb.MICommand descriptor.  */

PyTypeObject micmdpy_object_type = {
  PyVarObject_HEAD_INIT (nullptr, 0) "gdb.MICommand", /*tp_name */
  sizeof (micmdpy_object),			   /*tp_basicsize */
  0,						   /*tp_itemsize */
  micmdpy_dealloc,				   /*tp_dealloc */
  0,						   /*tp_print */
  0,						   /*tp_getattr */
  0,						   /*tp_setattr */
  0,						   /*tp_compare */
  0,						   /*tp_repr */
  0,						   /*tp_as_number */
  0,						   /*tp_as_sequence */
  0,						   /*tp_as_mapping */
  0,						   /*tp_hash */
  0,						   /*tp_call */
  0,						   /*tp_str */
  0,						   /*tp_getattro */
  0,						   /*tp_setattro */
  0,						   /*tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/*tp_flags */
  "GDB mi-command object",			   /* tp_doc */
  0,						   /* tp_traverse */
  0,						   /* tp_clear */
  0,						   /* tp_richcompare */
  0,						   /* tp_weaklistoffset */
  0,						   /* tp_iter */
  0,						   /* tp_iternext */
  0,						   /* tp_methods */
  0,						   /* tp_members */
  micmdpy_object_getset,			   /* tp_getset */
  0,						   /* tp_base */
  0,						   /* tp_dict */
  0,						   /* tp_descr_get */
  0,						   /* tp_descr_set */
  0,						   /* tp_dictoffset */
  micmdpy_init,					   /* tp_init */
  0,						   /* tp_alloc */
};

void _initialize_py_micmd ();
void
_initialize_py_micmd ()
{
  add_setshow_boolean_cmd
    ("py-micmd", class_maintenance, &pymicmd_debug,
     _("Set Python micmd debugging."),
     _("Show Python micmd debugging."),
     _("When on, Python micmd debugging is enabled."),
     nullptr,
     show_pymicmd_debug,
     &setdebuglist, &showdebuglist);
}
