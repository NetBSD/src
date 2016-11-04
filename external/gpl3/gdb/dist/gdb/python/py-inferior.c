/* Python interface to inferiors.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "gdbthread.h"
#include "inferior.h"
#include "objfiles.h"
#include "observer.h"
#include "python-internal.h"
#include "arch-utils.h"
#include "language.h"
#include "gdb_signals.h"
#include "py-event.h"
#include "py-stopevent.h"

struct threadlist_entry {
  thread_object *thread_obj;
  struct threadlist_entry *next;
};

typedef struct
{
  PyObject_HEAD

  /* The inferior we represent.  */
  struct inferior *inferior;

  /* thread_object instances under this inferior.  This list owns a
     reference to each object it contains.  */
  struct threadlist_entry *threads;

  /* Number of threads in the list.  */
  int nthreads;
} inferior_object;

extern PyTypeObject inferior_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("inferior_object");

static const struct inferior_data *infpy_inf_data_key;

typedef struct {
  PyObject_HEAD
  void *buffer;

  /* These are kept just for mbpy_str.  */
  CORE_ADDR addr;
  CORE_ADDR length;
} membuf_object;

extern PyTypeObject membuf_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("membuf_object");

/* Require that INFERIOR be a valid inferior ID.  */
#define INFPY_REQUIRE_VALID(Inferior)				\
  do {								\
    if (!Inferior->inferior)					\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Inferior no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)

