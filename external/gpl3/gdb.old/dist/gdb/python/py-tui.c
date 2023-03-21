/* TUI windows implemented in Python

   Copyright (C) 2020 Free Software Foundation, Inc.

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
#include "python-internal.h"

#ifdef TUI

/* Note that Python's public headers may define HAVE_NCURSES_H, so if
   we unconditionally include this (outside the #ifdef above), then we
   can get a compile error when ncurses is not in fact installed.  See
   PR tui/25597; or the upstream Python bug
   https://bugs.python.org/issue20768.  */
#include "gdb_curses.h"

#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-layout.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"

class tui_py_window;

/* A PyObject representing a TUI window.  */

struct gdbpy_tui_window
{
  PyObject_HEAD

  /* The TUI window, or nullptr if the window has been deleted.  */
  tui_py_window *window;
};

extern PyTypeObject gdbpy_tui_window_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("gdbpy_tui_window");

/* A TUI window written in Python.  */

class tui_py_window : public tui_win_info
{
public:

  tui_py_window (const char *name, gdbpy_ref<gdbpy_tui_window> wrapper)
    : m_name (name),
      m_wrapper (std::move (wrapper))
  {
    m_wrapper->window = this;
  }

  ~tui_py_window ();

  DISABLE_COPY_AND_ASSIGN (tui_py_window);

  /* Set the "user window" to the indicated reference.  The user
     window is the object returned the by user-defined window
     constructor.  */
  void set_user_window (gdbpy_ref<> &&user_window)
  {
    m_window = std::move (user_window);
  }

  const char *name () const override
  {
    return m_name.c_str ();
  }

  void rerender () override;
  void do_scroll_vertical (int num_to_scroll) override;
  void do_scroll_horizontal (int num_to_scroll) override;

  /* Erase and re-box the window.  */
  void erase ()
  {
    if (is_visible ())
      {
	werase (handle.get ());
	check_and_display_highlight_if_needed ();
	cursor_x = 0;
	cursor_y = 0;
      }
  }

  /* Write STR to the window.  */
  void output (const char *str);

  /* A helper function to compute the viewport width.  */
  int viewport_width () const
  {
    return std::max (0, width - 2);
  }

  /* A helper function to compute the viewport height.  */
  int viewport_height () const
  {
    return std::max (0, height - 2);
  }

private:

  /* Location of the cursor.  */
  int cursor_x = 0;
  int cursor_y = 0;

  /* The name of this window.  */
  std::string m_name;

  /* The underlying Python window object.  */
  gdbpy_ref<> m_window;

  /* The Python wrapper for this object.  */
  gdbpy_ref<gdbpy_tui_window> m_wrapper;
};

tui_py_window::~tui_py_window ()
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* This can be null if the user-provided Python construction
     function failed.  */
  if (m_window != nullptr
      && PyObject_HasAttrString (m_window.get (), "close"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "close",
					       nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }

  /* Unlink.  */
  m_wrapper->window = nullptr;
  /* Explicitly free the Python references.  We have to do this
     manually because we need to hold the GIL while doing so.  */
  m_wrapper.reset (nullptr);
  m_window.reset (nullptr);
}

