/* Gdb/Python header for private use by Python module.

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

#ifndef GDB_PYTHON_INTERNAL_H
#define GDB_PYTHON_INTERNAL_H

#include "extension.h"
#include "extension-priv.h"

/* These WITH_* macros are defined by the CPython API checker that
   comes with the Python plugin for GCC.  See:
   https://gcc-python-plugin.readthedocs.org/en/latest/cpychecker.html
   The checker defines a WITH_ macro for each attribute it
   exposes.  */

#ifdef WITH_CPYCHECKER_RETURNS_BORROWED_REF_ATTRIBUTE
#define CPYCHECKER_RETURNS_BORROWED_REF			\
  __attribute__ ((cpychecker_returns_borrowed_ref))
#else
#define CPYCHECKER_RETURNS_BORROWED_REF
#endif

#ifdef WITH_CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF_ATTRIBUTE
#define CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF(ARG)		\
  __attribute__ ((cpychecker_type_object_for_typedef (ARG)))
#else
#define CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF(ARG)
#endif

#ifdef WITH_CPYCHECKER_STEALS_REFERENCE_TO_ARG_ATTRIBUTE
#define CPYCHECKER_STEALS_REFERENCE_TO_ARG(n) \
   __attribute__ ((cpychecker_steals_reference_to_arg (n)))
#else
#define CPYCHECKER_STEALS_REFERENCE_TO_ARG(n)
#endif

#ifdef WITH_CPYCHECKER_SETS_EXCEPTION_ATTRIBUTE
#define CPYCHECKER_SETS_EXCEPTION __attribute__ ((cpychecker_sets_exception))
#else
#define CPYCHECKER_SETS_EXCEPTION
#endif

#ifdef WITH_CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION_ATTRIBUTE
#define CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION		\
  __attribute__ ((cpychecker_negative_result_sets_exception))
#else
#define CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
#endif

/* Python 2.4 doesn't include stdint.h soon enough to get {u,}intptr_t
   needed by pyport.h.  */
/* /usr/include/features.h on linux systems will define _POSIX_C_SOURCE
   if it sees _GNU_SOURCE (which config.h will define).
   pyconfig.h defines _POSIX_C_SOURCE to a different value than
   /usr/include/features.h does causing compilation to fail.
   To work around this, undef _POSIX_C_SOURCE before we include Python.h.

   Same problem with _XOPEN_SOURCE.  */
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

/* On sparc-solaris, /usr/include/sys/feature_tests.h defines
   _FILE_OFFSET_BITS, which pyconfig.h also defines.  Same work
   around technique as above.  */
#undef _FILE_OFFSET_BITS

/* A kludge to avoid redefinition of snprintf on Windows by pyerrors.h.  */
#if defined(_WIN32) && defined(HAVE_DECL_SNPRINTF)
#define HAVE_SNPRINTF 1
#endif

/* Request clean size types from Python.  */
#define PY_SSIZE_T_CLEAN

/* Include the Python header files using angle brackets rather than
   double quotes.  On case-insensitive filesystems, this prevents us
   from including our python/python.h header file.  */
#include <Python.h>
#include <frameobject.h>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K 1
#endif

#ifdef IS_PY3K
#define Py_TPFLAGS_HAVE_ITER 0
#define Py_TPFLAGS_CHECKTYPES 0

#define PyInt_Check PyLong_Check
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromSsize_t PyLong_FromSsize_t
#define PyInt_AsLong PyLong_AsLong
#define PyInt_AsSsize_t PyLong_AsSsize_t

#define PyString_FromString PyUnicode_FromString
#define PyString_Decode PyUnicode_Decode
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_Check PyUnicode_Check
#endif

#if HAVE_LIBPYTHON2_4
/* Py_ssize_t is not defined until 2.5.
   Logical type for Py_ssize_t is Py_intptr_t, but that fails in 64-bit
   compilation due to several apparent mistakes in python2.4 API, so we
   use 'int' instead.  */