static void
python_on_normal_stop (struct bpstats *bs, int print_frame)
{
  struct cleanup *cleanup;
  enum gdb_signal stop_signal;

  if (!gdb_python_initialized)
    return;

  if (!find_thread_ptid (inferior_ptid))
      return;

  stop_signal = inferior_thread ()->suspend.stop_signal;

  cleanup = ensure_python_env (get_current_arch (), current_language);

  if (emit_stop_event (bs, stop_signal) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

static void
python_on_resume (ptid_t ptid)
{
  struct cleanup *cleanup;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (emit_continue_event (ptid) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call is about to be made. */

static void
python_on_inferior_call_pre (ptid_t thread, CORE_ADDR address)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (emit_inferior_call_event (INFERIOR_CALL_PRE, thread, address) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call has completed. */

static void
python_on_inferior_call_post (ptid_t thread, CORE_ADDR address)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (emit_inferior_call_event (INFERIOR_CALL_POST, thread, address) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

/* Callback, registered as an observer, that notifies Python listeners
   when a part of memory has been modified by user action (eg via a
   'set' command). */

static void
python_on_memory_change (struct inferior *inferior, CORE_ADDR addr, ssize_t len, const bfd_byte *data)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (emit_memory_changed_event (addr, len) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

/* Callback, registered as an observer, that notifies Python listeners
   when a register has been modified by user action (eg via a 'set'
   command). */

static void
python_on_register_change (struct frame_info *frame, int regnum)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (emit_register_changed_event (frame, regnum) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

static void
python_inferior_exit (struct inferior *inf)
{
  struct cleanup *cleanup;
  const LONGEST *exit_code = NULL;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (target_gdbarch (), current_language);

  if (inf->has_exit_code)
    exit_code = &inf->exit_code;

  if (emit_exited_event (exit_code, inf) < 0)
    gdbpy_print_stack ();

  do_cleanups (cleanup);
}

/* Callback used to notify Python listeners about new objfiles loaded in the
   inferior.  OBJFILE may be NULL which means that the objfile list has been
   cleared (emptied).  */

static void
python_new_objfile (struct objfile *objfile)
{
  struct cleanup *cleanup;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (objfile != NULL
			       ? get_objfile_arch (objfile)
			       : target_gdbarch (),
			       current_language);

  if (objfile == NULL)
    {
      if (emit_clear_objfiles_event () < 0)
	gdbpy_print_stack ();
    }
  else
    {
      if (emit_new_objfile_event (objfile) < 0)
	gdbpy_print_stack ();
    }

  do_cleanups (cleanup);
}

/* Return a reference to the Python object of type Inferior
   representing INFERIOR.  If the object has already been created,
   return it and increment the reference count,  otherwise, create it.
   Return NULL on failure.  */
PyObject *
inferior_to_inferior_object (struct inferior *inferior)
{
  inferior_object *inf_obj;

  inf_obj = (inferior_object *) inferior_data (inferior, infpy_inf_data_key);
  if (!inf_obj)
    {
      inf_obj = PyObject_New (inferior_object, &inferior_object_type);
      if (!inf_obj)
	  return NULL;

      inf_obj->inferior = inferior;
      inf_obj->threads = NULL;
      inf_obj->nthreads = 0;

      set_inferior_data (inferior, infpy_inf_data_key, inf_obj);

    }
  else
    Py_INCREF ((PyObject *)inf_obj);

  return (PyObject *) inf_obj;
}

/* Finds the Python Inferior object for the given PID.  Returns a
   reference, or NULL if PID does not match any inferior object. */

PyObject *
find_inferior_object (int pid)
{
  struct inferior *inf = find_inferior_pid (pid);

  if (inf)
    return inferior_to_inferior_object (inf);

  return NULL;
}

thread_object *
find_thread_object (ptid_t ptid)
{
  int pid;
  struct threadlist_entry *thread;
  PyObject *inf_obj;
  thread_object *found = NULL;

  pid = ptid_get_pid (ptid);
  if (pid == 0)
    return NULL;

  inf_obj = find_inferior_object (pid);

  if (! inf_obj)
    return NULL;

  for (thread = ((inferior_object *)inf_obj)->threads; thread;
       thread = thread->next)
    if (ptid_equal (thread->thread_obj->thread->ptid, ptid))
      {
	found = thread->thread_obj;
	break;
      }

  Py_DECREF (inf_obj);

  if (found)
    return found;

  return NULL;
}

static void
add_thread_object (struct thread_info *tp)
{
  struct cleanup *cleanup;
  thread_object *thread_obj;
  inferior_object *inf_obj;
  struct threadlist_entry *entry;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (python_gdbarch, python_language);

  thread_obj = create_thread_object (tp);
  if (!thread_obj)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanup);
      return;
    }

  inf_obj = (inferior_object *) thread_obj->inf_obj;

  entry = XNEW (struct threadlist_entry);
  entry->thread_obj = thread_obj;
  entry->next = inf_obj->threads;

  inf_obj->threads = entry;
  inf_obj->nthreads++;

  do_cleanups (cleanup);
}

static void
delete_thread_object (struct thread_info *tp, int ignore)
{
  struct cleanup *cleanup;
  inferior_object *inf_obj;
  struct threadlist_entry **entry, *tmp;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (python_gdbarch, python_language);

  inf_obj
    = (inferior_object *) find_inferior_object (ptid_get_pid (tp->ptid));
  if (!inf_obj)
    {
      do_cleanups (cleanup);
      return;
    }

  /* Find thread entry in its inferior's thread_list.  */
  for (entry = &inf_obj->threads; *entry != NULL; entry =
	 &(*entry)->next)
    if ((*entry)->thread_obj->thread == tp)
      break;

  if (!*entry)
    {
      Py_DECREF (inf_obj);
      do_cleanups (cleanup);
      return;
    }

  tmp = *entry;
  tmp->thread_obj->thread = NULL;

  *entry = (*entry)->next;
  inf_obj->nthreads--;

  Py_DECREF (tmp->thread_obj);
  Py_DECREF (inf_obj);
  xfree (tmp);

  do_cleanups (cleanup);
}

static PyObject *
infpy_threads (PyObject *self, PyObject *args)
{
  int i;
  struct threadlist_entry *entry;
  inferior_object *inf_obj = (inferior_object *) self;
  PyObject *tuple;

  INFPY_REQUIRE_VALID (inf_obj);

  TRY
    {
      update_thread_list ();
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  tuple = PyTuple_New (inf_obj->nthreads);
  if (!tuple)
    return NULL;

  for (i = 0, entry = inf_obj->threads; i < inf_obj->nthreads;
       i++, entry = entry->next)
    {
      Py_INCREF (entry->thread_obj);
      PyTuple_SET_ITEM (tuple, i, (PyObject *) entry->thread_obj);
    }

  return tuple;
}

static PyObject *
infpy_get_num (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return PyLong_FromLong (inf->inferior->num);
}

static PyObject *
infpy_get_pid (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return PyLong_FromLong (inf->inferior->pid);
}

static PyObject *
infpy_get_was_attached (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);
  if (inf->inferior->attach_flag)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static int
build_inferior_list (struct inferior *inf, void *arg)
{
  PyObject *list = (PyObject *) arg;
  PyObject *inferior = inferior_to_inferior_object (inf);
  int success = 0;

  if (! inferior)
    return 0;

  success = PyList_Append (list, inferior);
  Py_DECREF (inferior);

  if (success)
    return 1;

  return 0;
}

/* Implementation of gdb.inferiors () -> (gdb.Inferior, ...).
   Returns a tuple of all inferiors.  */
PyObject *
gdbpy_inferiors (PyObject *unused, PyObject *unused2)
{
  PyObject *list, *tuple;

  list = PyList_New (0);
  if (!list)
    return NULL;

  if (iterate_over_inferiors (build_inferior_list, list))
    {
      Py_DECREF (list);
      return NULL;
    }

  tuple = PyList_AsTuple (list);
  Py_DECREF (list);

  return tuple;
}

/* Membuf and memory manipulation.  */

/* Implementation of Inferior.read_memory (address, length).
   Returns a Python buffer object with LENGTH bytes of the inferior's
   memory at ADDRESS.  Both arguments are integers.  Returns NULL on error,
   with a python exception set.  */
static PyObject *
infpy_read_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  CORE_ADDR addr, length;
  gdb_byte *buffer = NULL;
  membuf_object *membuf_obj;
  PyObject *addr_obj, *length_obj, *result;
  static char *keywords[] = { "address", "length", NULL };

  if (! PyArg_ParseTupleAndKeywords (args, kw, "OO", keywords,
				     &addr_obj, &length_obj))
    return NULL;

  if (get_addr_from_python (addr_obj, &addr) < 0
      || get_addr_from_python (length_obj, &length) < 0)
    return NULL;

  TRY
    {
      buffer = (gdb_byte *) xmalloc (length);

      read_memory (addr, buffer, length);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      xfree (buffer);
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  membuf_obj = PyObject_New (membuf_object, &membuf_object_type);
  if (membuf_obj == NULL)
    {
      xfree (buffer);
      return NULL;
    }

  membuf_obj->buffer = buffer;
  membuf_obj->addr = addr;
  membuf_obj->length = length;

#ifdef IS_PY3K
  result = PyMemoryView_FromObject ((PyObject *) membuf_obj);
#else
  result = PyBuffer_FromReadWriteObject ((PyObject *) membuf_obj, 0,
					 Py_END_OF_BUFFER);
#endif
  Py_DECREF (membuf_obj);

  return result;
}

/* Implementation of Inferior.write_memory (address, buffer [, length]).
   Writes the contents of BUFFER (a Python object supporting the read
   buffer protocol) at ADDRESS in the inferior's memory.  Write LENGTH
   bytes from BUFFER, or its entire contents if the argument is not
   provided.  The function returns nothing.  Returns NULL on error, with
   a python exception set.  */
static PyObject *
infpy_write_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  struct gdb_exception except = exception_none;
  Py_ssize_t buf_len;
  const gdb_byte *buffer;
  CORE_ADDR addr, length;
  PyObject *addr_obj, *length_obj = NULL;
  static char *keywords[] = { "address", "buffer", "length", NULL };
#ifdef IS_PY3K
  Py_buffer pybuf;

  if (! PyArg_ParseTupleAndKeywords (args, kw, "Os*|O", keywords,
				     &addr_obj, &pybuf,
				     &length_obj))
    return NULL;

  buffer = (const gdb_byte *) pybuf.buf;
  buf_len = pybuf.len;
#else
  if (! PyArg_ParseTupleAndKeywords (args, kw, "Os#|O", keywords,
				     &addr_obj, &buffer, &buf_len,
				     &length_obj))
    return NULL;

  buffer = (const gdb_byte *) buffer;
#endif

  if (get_addr_from_python (addr_obj, &addr) < 0)
    goto fail;

  if (!length_obj)
    length = buf_len;
  else if (get_addr_from_python (length_obj, &length) < 0)
    goto fail;

  TRY
    {
      write_memory_with_notification (addr, buffer, length);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      except = ex;
    }
  END_CATCH

#ifdef IS_PY3K
  PyBuffer_Release (&pybuf);
#endif
  GDB_PY_HANDLE_EXCEPTION (except);

  Py_RETURN_NONE;

 fail:
#ifdef IS_PY3K
  PyBuffer_Release (&pybuf);
#endif
  return NULL;
}

/* Destructor of Membuf objects.  */
static void
mbpy_dealloc (PyObject *self)
{
  xfree (((membuf_object *) self)->buffer);
  Py_TYPE (self)->tp_free (self);
}

/* Return a description of the Membuf object.  */
static PyObject *
mbpy_str (PyObject *self)
{
  membuf_object *membuf_obj = (membuf_object *) self;

  return PyString_FromFormat (_("Memory buffer for address %s, \
which is %s bytes long."),
			      paddress (python_gdbarch, membuf_obj->addr),
			      pulongest (membuf_obj->length));
}

#ifdef IS_PY3K

static int
get_buffer (PyObject *self, Py_buffer *buf, int flags)
{
  membuf_object *membuf_obj = (membuf_object *) self;
  int ret;

  ret = PyBuffer_FillInfo (buf, self, membuf_obj->buffer,
			   membuf_obj->length, 0,
			   PyBUF_CONTIG);
  buf->format = "c";

  return ret;
}

#else

static Py_ssize_t
get_read_buffer (PyObject *self, Py_ssize_t segment, void **ptrptr)
{
  membuf_object *membuf_obj = (membuf_object *) self;

  if (segment)
    {
      PyErr_SetString (PyExc_SystemError,
		       _("The memory buffer supports only one segment."));
      return -1;
    }

  *ptrptr = membuf_obj->buffer;

  return membuf_obj->length;
}

static Py_ssize_t
get_write_buffer (PyObject *self, Py_ssize_t segment, void **ptrptr)
{
  return get_read_buffer (self, segment, ptrptr);
}

static Py_ssize_t
get_seg_count (PyObject *self, Py_ssize_t *lenp)
{
  if (lenp)
    *lenp = ((membuf_object *) self)->length;

  return 1;
}

static Py_ssize_t
get_char_buffer (PyObject *self, Py_ssize_t segment, char **ptrptr)
{
  void *ptr = NULL;
  Py_ssize_t ret;

  ret = get_read_buffer (self, segment, &ptr);
  *ptrptr = (char *) ptr;

  return ret;
}

#endif	/* IS_PY3K */

/* Implementation of
   gdb.search_memory (address, length, pattern).  ADDRESS is the
   address to start the search.  LENGTH specifies the scope of the
   search from ADDRESS.  PATTERN is the pattern to search for (and
   must be a Python object supporting the buffer protocol).
   Returns a Python Long object holding the address where the pattern
   was located, or if the pattern was not found, returns None.  Returns NULL
   on error, with a python exception set.  */
static PyObject *
infpy_search_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  struct gdb_exception except = exception_none;
  CORE_ADDR start_addr, length;
  static char *keywords[] = { "address", "length", "pattern", NULL };
  PyObject *start_addr_obj, *length_obj;
  Py_ssize_t pattern_size;
  const gdb_byte *buffer;
  CORE_ADDR found_addr;
  int found = 0;
#ifdef IS_PY3K
  Py_buffer pybuf;

  if (! PyArg_ParseTupleAndKeywords (args, kw, "OOs*", keywords,
				     &start_addr_obj, &length_obj,
				     &pybuf))
    return NULL;

  buffer = (const gdb_byte *) pybuf.buf;
  pattern_size = pybuf.len;
#else
  PyObject *pattern;
  const void *vbuffer;

  if (! PyArg_ParseTupleAndKeywords (args, kw, "OOO", keywords,
 				     &start_addr_obj, &length_obj,
				     &pattern))
     return NULL;

  if (!PyObject_CheckReadBuffer (pattern))
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("The pattern is not a Python buffer."));

      return NULL;
    }

  if (PyObject_AsReadBuffer (pattern, &vbuffer, &pattern_size) == -1)
    return NULL;

  buffer = (const gdb_byte *) vbuffer;
#endif

  if (get_addr_from_python (start_addr_obj, &start_addr) < 0)
    goto fail;

  if (get_addr_from_python (length_obj, &length) < 0)
    goto fail;

  if (!length)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Search range is empty."));
      goto fail;
    }
  /* Watch for overflows.  */
  else if (length > CORE_ADDR_MAX
	   || (start_addr + length - 1) < start_addr)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("The search range is too large."));
      goto fail;
    }

  TRY
    {
      found = target_search_memory (start_addr, length,
				    buffer, pattern_size,
				    &found_addr);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      except = ex;
    }
  END_CATCH

#ifdef IS_PY3K
  PyBuffer_Release (&pybuf);
#endif
  GDB_PY_HANDLE_EXCEPTION (except);

  if (found)
    return PyLong_FromLong (found_addr);
  else
    Py_RETURN_NONE;

 fail:
#ifdef IS_PY3K
  PyBuffer_Release (&pybuf);
#endif
  return NULL;
}

