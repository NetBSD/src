/* Python interface to inferiors.

   Copyright (C) 2009-2019 Free Software Foundation, Inc.

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
#include "observable.h"
#include "python-internal.h"
#include "arch-utils.h"
#include "language.h"
#include "common/gdb_signals.h"
#include "py-event.h"
#include "py-stopevent.h"

struct threadlist_entry {
  thread_object *thread_obj;
  struct threadlist_entry *next;
};

struct inferior_object
{
  PyObject_HEAD

  /* The inferior we represent.  */
  struct inferior *inferior;

  /* thread_object instances under this inferior.  This list owns a
     reference to each object it contains.  */
  struct threadlist_entry *threads;

  /* Number of threads in the list.  */
  int nthreads;
};

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
  enum gdb_signal stop_signal;

  if (!gdb_python_initialized)
    return;

  if (inferior_ptid == null_ptid)
    return;

  stop_signal = inferior_thread ()->suspend.stop_signal;

  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (emit_stop_event (bs, stop_signal) < 0)
    gdbpy_print_stack ();
}

static void
python_on_resume (ptid_t ptid)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (emit_continue_event (ptid) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call is about to be made. */

static void
python_on_inferior_call_pre (ptid_t thread, CORE_ADDR address)
{
  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (emit_inferior_call_event (INFERIOR_CALL_PRE, thread, address) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call has completed. */

static void
python_on_inferior_call_post (ptid_t thread, CORE_ADDR address)
{
  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (emit_inferior_call_event (INFERIOR_CALL_POST, thread, address) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when a part of memory has been modified by user action (eg via a
   'set' command). */

static void
python_on_memory_change (struct inferior *inferior, CORE_ADDR addr, ssize_t len, const bfd_byte *data)
{
  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (emit_memory_changed_event (addr, len) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when a register has been modified by user action (eg via a 'set'
   command). */

static void
python_on_register_change (struct frame_info *frame, int regnum)
{
  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (emit_register_changed_event (frame, regnum) < 0)
    gdbpy_print_stack ();
}

static void
python_inferior_exit (struct inferior *inf)
{
  const LONGEST *exit_code = NULL;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (target_gdbarch (), current_language);

  if (inf->has_exit_code)
    exit_code = &inf->exit_code;

  if (emit_exited_event (exit_code, inf) < 0)
    gdbpy_print_stack ();
}

/* Callback used to notify Python listeners about new objfiles loaded in the
   inferior.  OBJFILE may be NULL which means that the objfile list has been
   cleared (emptied).  */

static void
python_new_objfile (struct objfile *objfile)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (objfile != NULL
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
}

/* Return a reference to the Python object of type Inferior
   representing INFERIOR.  If the object has already been created,
   return it and increment the reference count,  otherwise, create it.
   Return NULL on failure.  */

gdbpy_ref<inferior_object>
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

      /* PyObject_New initializes the new object with a refcount of 1.  This
	 counts for the reference we are keeping in the inferior data.  */
      set_inferior_data (inferior, infpy_inf_data_key, inf_obj);
    }

  /* We are returning a new reference.  */
  gdb_assert (inf_obj != nullptr);
  return gdbpy_ref<inferior_object>::new_reference (inf_obj);
}

/* Called when a new inferior is created.  Notifies any Python event
   listeners.  */
static void
python_new_inferior (struct inferior *inf)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (python_gdbarch, python_language);

  if (evregpy_no_listeners_p (gdb_py_events.new_inferior))
    return;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (inf);
  if (inf_obj == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> event = create_event_object (&new_inferior_event_object_type);
  if (event == NULL
      || evpy_add_attribute (event.get (), "inferior",
			     (PyObject *) inf_obj.get ()) < 0
      || evpy_emit_event (event.get (), gdb_py_events.new_inferior) < 0)
    gdbpy_print_stack ();
}

/* Called when an inferior is removed.  Notifies any Python event
   listeners.  */
static void
python_inferior_deleted (struct inferior *inf)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (python_gdbarch, python_language);

  if (evregpy_no_listeners_p (gdb_py_events.inferior_deleted))
    return;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (inf);
  if (inf_obj == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> event = create_event_object (&inferior_deleted_event_object_type);
  if (event == NULL
      || evpy_add_attribute (event.get (), "inferior",
			     (PyObject *) inf_obj.get ()) < 0
      || evpy_emit_event (event.get (), gdb_py_events.inferior_deleted) < 0)
    gdbpy_print_stack ();
}

gdbpy_ref<>
thread_to_thread_object (thread_info *thr)
{
  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (thr->inf);
  if (inf_obj == NULL)
    return NULL;

  for (threadlist_entry *thread = inf_obj->threads;
       thread != NULL;
       thread = thread->next)
    if (thread->thread_obj->thread == thr)
      return gdbpy_ref<>::new_reference ((PyObject *) thread->thread_obj);

  PyErr_SetString (PyExc_SystemError,
		   _("could not find gdb thread object"));
  return NULL;
}

static void
add_thread_object (struct thread_info *tp)
{
  thread_object *thread_obj;
  inferior_object *inf_obj;
  struct threadlist_entry *entry;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (python_gdbarch, python_language);

  thread_obj = create_thread_object (tp);
  if (!thread_obj)
    {
      gdbpy_print_stack ();
      return;
    }

  inf_obj = (inferior_object *) thread_obj->inf_obj;

  entry = XNEW (struct threadlist_entry);
  entry->thread_obj = thread_obj;
  entry->next = inf_obj->threads;

  inf_obj->threads = entry;
  inf_obj->nthreads++;

  if (evregpy_no_listeners_p (gdb_py_events.new_thread))
    return;

  gdbpy_ref<> event = create_thread_event_object (&new_thread_event_object_type,
						  (PyObject *) thread_obj);
  if (event == NULL
      || evpy_emit_event (event.get (), gdb_py_events.new_thread) < 0)
    gdbpy_print_stack ();
}

static void
delete_thread_object (struct thread_info *tp, int ignore)
{
  struct threadlist_entry **entry, *tmp;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (python_gdbarch, python_language);

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (tp->inf);
  if (inf_obj == NULL)
    return;

  /* Find thread entry in its inferior's thread_list.  */
  for (entry = &inf_obj->threads; *entry != NULL; entry =
	 &(*entry)->next)
    if ((*entry)->thread_obj->thread == tp)
      break;

  if (!*entry)
    return;

  tmp = *entry;
  tmp->thread_obj->thread = NULL;

  *entry = (*entry)->next;
  inf_obj->nthreads--;

  Py_DECREF (tmp->thread_obj);
  xfree (tmp);
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

/* Getter of gdb.Inferior.progspace.  */

static PyObject *
infpy_get_progspace (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  program_space *pspace = inf->inferior->pspace;
  gdb_assert (pspace != nullptr);

  return pspace_to_pspace_object (pspace).release ();
}

static int
build_inferior_list (struct inferior *inf, void *arg)
{
  PyObject *list = (PyObject *) arg;
  gdbpy_ref<inferior_object> inferior = inferior_to_inferior_object (inf);

  if (inferior == NULL)
    return 0;

  return PyList_Append (list, (PyObject *) inferior.get ()) ? 1 : 0;
}

/* Implementation of gdb.inferiors () -> (gdb.Inferior, ...).
   Returns a tuple of all inferiors.  */
PyObject *
gdbpy_inferiors (PyObject *unused, PyObject *unused2)
{
  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  if (iterate_over_inferiors (build_inferior_list, list.get ()))
    return NULL;

  return PyList_AsTuple (list.get ());
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
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;
  PyObject *addr_obj, *length_obj, *result;
  static const char *keywords[] = { "address", "length", NULL };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OO", keywords,
					&addr_obj, &length_obj))
    return NULL;

  if (get_addr_from_python (addr_obj, &addr) < 0
      || get_addr_from_python (length_obj, &length) < 0)
    return NULL;

  TRY
    {
      buffer.reset ((gdb_byte *) xmalloc (length));

      read_memory (addr, buffer.get (), length);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  gdbpy_ref<membuf_object> membuf_obj (PyObject_New (membuf_object,
						     &membuf_object_type));
  if (membuf_obj == NULL)
    return NULL;

  membuf_obj->buffer = buffer.release ();
  membuf_obj->addr = addr;
  membuf_obj->length = length;

#ifdef IS_PY3K
  result = PyMemoryView_FromObject ((PyObject *) membuf_obj.get ());
#else
  result = PyBuffer_FromReadWriteObject ((PyObject *) membuf_obj.get (), 0,
					 Py_END_OF_BUFFER);
#endif

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
  static const char *keywords[] = { "address", "buffer", "length", NULL };
#ifdef IS_PY3K
  Py_buffer pybuf;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "Os*|O", keywords,
					&addr_obj, &pybuf, &length_obj))
    return NULL;

  buffer = (const gdb_byte *) pybuf.buf;
  buf_len = pybuf.len;
#else
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "Os#|O", keywords,
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

  /* Despite the documentation saying this field is a "const char *",
     in Python 3.4 at least, it's really a "char *".  */
  buf->format = (char *) "c";

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
  static const char *keywords[] = { "address", "length", "pattern", NULL };
  PyObject *start_addr_obj, *length_obj;
  Py_ssize_t pattern_size;
  const gdb_byte *buffer;
  CORE_ADDR found_addr;
  int found = 0;
#ifdef IS_PY3K
  Py_buffer pybuf;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OOs*", keywords,
					&start_addr_obj, &length_obj,
					&pybuf))
    return NULL;

  buffer = (const gdb_byte *) pybuf.buf;
  pattern_size = pybuf.len;
#else
  PyObject *pattern;
  const void *vbuffer;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OOO", keywords,
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

/* Implementation of gdb.Inferior.thread_from_thread_handle (self, handle)
                        ->  gdb.InferiorThread.  */

static PyObject *
infpy_thread_from_thread_handle (PyObject *self, PyObject *args, PyObject *kw)
{
  PyObject *handle_obj;
  inferior_object *inf_obj = (inferior_object *) self;
  static const char *keywords[] = { "thread_handle", NULL };

  INFPY_REQUIRE_VALID (inf_obj);

  if (! gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords, &handle_obj))
    return NULL;

  if (!gdbpy_is_value_object (handle_obj))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Argument 'handle_obj' must be a thread handle object."));

      return NULL;
    }

  TRY
    {
      struct thread_info *thread_info;
      struct value *val = value_object_to_value (handle_obj);

      thread_info = find_thread_by_handle (val, inf_obj->inferior);
      if (thread_info != NULL)
	return thread_to_thread_object (thread_info).release ();
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  Py_RETURN_NONE;
}

/* Implementation of gdb.Inferior.architecture.  */

static PyObject *
infpy_architecture (PyObject *self, PyObject *args)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return gdbarch_to_arch_object (inf->inferior->gdbarch);
}