typedef int Py_ssize_t;
#endif

#ifndef PyVarObject_HEAD_INIT
/* Python 2.4 does not define PyVarObject_HEAD_INIT.  */
#define PyVarObject_HEAD_INIT(type, size)       \
    PyObject_HEAD_INIT(type) size,

#endif

#ifndef Py_TYPE
/* Python 2.4 does not define Py_TYPE.  */
#define Py_TYPE(ob)             (((PyObject*)(ob))->ob_type)
#endif

/* If Python.h does not define WITH_THREAD, then the various
   GIL-related functions will not be defined.  However,
   PyGILState_STATE will be.  */
#ifndef WITH_THREAD
#define PyGILState_Ensure() ((PyGILState_STATE) 0)
#define PyGILState_Release(ARG) ((void)(ARG))
#define PyEval_InitThreads()
#define PyThreadState_Swap(ARG) ((void)(ARG))
#define PyEval_ReleaseLock()
#endif

/* Python supplies HAVE_LONG_LONG and some `long long' support when it
   is available.  These defines let us handle the differences more
   cleanly.  */
#ifdef HAVE_LONG_LONG

#define GDB_PY_LL_ARG "L"
#define GDB_PY_LLU_ARG "K"
typedef PY_LONG_LONG gdb_py_longest;
typedef unsigned PY_LONG_LONG gdb_py_ulongest;
#define gdb_py_long_from_longest PyLong_FromLongLong
#define gdb_py_long_from_ulongest PyLong_FromUnsignedLongLong
#define gdb_py_long_as_ulongest PyLong_AsUnsignedLongLong

#else /* HAVE_LONG_LONG */

#define GDB_PY_LL_ARG "L"
#define GDB_PY_LLU_ARG "K"
typedef long gdb_py_longest;
typedef unsigned long gdb_py_ulongest;
#define gdb_py_long_from_longest PyLong_FromLong
#define gdb_py_long_from_ulongest PyLong_FromUnsignedLong
#define gdb_py_long_as_ulongest PyLong_AsUnsignedLong

#endif /* HAVE_LONG_LONG */

#if PY_VERSION_HEX < 0x03020000
typedef long Py_hash_t;
#endif

/* PyMem_RawMalloc appeared in Python 3.4.  For earlier versions, we can just
   fall back to PyMem_Malloc.  */

#if PY_VERSION_HEX < 0x03040000
#define PyMem_RawMalloc PyMem_Malloc
#endif

/* Python 2.6 did not wrap Py_DECREF in 'do {...} while (0)', leading
   to 'suggest explicit braces to avoid ambiguous ‘else’' gcc errors.
   Wrap it ourselves, so that callers don't need to care.  */

static inline void
gdb_Py_DECREF (void *op) /* ARI: editCase function */
{
  /* ... and Python 2.4 didn't cast OP to PyObject pointer on the
     '(op)->ob_refcnt' references within the macro.  Cast it ourselves
     too.  */
  Py_DECREF ((PyObject *) op);
}

#undef Py_DECREF
#define Py_DECREF(op) gdb_Py_DECREF (op)

/* The second argument to PyObject_GetAttrString was missing the 'const'
   qualifier in Python-2.4.  Hence, we wrap it in a function to avoid errors
   when compiled with -Werror.  */

static inline PyObject *
gdb_PyObject_GetAttrString (PyObject *obj,
			    const char *attr) /* ARI: editCase function */
{
  return PyObject_GetAttrString (obj, (char *) attr);
}

#define PyObject_GetAttrString(obj, attr) gdb_PyObject_GetAttrString (obj, attr)

/* The second argument to PyObject_HasAttrString was also missing the 'const'
   qualifier in Python-2.4.  Hence, we wrap it also in a function to avoid
   errors when compiled with -Werror.  */