void
tui_py_window::rerender ()
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (PyObject_HasAttrString (m_window.get (), "render"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "render",
					       nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::do_scroll_horizontal (int num_to_scroll)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (PyObject_HasAttrString (m_window.get (), "hscroll"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get(), "hscroll",
					       "i", num_to_scroll, nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::do_scroll_vertical (int num_to_scroll)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (PyObject_HasAttrString (m_window.get (), "vscroll"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "vscroll",
					       "i", num_to_scroll, nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::output (const char *text)
{
  int vwidth = viewport_width ();

  while (cursor_y < viewport_height () && *text != '\0')
    {
      wmove (handle.get (), cursor_y + 1, cursor_x + 1);

      std::string line = tui_copy_source_line (&text, 0, 0,
					       vwidth - cursor_x, 0);
      tui_puts (line.c_str (), handle.get ());

      if (*text == '\n')
	{
	  ++text;
	  ++cursor_y;
	  cursor_x = 0;
	}
      else
	cursor_x = getcurx (handle.get ()) - 1;
    }

  wrefresh (handle.get ());
}



/* A callable that is used to create a TUI window.  It wraps the
   user-supplied window constructor.  */

class gdbpy_tui_window_maker
{
public:

  explicit gdbpy_tui_window_maker (gdbpy_ref<> &&constr)
    : m_constr (std::move (constr))
  {
  }

  ~gdbpy_tui_window_maker ();

  gdbpy_tui_window_maker (gdbpy_tui_window_maker &&other) noexcept
    : m_constr (std::move (other.m_constr))
  {
  }

  gdbpy_tui_window_maker (const gdbpy_tui_window_maker &other)
  {
    gdbpy_enter enter_py (get_current_arch (), current_language);
    m_constr = other.m_constr;
  }

  gdbpy_tui_window_maker &operator= (gdbpy_tui_window_maker &&other)
  {
    m_constr = std::move (other.m_constr);
    return *this;
  }

  gdbpy_tui_window_maker &operator= (const gdbpy_tui_window_maker &other)
  {
    gdbpy_enter enter_py (get_current_arch (), current_language);
    m_constr = other.m_constr;
    return *this;
  }

  tui_win_info *operator() (const char *name);

private:

  /* A constructor that is called to make a TUI window.  */
  gdbpy_ref<> m_constr;
};

gdbpy_tui_window_maker::~gdbpy_tui_window_maker ()
{
  gdbpy_enter enter_py (get_current_arch (), current_language);
  m_constr.reset (nullptr);
}

tui_win_info *
gdbpy_tui_window_maker::operator() (const char *win_name)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  gdbpy_ref<gdbpy_tui_window> wrapper
    (PyObject_New (gdbpy_tui_window, &gdbpy_tui_window_object_type));
  if (wrapper == nullptr)
    {
      gdbpy_print_stack ();
      return nullptr;
    }

  std::unique_ptr<tui_py_window> window
    (new tui_py_window (win_name, wrapper));

  gdbpy_ref<> user_window
    (PyObject_CallFunctionObjArgs (m_constr.get (),
				   (PyObject *) wrapper.get (),
				   nullptr));
  if (user_window == nullptr)
    {
      gdbpy_print_stack ();
      return nullptr;
    }

  window->set_user_window (std::move (user_window));
  /* Window is now owned by the TUI.  */
  return window.release ();
}

/* Implement "gdb.register_window_type".  */

PyObject *
gdbpy_register_tui_window (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "constructor", nullptr };

  const char *name;
  PyObject *cons_obj;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "sO", keywords,
					&name, &cons_obj))
    return nullptr;

  try
    {
      gdbpy_tui_window_maker constr (gdbpy_ref<>::new_reference (cons_obj));
      tui_register_window (name, constr);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return nullptr;
    }

  Py_RETURN_NONE;
}



/* Require that "Window" be a valid window.  */

#define REQUIRE_WINDOW(Window)					\
    do {							\
      if ((Window)->window == nullptr)				\
        return PyErr_Format (PyExc_RuntimeError,		\
                             _("TUI window is invalid."));	\
    } while (0)

/* Python function which checks the validity of a TUI window
   object.  */
static PyObject *
gdbpy_tui_is_valid (PyObject *self, PyObject *args)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  if (win->window != nullptr)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Python function that erases the TUI window.  */
static PyObject *
gdbpy_tui_erase (PyObject *self, PyObject *args)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  REQUIRE_WINDOW (win);

  win->window->erase ();

  Py_RETURN_NONE;
}

/* Python function that writes some text to a TUI window.  */
static PyObject *
gdbpy_tui_write (PyObject *self, PyObject *args)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  const char *text;

  if (!PyArg_ParseTuple (args, "s", &text))
    return nullptr;

  REQUIRE_WINDOW (win);

  win->window->output (text);

  Py_RETURN_NONE;
}

/* Return the width of the TUI window.  */
static PyObject *
gdbpy_tui_width (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  return PyLong_FromLong (win->window->viewport_width ());
}

/* Return the height of the TUI window.  */
static PyObject *
gdbpy_tui_height (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  return PyLong_FromLong (win->window->viewport_height ());
}

/* Return the title of the TUI window.  */
static PyObject *
gdbpy_tui_title (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  return host_string_to_python_string (win->window->title.c_str ()).release ();
}

/* Set the title of the TUI window.  */
static int
gdbpy_tui_set_title (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  if (win->window == nullptr)
    {
      PyErr_Format (PyExc_RuntimeError, _("TUI window is invalid."));
      return -1;
    }

  if (win->window == nullptr)
    {
      PyErr_Format (PyExc_TypeError, _("Cannot delete \"title\" attribute."));
      return -1;
    }

  gdb::unique_xmalloc_ptr<char> value
    = python_string_to_host_string (newvalue);
  if (value == nullptr)
    return -1;

  win->window->title = value.get ();
  return 0;
}

static gdb_PyGetSetDef tui_object_getset[] =
{
  { "width", gdbpy_tui_width, NULL, "Width of the window.", NULL },
  { "height", gdbpy_tui_height, NULL, "Height of the window.", NULL },
  { "title", gdbpy_tui_title, gdbpy_tui_set_title, "Title of the window.",
    NULL },
  { NULL }  /* Sentinel */
};

static PyMethodDef tui_object_methods[] =
{
  { "is_valid", gdbpy_tui_is_valid, METH_NOARGS,
    "is_valid () -> Boolean\n\
Return true if this TUI window is valid, false if not." },
  { "erase", gdbpy_tui_erase, METH_NOARGS,
    "Erase the TUI window." },
  { "write", (PyCFunction) gdbpy_tui_write, METH_VARARGS,
    "Append a string to the TUI window." },
  { NULL } /* Sentinel.  */
};

PyTypeObject gdbpy_tui_window_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.TuiWindow",		  /*tp_name*/
  sizeof (gdbpy_tui_window),	  /*tp_basicsize*/
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
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro */
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
  "GDB TUI window object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  tui_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  tui_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
};

#endif /* TUI */

/* Initialize this module.  */

int
gdbpy_initialize_tui ()
{
#ifdef TUI
  gdbpy_tui_window_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&gdbpy_tui_window_object_type) < 0)
    return -1;
#endif	/* TUI */

  return 0;
}