/* Implementation of gdb.Inferior.is_valid (self) -> Boolean.
   Returns True if this inferior object still exists in GDB.  */

static PyObject *
infpy_is_valid (PyObject *self, PyObject *args)
{
  inferior_object *inf = (inferior_object *) self;

  if (! inf->inferior)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

static void
infpy_dealloc (PyObject *obj)
{
  inferior_object *inf_obj = (inferior_object *) obj;
  struct inferior *inf = inf_obj->inferior;

  if (! inf)
    return;

  set_inferior_data (inf, infpy_inf_data_key, NULL);
}

/* Clear the INFERIOR pointer in an Inferior object and clear the
   thread list.  */
static void
py_free_inferior (struct inferior *inf, void *datum)
{

  struct cleanup *cleanup;
  inferior_object *inf_obj = (inferior_object *) datum;
  struct threadlist_entry *th_entry, *th_tmp;

  if (!gdb_python_initialized)
    return;

  cleanup = ensure_python_env (python_gdbarch, python_language);

  inf_obj->inferior = NULL;

  /* Deallocate threads list.  */
  for (th_entry = inf_obj->threads; th_entry != NULL;)
    {
      Py_DECREF (th_entry->thread_obj);

      th_tmp = th_entry;
      th_entry = th_entry->next;
      xfree (th_tmp);
    }

  inf_obj->nthreads = 0;

  Py_DECREF ((PyObject *) inf_obj);
  do_cleanups (cleanup);
}

/* Implementation of gdb.selected_inferior() -> gdb.Inferior.
   Returns the current inferior object.  */

PyObject *
gdbpy_selected_inferior (PyObject *self, PyObject *args)
{
  return inferior_to_inferior_object (current_inferior ());
}

int
gdbpy_initialize_inferior (void)
{
  if (PyType_Ready (&inferior_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "Inferior",
			      (PyObject *) &inferior_object_type) < 0)
    return -1;

  infpy_inf_data_key =
    register_inferior_data_with_cleanup (NULL, py_free_inferior);

  observer_attach_new_thread (add_thread_object);
  observer_attach_thread_exit (delete_thread_object);
  observer_attach_normal_stop (python_on_normal_stop);
  observer_attach_target_resumed (python_on_resume);
  observer_attach_inferior_call_pre (python_on_inferior_call_pre);
  observer_attach_inferior_call_post (python_on_inferior_call_post);
  observer_attach_memory_changed (python_on_memory_change);
  observer_attach_register_changed (python_on_register_change);
  observer_attach_inferior_exit (python_inferior_exit);
  observer_attach_new_objfile (python_new_objfile);

  membuf_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&membuf_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Membuf", (PyObject *)
				 &membuf_object_type);
}