static inline int
gdb_PyObject_HasAttrString (PyObject *obj,
			    const char *attr)  /* ARI: editCase function */
{
  return PyObject_HasAttrString (obj, (char *) attr);
}

#define PyObject_HasAttrString(obj, attr) gdb_PyObject_HasAttrString (obj, attr)

/* PyObject_CallMethod's 'method' and 'format' parameters were missing
   the 'const' qualifier before Python 3.4.  Hence, we wrap the
   function in our own version to avoid errors with string literals.
   Note, this is a variadic template because PyObject_CallMethod is a
   varargs function and Python doesn't have a "PyObject_VaCallMethod"
   variant taking a va_list that we could defer to instead.  */

template<typename... Args>
static inline PyObject *
gdb_PyObject_CallMethod (PyObject *o, const char *method, const char *format,
			 Args... args) /* ARI: editCase function */
{
  return PyObject_CallMethod (o,
			      const_cast<char *> (method),
			      const_cast<char *> (format),
			      args...);
}

#undef PyObject_CallMethod
#define PyObject_CallMethod gdb_PyObject_CallMethod

/* The 'name' parameter of PyErr_NewException was missing the 'const'
   qualifier in Python <= 3.4.  Hence, we wrap it in a function to
   avoid errors when compiled with -Werror.  */

static inline PyObject*
gdb_PyErr_NewException (const char *name, PyObject *base, PyObject *dict)
{
  return PyErr_NewException (const_cast<char *> (name), base, dict);
}

#define PyErr_NewException gdb_PyErr_NewException

/* PySys_GetObject's 'name' parameter was missing the 'const'
   qualifier before Python 3.4.  Hence, we wrap it in a function to
   avoid errors when compiled with -Werror.  */

static inline PyObject *
gdb_PySys_GetObject (const char *name)
{
  return PySys_GetObject (const_cast<char *> (name));
}

#define PySys_GetObject gdb_PySys_GetObject

/* PySys_SetPath's 'path' parameter was missing the 'const' qualifier
   before Python 3.6.  Hence, we wrap it in a function to avoid errors
   when compiled with -Werror.  */

#ifdef IS_PY3K
# define GDB_PYSYS_SETPATH_CHAR wchar_t
#else
# define GDB_PYSYS_SETPATH_CHAR char
#endif

static inline void
gdb_PySys_SetPath (const GDB_PYSYS_SETPATH_CHAR *path)
{
  PySys_SetPath (const_cast<GDB_PYSYS_SETPATH_CHAR *> (path));
}

#define PySys_SetPath gdb_PySys_SetPath

/* Wrap PyGetSetDef to allow convenient construction with string
   literals.  Unfortunately, PyGetSetDef's 'name' and 'doc' members
   are 'char *' instead of 'const char *', meaning that in order to
   list-initialize PyGetSetDef arrays with string literals (and
   without the wrapping below) would require writing explicit 'char *'
   casts.  Instead, we extend PyGetSetDef and add constexpr
   constructors that accept const 'name' and 'doc', hiding the ugly
   casts here in a single place.  */

struct gdb_PyGetSetDef : PyGetSetDef
{
  constexpr gdb_PyGetSetDef (const char *name_, getter get_, setter set_,
			     const char *doc_, void *closure_)
    : PyGetSetDef {const_cast<char *> (name_), get_, set_,
		   const_cast<char *> (doc_), closure_}
  {}

  /* Alternative constructor that allows omitting the closure in list
     initialization.  */
  constexpr gdb_PyGetSetDef (const char *name_, getter get_, setter set_,
			     const char *doc_)
    : gdb_PyGetSetDef {name_, get_, set_, doc_, NULL}
  {}

  /* Constructor for the sentinel entries.  */
  constexpr gdb_PyGetSetDef (std::nullptr_t)
    : gdb_PyGetSetDef {NULL, NULL, NULL, NULL, NULL}
  {}
};