/* Implement repr() for gdb.Inferior.  */

static PyObject *
infpy_repr (PyObject *obj)
{
  inferior_object *self = (inferior_object *) obj;
  inferior *inf = self->inferior;

  if (inf == nullptr)
    return PyString_FromString ("<gdb.Inferior (invalid)>");

  return PyString_FromFormat ("<gdb.Inferior num=%d, pid=%d>",
			      inf->num, inf->pid);
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
  gdbpy_ref<inferior_object> inf_obj ((inferior_object *) datum);
  struct threadlist_entry *th_entry, *th_tmp;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (python_gdbarch, python_language);

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
}

/* Implementation of gdb.selected_inferior() -> gdb.Inferior.
   Returns the current inferior object.  */

PyObject *
gdbpy_selected_inferior (PyObject *self, PyObject *args)
{
  return ((PyObject *)
	  inferior_to_inferior_object (current_inferior ()).release ());
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

  gdb::observers::new_thread.attach (add_thread_object);
  gdb::observers::thread_exit.attach (delete_thread_object);
  gdb::observers::normal_stop.attach (python_on_normal_stop);
  gdb::observers::target_resumed.attach (python_on_resume);
  gdb::observers::inferior_call_pre.attach (python_on_inferior_call_pre);
  gdb::observers::inferior_call_post.attach (python_on_inferior_call_post);
  gdb::observers::memory_changed.attach (python_on_memory_change);
  gdb::observers::register_changed.attach (python_on_register_change);
  gdb::observers::inferior_exit.attach (python_inferior_exit);
  gdb::observers::new_objfile.attach (python_new_objfile);
  gdb::observers::inferior_added.attach (python_new_inferior);
  gdb::observers::inferior_removed.attach (python_inferior_deleted);

  membuf_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&membuf_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Membuf",
				 (PyObject *) &membuf_object_type);
}

static gdb_PyGetSetDef inferior_object_getset[] =
{
  { "num", infpy_get_num, NULL, "ID of inferior, as assigned by GDB.", NULL },
  { "pid", infpy_get_pid, NULL, "PID of inferior, as assigned by the OS.",
    NULL },
  { "was_attached", infpy_get_was_attached, NULL,
    "True if the inferior was created using 'attach'.", NULL },
  { "progspace", infpy_get_progspace, NULL, "Program space of this inferior" },
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
  { "thread_from_thread_handle", (PyCFunction) infpy_thread_from_thread_handle,
    METH_VARARGS | METH_KEYWORDS,
    "thread_from_thread_handle (handle) -> gdb.InferiorThread.\n\
Return thread object corresponding to thread handle." },
  { "architecture", (PyCFunction) infpy_architecture, METH_NOARGS,
    "architecture () -> gdb.Architecture\n\
Return architecture of this inferior." },
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
  infpy_repr,			  /* tp_repr */
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