static PyGetSetDef inferior_object_getset[] =
{
  { "num", infpy_get_num, NULL, "ID of inferior, as assigned by GDB.", NULL },
  { "pid", infpy_get_pid, NULL, "PID of inferior, as assigned by the OS.",
    NULL },
  { "was_attached", infpy_get_was_attached, NULL,
    "True if the inferior was created using 'attach'.", NULL },
  { NULL }
};

static PyMethodDef inferior_object_methods[] =
{
  { "is_valid", infpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this inferior is valid, false if not." },
  { "threads", infpy_threads, METH_NOARGS,
    "Return all the threads of this inferior." },
  { "read_memory", (PyCFunction) infpy_read_memory,
    METH_VARARGS | METH_KEYWORDS,
    "read_memory (address, length) -> buffer\n\
Return a buffer object for reading from the inferior's memory." },
  { "write_memory", (PyCFunction) infpy_write_memory,
    METH_VARARGS | METH_KEYWORDS,
    "write_memory (address, buffer [, length])\n\
Write the given buffer object to the inferior's memory." },
  { "search_memory", (PyCFunction) infpy_search_memory,
    METH_VARARGS | METH_KEYWORDS,
    "search_memory (address, length, pattern) -> long\n\
Return a long with the address of a match, or None." },
  { NULL }
};

PyTypeObject inferior_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Inferior",		  /* tp_name */
  sizeof (inferior_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  infpy_dealloc,		  /* tp_dealloc */
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
  0,				  /* tp_str */
  0,				  /* tp_getattro */
  0,				  /* tp_setattro */
  0,				  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,  /* tp_flags */
  "GDB inferior object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  inferior_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  inferior_object_getset,	  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};

#ifdef IS_PY3K

static PyBufferProcs buffer_procs =
{
  get_buffer
};

#else

/* Python doesn't provide a decent way to get compatibility here.  */
#if HAVE_LIBPYTHON2_4
#define CHARBUFFERPROC_NAME getcharbufferproc
#else
#define CHARBUFFERPROC_NAME charbufferproc
#endif

static PyBufferProcs buffer_procs = {
  get_read_buffer,
  get_write_buffer,
  get_seg_count,
  /* The cast here works around a difference between Python 2.4 and
     Python 2.5.  */
  (CHARBUFFERPROC_NAME) get_char_buffer
};
#endif	/* IS_PY3K */

PyTypeObject membuf_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Membuf",			  /*tp_name*/
  sizeof (membuf_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  mbpy_dealloc,			  /*tp_dealloc*/
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
  mbpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  &buffer_procs,		  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB memory buffer object", 	  /*tp_doc*/
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
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