/* The 'keywords' parameter of PyArg_ParseTupleAndKeywords has type
   'char **'.  However, string literals are const in C++, and so to
   avoid casting at every keyword array definition, we'll need to make
   the keywords array an array of 'const char *'.  To avoid having all
   callers add a 'const_cast<char **>' themselves when passing such an
   array through 'char **', we define our own version of
   PyArg_ParseTupleAndKeywords here with a corresponding 'keywords'
   parameter type that does the cast in a single place.  (This is not
   an overload of PyArg_ParseTupleAndKeywords in order to make it
   clearer that we're calling our own function instead of a function
   that exists in some newer Python version.)  */

static inline int
gdb_PyArg_ParseTupleAndKeywords (PyObject *args, PyObject *kw,
				 const char *format, const char **keywords, ...)
{
  va_list ap;
  int res;

  va_start (ap, keywords);
  res = PyArg_VaParseTupleAndKeywords (args, kw, format,
				       const_cast<char **> (keywords),
				       ap);
  va_end (ap);

  return res;
}

/* In order to be able to parse symtab_and_line_to_sal_object function
   a real symtab_and_line structure is needed.  */
#include "symtab.h"

/* Also needed to parse enum var_types. */
#include "command.h"
#include "breakpoint.h"

enum gdbpy_iter_kind { iter_keys, iter_values, iter_items };

struct block;
struct value;
struct language_defn;
struct program_space;
struct bpstats;
struct inferior;

extern int gdb_python_initialized;

extern PyObject *gdb_module;
extern PyObject *gdb_python_module;
extern PyTypeObject value_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("value_object");
extern PyTypeObject block_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF("block_object");
extern PyTypeObject symbol_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("symbol_object");
extern PyTypeObject event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject stop_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject breakpoint_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("breakpoint_object");
extern PyTypeObject frame_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("frame_object");

typedef struct gdbpy_breakpoint_object
{
  PyObject_HEAD

  /* The breakpoint number according to gdb.  */
  int number;

  /* The gdb breakpoint object, or NULL if the breakpoint has been
     deleted.  */
  struct breakpoint *bp;

  /* 1 is this is a FinishBreakpoint object, 0 otherwise.  */
  int is_finish_bp;
} gdbpy_breakpoint_object;

/* Require that BREAKPOINT be a valid breakpoint ID; throw a Python
   exception if it is invalid.  */
#define BPPY_REQUIRE_VALID(Breakpoint)                                  \
    do {                                                                \
      if ((Breakpoint)->bp == NULL)                                     \
        return PyErr_Format (PyExc_RuntimeError,                        \
                             _("Breakpoint %d is invalid."),            \
                             (Breakpoint)->number);                     \
    } while (0)

/* Require that BREAKPOINT be a valid breakpoint ID; throw a Python
   exception if it is invalid.  This macro is for use in setter functions.  */
#define BPPY_SET_REQUIRE_VALID(Breakpoint)                              \
    do {                                                                \
      if ((Breakpoint)->bp == NULL)                                     \
        {                                                               \
          PyErr_Format (PyExc_RuntimeError, _("Breakpoint %d is invalid."), \
                        (Breakpoint)->number);                          \
          return -1;                                                    \
        }                                                               \
    } while (0)


/* Variables used to pass information between the Breakpoint
   constructor and the breakpoint-created hook function.  */
extern gdbpy_breakpoint_object *bppy_pending_object;


typedef struct
{
  PyObject_HEAD

  /* The thread we represent.  */
  struct thread_info *thread;

  /* The Inferior object to which this thread belongs.  */
  PyObject *inf_obj;
} thread_object;

extern struct cmd_list_element *set_python_list;
extern struct cmd_list_element *show_python_list;

/* extension_language_script_ops "methods".  */

extern int gdbpy_auto_load_enabled (const struct extension_language_defn *);

/* extension_language_ops "methods".  */

