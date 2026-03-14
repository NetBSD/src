/* Python interface to ui_file_style::color objects.

   Copyright (C) 2008-2025 Free Software Foundation, Inc.

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


#include "python-internal.h"
#include "py-color.h"
#include "cli/cli-decode.h"
#include "cli/cli-style.h"

/* Colorspace constants and their values.  */
static struct {
  const char *name;
  color_space value;
} colorspace_constants[] =
{
  { "COLORSPACE_MONOCHROME", color_space::MONOCHROME },
  { "COLORSPACE_ANSI_8COLOR", color_space::ANSI_8COLOR },
  { "COLORSPACE_AIXTERM_16COLOR", color_space::AIXTERM_16COLOR },
  { "COLORSPACE_XTERM_256COLOR", color_space::XTERM_256COLOR },
  { "COLORSPACE_RGB_24BIT", color_space::RGB_24BIT },
};

/* A color.  */
struct colorpy_object
{
  PyObject_HEAD

  /* Underlying value.  */
  ui_file_style::color color;
};

extern PyTypeObject colorpy_object_type;

/* See py-color.h.  */
gdbpy_ref<>
create_color_object (const ui_file_style::color &color)
{
  gdbpy_ref<colorpy_object> color_obj (PyObject_New (colorpy_object,
						     &colorpy_object_type));

  if (color_obj == nullptr)
    return nullptr;

  color_obj->color = color;
  return gdbpy_ref<> ((PyObject *) color_obj.release ());
}

/* See py-color.h.  */
bool
gdbpy_is_color (PyObject *obj)
{
  gdb_assert (obj != nullptr);
  return PyObject_TypeCheck (obj, &colorpy_object_type) != 0;
}

/* See py-color.h.  */
const ui_file_style::color &
gdbpy_get_color (PyObject *obj)
{
  gdb_assert (gdbpy_is_color (obj));
  colorpy_object *self = (colorpy_object *) obj;
  return self->color;
}

/* Get an attribute.  */
static PyObject *
get_attr (PyObject *obj, PyObject *attr_name)
{
  if (!PyUnicode_Check (attr_name))
    return PyObject_GenericGetAttr (obj, attr_name);

  colorpy_object *self = (colorpy_object *) obj;
  const ui_file_style::color &color = self->color;

  if (!PyUnicode_CompareWithASCIIString (attr_name, "colorspace"))
    {
      int value = static_cast<int> (color.colorspace ());
      return gdb_py_object_from_longest (value).release ();
    }

  if (!PyUnicode_CompareWithASCIIString (attr_name, "is_none"))
    return PyBool_FromLong (color.is_none ());

  if (!PyUnicode_CompareWithASCIIString (attr_name, "is_indexed"))
    return PyBool_FromLong (color.is_indexed ());

  if (!PyUnicode_CompareWithASCIIString (attr_name, "is_direct"))
    return PyBool_FromLong (color.is_direct ());

  if (color.is_indexed ()
      && !PyUnicode_CompareWithASCIIString (attr_name, "index"))
    return gdb_py_object_from_longest (color.get_value ()).release ();

  if (color.is_direct ()
      && !PyUnicode_CompareWithASCIIString (attr_name, "components"))
    {
      uint8_t rgb[3];
      color.get_rgb (rgb);

      gdbpy_ref<> rgb_objects[3];
      for (int i = 0; i < 3; ++i)
	{
	  rgb_objects[i] = gdb_py_object_from_ulongest (rgb[i]);
	  if (rgb_objects[i] == nullptr)
	    return nullptr;
	}

      PyObject *comp = PyTuple_New (3);
      if (comp == nullptr)
	return nullptr;

      for (int i = 0; i < 3; ++i)
	PyTuple_SET_ITEM (comp, i, rgb_objects[i].release ());

      return comp;
    }

  return PyObject_GenericGetAttr (obj, attr_name);
}

/* Implementation of Color.escape_sequence (self, is_fg) -> str.  */

static PyObject *
colorpy_escape_sequence (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "is_foreground", nullptr };
  PyObject *is_fg_obj;

  /* Parse method arguments.  */
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "O!", keywords,
					&PyBool_Type, &is_fg_obj))
    return nullptr;

  /* Python ensures the type of SELF.  */
  gdb_assert (gdbpy_is_color (self));

  /* The argument parsing ensures we have a bool.  */
  gdb_assert (PyBool_Check (is_fg_obj));

  std::string s;
  if (term_cli_styling ())
    {
      bool is_fg = is_fg_obj == Py_True;
      s = gdbpy_get_color (self).to_ansi (is_fg);
    }

  return host_string_to_python_string (s.c_str ()).release ();
}

/* Object initializer; fills color with value.

   Use: __init__(VALUE = None, COLORSPACE = None)

   VALUE is a string, integer, RGB-tuple or None.

   COLORSPACE is the color space index.

   Returns -1 on error, with a python exception set.  */