extern enum ext_lang_rc gdbpy_apply_val_pretty_printer
  (const struct extension_language_defn *,
   struct type *type,
   LONGEST embedded_offset, CORE_ADDR address,
   struct ui_file *stream, int recurse,
   struct value *val,
   const struct value_print_options *options,
   const struct language_defn *language);
extern enum ext_lang_bt_status gdbpy_apply_frame_filter
  (const struct extension_language_defn *,
   struct frame_info *frame, int flags, enum ext_lang_frame_args args_type,
   struct ui_out *out, int frame_low, int frame_high);
extern void gdbpy_preserve_values (const struct extension_language_defn *,
				   struct objfile *objfile,
				   htab_t copied_types);
extern enum ext_lang_bp_stop gdbpy_breakpoint_cond_says_stop
  (const struct extension_language_defn *, struct breakpoint *);
extern int gdbpy_breakpoint_has_cond (const struct extension_language_defn *,
				      struct breakpoint *b);

extern void *gdbpy_clone_xmethod_worker_data
  (const struct extension_language_defn *extlang, void *data);
extern void gdbpy_free_xmethod_worker_data
  (const struct extension_language_defn *extlang, void *data);
extern enum ext_lang_rc gdbpy_get_matching_xmethod_workers
  (const struct extension_language_defn *extlang,
   struct type *obj_type, const char *method_name,
   xmethod_worker_vec **dm_vec);
extern enum ext_lang_rc gdbpy_get_xmethod_arg_types
  (const struct extension_language_defn *extlang,
   struct xmethod_worker *worker,
   int *nargs,
   struct type ***arg_types);
extern enum ext_lang_rc gdbpy_get_xmethod_result_type
  (const struct extension_language_defn *extlang,
   struct xmethod_worker *worker,
   struct value *object, struct value **args, int nargs,
   struct type **result_type);
extern struct value *gdbpy_invoke_xmethod
  (const struct extension_language_defn *extlang,
   struct xmethod_worker *worker,
   struct value *obj, struct value **args, int nargs);

PyObject *gdbpy_history (PyObject *self, PyObject *args);
PyObject *gdbpy_breakpoints (PyObject *, PyObject *);
PyObject *gdbpy_frame_stop_reason_string (PyObject *, PyObject *);
PyObject *gdbpy_lookup_symbol (PyObject *self, PyObject *args, PyObject *kw);
PyObject *gdbpy_lookup_global_symbol (PyObject *self, PyObject *args,
				      PyObject *kw);
PyObject *gdbpy_start_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_current_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_stop_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_newest_frame (PyObject *self, PyObject *args);
PyObject *gdbpy_selected_frame (PyObject *self, PyObject *args);
PyObject *gdbpy_block_for_pc (PyObject *self, PyObject *args);
PyObject *gdbpy_lookup_type (PyObject *self, PyObject *args, PyObject *kw);
int gdbpy_is_field (PyObject *obj);
PyObject *gdbpy_create_lazy_string_object (CORE_ADDR address, long length,
					   const char *encoding,
					   struct type *type);
PyObject *gdbpy_inferiors (PyObject *unused, PyObject *unused2);
PyObject *gdbpy_create_ptid_object (ptid_t ptid);
PyObject *gdbpy_selected_thread (PyObject *self, PyObject *args);
PyObject *gdbpy_selected_inferior (PyObject *self, PyObject *args);
PyObject *gdbpy_string_to_argv (PyObject *self, PyObject *args);
PyObject *gdbpy_parameter_value (enum var_types type, void *var);
char *gdbpy_parse_command_name (const char *name,
				struct cmd_list_element ***base_list,
				struct cmd_list_element **start_list);

PyObject *symtab_and_line_to_sal_object (struct symtab_and_line sal);
PyObject *symtab_to_symtab_object (struct symtab *symtab);
PyObject *symbol_to_symbol_object (struct symbol *sym);
PyObject *block_to_block_object (const struct block *block,
				 struct objfile *objfile);
PyObject *value_to_value_object (struct value *v);
PyObject *type_to_type_object (struct type *);
PyObject *frame_info_to_frame_object (struct frame_info *frame);
PyObject *symtab_to_linetable_object (PyObject *symtab);
PyObject *pspace_to_pspace_object (struct program_space *)
    CPYCHECKER_RETURNS_BORROWED_REF;
PyObject *pspy_get_printers (PyObject *, void *);
PyObject *pspy_get_frame_filters (PyObject *, void *);
PyObject *pspy_get_frame_unwinders (PyObject *, void *);
PyObject *pspy_get_xmethods (PyObject *, void *);

PyObject *objfile_to_objfile_object (struct objfile *)
    CPYCHECKER_RETURNS_BORROWED_REF;
PyObject *objfpy_get_printers (PyObject *, void *);
PyObject *objfpy_get_frame_filters (PyObject *, void *);
PyObject *objfpy_get_frame_unwinders (PyObject *, void *);
PyObject *objfpy_get_xmethods (PyObject *, void *);
PyObject *gdbpy_lookup_objfile (PyObject *self, PyObject *args, PyObject *kw);

PyObject *gdbarch_to_arch_object (struct gdbarch *gdbarch);

thread_object *create_thread_object (struct thread_info *tp);
thread_object *find_thread_object (ptid_t ptid)
    CPYCHECKER_RETURNS_BORROWED_REF;
PyObject *find_inferior_object (int pid);
PyObject *inferior_to_inferior_object (struct inferior *inferior);

const struct block *block_object_to_block (PyObject *obj);
struct symbol *symbol_object_to_symbol (PyObject *obj);
struct value *value_object_to_value (PyObject *self);
struct value *convert_value_from_python (PyObject *obj);
struct type *type_object_to_type (PyObject *obj);
struct symtab *symtab_object_to_symtab (PyObject *obj);
struct symtab_and_line *sal_object_to_symtab_and_line (PyObject *obj);
struct frame_info *frame_object_to_frame_info (PyObject *frame_obj);
struct gdbarch *arch_object_to_gdbarch (PyObject *obj);