static int
colorpy_init (PyObject *self, PyObject *args, PyObject *kwds)
{
  colorpy_object *obj = (colorpy_object *) self;
  PyObject *value_obj = nullptr;
  PyObject *colorspace_obj = nullptr;
  color_space colorspace = color_space::MONOCHROME;

  static const char *keywords[] = { "value", "color_space", nullptr };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwds, "|OO", keywords,
					&value_obj, &colorspace_obj))
    return -1;

  try
    {
      if (colorspace_obj != nullptr)
	{
	  if (PyLong_Check (colorspace_obj))
	    {
	      long colorspace_id = -1;
	      if (!gdb_py_int_as_long (colorspace_obj, &colorspace_id))
		return -1;
	      if (!color_space_safe_cast (&colorspace, colorspace_id))
		error (_("colorspace %ld is out of range."), colorspace_id);
	    }
	  else if (colorspace_obj == Py_None)
	    colorspace_obj = nullptr;
	  else
	    error (_("colorspace must be None or integer"));
	}

      if (value_obj == nullptr || value_obj == Py_None)
	  obj->color = ui_file_style::color (colorspace, -1);
      else if (PyLong_Check (value_obj))
	{
	  long value = -1;
	  if (!gdb_py_int_as_long (value_obj, &value))
	    return -1;
	  if (value < 0 || value > INT_MAX)
	    error (_("value %ld is out of range."), value);
	  if (colorspace_obj != nullptr)
	    obj->color = ui_file_style::color (colorspace, value);
	  else
	    obj->color = ui_file_style::color (value);
	}
      else if (PyTuple_Check (value_obj))
	{
	  if (colorspace_obj == nullptr || colorspace != color_space::RGB_24BIT)
	    error (_("colorspace must be gdb.COLORSPACE_RGB_24BIT with "
		   "value of tuple type."));
	  Py_ssize_t tuple_size = PyTuple_Size (value_obj);
	  if (tuple_size < 0)
	    return -1;
	  if (tuple_size != 3)
	    error (_("Tuple value with RGB must be of size 3."));
	  uint8_t rgb[3];
	  for (int i = 0; i < 3; ++i)
	    {
	      PyObject *item = PyTuple_GetItem (value_obj, i);
	      if (!PyLong_Check (item))
		error (_("Item %d of an RGB tuple must be integer."), i);
	      long item_value = -1;
	      if (!gdb_py_int_as_long (item, &item_value))
		return -1;
	      if (item_value < 0 || item_value > UINT8_MAX)
		error (_("RGB item %ld is out of byte range."), item_value);
	      rgb[i] = static_cast<uint8_t> (item_value);
	    }

	  obj->color = ui_file_style::color (rgb[0], rgb[1], rgb[2]);
	}
      else if (PyUnicode_Check (value_obj))
	{
	  gdb::unique_xmalloc_ptr<char>
	    str (python_string_to_host_string (value_obj));
	  if (str == nullptr)
	    return -1;
	  obj->color = parse_var_color (str.get());

	  if (colorspace_obj != nullptr
	      && colorspace != obj->color.colorspace ())
	    error (_("colorspace doesn't match to the value."));
	}
      else
	error (_("value must be one of None, integer, tuple or str."));
    }
  catch (const gdb_exception &except)
    {
      return gdbpy_handle_gdb_exception (-1, except);
    }

  return 0;
}

static PyObject *
colorpy_str (PyObject *self)
{
  colorpy_object *obj = reinterpret_cast<colorpy_object *> (self);

  return PyUnicode_FromString (obj->color.to_string ().c_str ());
}

/* Initialize the 'color' module.  */
static int
gdbpy_initialize_color (void)
{
  for (auto &pair : colorspace_constants)
    if (PyModule_AddIntConstant (gdb_module, pair.name,
				 static_cast<long> (pair.value)) < 0)
      return -1;

  colorpy_object_type.tp_new = PyType_GenericNew;
  return gdbpy_type_ready (&colorpy_object_type, gdb_module);
}

/* Color methods.  */

static PyMethodDef color_methods[] =
{
  { "escape_sequence", (PyCFunction) colorpy_escape_sequence,
    METH_VARARGS | METH_KEYWORDS,
    "escape_sequence (is_foreground) -> str.\n\
Return the ANSI escape sequence for this color.\n\
IS_FOREGROUND indicates whether this is a foreground or background color."},
  {nullptr}
};

PyTypeObject colorpy_object_type =
{
  PyVarObject_HEAD_INIT (nullptr, 0)
  "gdb.Color",			  /*tp_name*/
  sizeof (colorpy_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  colorpy_str,			  /*tp_str*/
  get_attr,			  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB color object",	  	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  color_methods,		  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  colorpy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};

GDBPY_INITIALIZE_FILE (gdbpy_initialize_color);