void gdbpy_initialize_gdb_readline (void);
int gdbpy_initialize_auto_load (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_values (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_frames (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_instruction (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_btrace (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_record (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_symtabs (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_commands (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_symbols (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_symtabs (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_blocks (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_types (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_functions (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_pspace (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_objfile (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_breakpoints (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_finishbreakpoints (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_lazy_string (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_linetable (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_parameters (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_thread (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_inferior (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_eventregistry (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_py_events (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_stop_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_signal_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_breakpoint_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_continue_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_inferior_call_pre_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_inferior_call_post_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_register_changed_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_memory_changed_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_exited_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_thread_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_new_objfile_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_clear_objfiles_event (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_arch (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_xmethods (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_unwind (void)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

/* Called before entering the Python interpreter to install the
   current language and architecture to be used for Python values.
   Also set the active extension language for GDB so that SIGINT's
   are directed our way, and if necessary install the right SIGINT
   handler.  */
class gdbpy_enter
{
 public:

  gdbpy_enter (struct gdbarch *gdbarch, const struct language_defn *language);

  ~gdbpy_enter ();

  gdbpy_enter (const gdbpy_enter &) = delete;
  gdbpy_enter &operator= (const gdbpy_enter &) = delete;

 private:

  struct active_ext_lang_state *m_previous_active;
  PyGILState_STATE m_state;
  struct gdbarch *m_gdbarch;
  const struct language_defn *m_language;
  PyObject *m_error_type, *m_error_value, *m_error_traceback;
};

/* Like gdbpy_enter, but takes a varobj.  This is a subclass just to
   make constructor delegation a little nicer.  */
class gdbpy_enter_varobj : public gdbpy_enter
{
 public:

  /* This is defined in varobj.c, where it can access varobj
     internals.  */
  gdbpy_enter_varobj (const struct varobj *var);

};

extern struct gdbarch *python_gdbarch;
extern const struct language_defn *python_language;

/* Use this after a TRY_EXCEPT to throw the appropriate Python
   exception.  */
#define GDB_PY_HANDLE_EXCEPTION(Exception)	\
  do {						\
    if (Exception.reason < 0)			\
      {						\
	gdbpy_convert_exception (Exception);	\
        return NULL;				\
      }						\
  } while (0)

/* Use this after a TRY_EXCEPT to throw the appropriate Python
   exception.  This macro is for use inside setter functions.  */
#define GDB_PY_SET_HANDLE_EXCEPTION(Exception)				\
    do {								\
      if (Exception.reason < 0)						\
        {								\
	  gdbpy_convert_exception (Exception);				\
	  return -1;							\
	}								\
    } while (0)

int gdbpy_print_python_errors_p (void);
void gdbpy_print_stack (void);

PyObject *python_string_to_unicode (PyObject *obj);
gdb::unique_xmalloc_ptr<char> unicode_to_target_string (PyObject *unicode_str);
gdb::unique_xmalloc_ptr<char> python_string_to_target_string (PyObject *obj);
PyObject *python_string_to_target_python_string (PyObject *obj);
gdb::unique_xmalloc_ptr<char> python_string_to_host_string (PyObject *obj);
PyObject *host_string_to_python_string (const char *str);
int gdbpy_is_string (PyObject *obj);
gdb::unique_xmalloc_ptr<char> gdbpy_obj_to_string (PyObject *obj);
gdb::unique_xmalloc_ptr<char> gdbpy_exception_to_string (PyObject *ptype,
							 PyObject *pvalue);

int gdbpy_is_lazy_string (PyObject *result);
void gdbpy_extract_lazy_string (PyObject *string, CORE_ADDR *addr,
				struct type **str_type,
				long *length,
				gdb::unique_xmalloc_ptr<char> *encoding);

int gdbpy_is_value_object (PyObject *obj);

/* Note that these are declared here, and not in python.h with the
   other pretty-printer functions, because they refer to PyObject.  */
PyObject *apply_varobj_pretty_printer (PyObject *print_obj,
				       struct value **replacement,
				       struct ui_file *stream);
PyObject *gdbpy_get_varobj_pretty_printer (struct value *value);
gdb::unique_xmalloc_ptr<char> gdbpy_get_display_hint (PyObject *printer);
PyObject *gdbpy_default_visualizer (PyObject *self, PyObject *args);

void bpfinishpy_pre_stop_hook (struct gdbpy_breakpoint_object *bp_obj);
void bpfinishpy_post_stop_hook (struct gdbpy_breakpoint_object *bp_obj);

extern PyObject *gdbpy_doc_cst;
extern PyObject *gdbpy_children_cst;
extern PyObject *gdbpy_to_string_cst;
extern PyObject *gdbpy_display_hint_cst;
extern PyObject *gdbpy_enabled_cst;
extern PyObject *gdbpy_value_cst;

/* Exception types.  */
extern PyObject *gdbpy_gdb_error;
extern PyObject *gdbpy_gdb_memory_error;
extern PyObject *gdbpy_gdberror_exc;

extern void gdbpy_convert_exception (struct gdb_exception)
    CPYCHECKER_SETS_EXCEPTION;

int get_addr_from_python (PyObject *obj, CORE_ADDR *addr)
    CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

PyObject *gdb_py_object_from_longest (LONGEST l);
PyObject *gdb_py_object_from_ulongest (ULONGEST l);
int gdb_py_int_as_long (PyObject *, long *);

PyObject *gdb_py_generic_dict (PyObject *self, void *closure);

int gdb_pymodule_addobject (PyObject *module, const char *name,
			    PyObject *object)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

struct varobj_iter;
struct varobj;
struct varobj_iter *py_varobj_get_iterator (struct varobj *var,
					    PyObject *printer);

#endif /* GDB_PYTHON_INTERNAL_H */
