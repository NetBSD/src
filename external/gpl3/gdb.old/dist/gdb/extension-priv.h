/* Private implementation details of interface between gdb and its
   extension languages.

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

#ifndef EXTENSION_PRIV_H
#define EXTENSION_PRIV_H

#include "extension.h"
#include <signal.h>

/* The return code for some API calls.  */

enum ext_lang_rc
  {
    /* The operation completed successfully.  */
    EXT_LANG_RC_OK,

    /* The operation was not performed (e.g., no pretty-printer).  */
    EXT_LANG_RC_NOP,

    /* There was an error (e.g., Python error while printing a value).
       When an error occurs no further extension languages are tried.
       This is to preserve existing behaviour, and because it's convenient
       for Python developers.
       Note: This is different than encountering a memory error trying to read
       a value for pretty-printing.  Here we're referring to, e.g., programming
       errors that trigger an exception in the extension language.  */
    EXT_LANG_RC_ERROR
  };

/* High level description of an extension/scripting language.
   An entry for each is compiled into GDB regardless of whether the support
   is present.  This is done so that we can issue meaningful errors if the
   support is not compiled in.  */

struct extension_language_defn
{
  /* Enum of the extension language.  */
  enum extension_language language;

  /* The name of the extension language, lowercase.  E.g., python.  */
  const char *name;

  /* The capitalized name of the extension language.
     For python this is "Python".  For gdb this is "GDB".  */
  const char *capitalized_name;

  /* The file suffix of this extension language.  E.g., ".py".  */
  const char *suffix;

  /* The suffix of per-objfile scripts to auto-load.
     E.g., When the program loads libfoo.so, look for libfoo.so-gdb.py.  */
  const char *auto_load_suffix;

  /* We support embedding external extension language code in GDB's own
     scripting language.  We do this by having a special command that begins
     the extension language snippet, and terminate it with "end".
     This specifies the control type used to implement this.  */
  enum command_control_type cli_control_type;

  /* A pointer to the "methods" to load scripts in this language,
     or NULL if the support is not compiled into GDB.  */
  const struct extension_language_script_ops *script_ops;

  /* Either a pointer to the "methods" of the extension language interface
     or NULL if the support is not compiled into GDB.
     This is also NULL for GDB's own scripting language which is relatively
     primitive, and doesn't provide these features.  */
  const struct extension_language_ops *ops;
};

/* The interface for loading scripts from external extension languages,
   as well as GDB's own scripting language.
   All of these methods are required to be implemented.

   By convention all of these functions take a pseudo-this parameter
   as the first argument.  */

struct extension_language_script_ops
{
  /* Load a script.  This is called, e.g., via the "source" command.
     If there's an error while processing the script this function may,
     but is not required to, throw an error.  */
  script_sourcer_func *script_sourcer;

  /* Load a script attached to an objfile.
     If there's an error while processing the script this function may,
     but is not required to, throw an error.  */
  objfile_script_sourcer_func *objfile_script_sourcer;

  /* Execute a script attached to an objfile.
     If there's an error while processing the script this function may,
     but is not required to, throw an error.  */
  objfile_script_executor_func *objfile_script_executor;

  /* Return non-zero if auto-loading scripts in this extension language
     is enabled.  */
  int (*auto_load_enabled) (const struct extension_language_defn *);
};

/* The interface for making calls from GDB to an external extension
   language.  This is for non-script-loading related functionality, like
   pretty-printing, etc.  The reason these are separated out is GDB's own
   scripting language makes use of extension_language_script_opts, but it
   makes no use of these.  There is no (current) intention to split
   extension_language_ops up any further.
   All of these methods are optional and may be NULL, except where
   otherwise indicated.

   By convention all of these functions take a pseudo-this parameter
   as the first argument.  */

struct extension_language_ops
{
  /* Called at the end of gdb initialization to give the extension language
     an opportunity to finish up.  This is useful for things like adding
     new commands where one has to wait until gdb itself is initialized.  */
  void (*finish_initialization) (const struct extension_language_defn *);

  /* Return non-zero if the extension language successfully initialized.
     This method is required.  */
  int (*initialized) (const struct extension_language_defn *);

  /* Process a sequence of commands embedded in GDB's own scripting language.
     E.g.,
     python
     print 42
     end  */
  void (*eval_from_control_command) (const struct extension_language_defn *,
				     struct command_line *);

  /* Type-printing support:
     start_type_printers, apply_type_printers, free_type_printers.
     These methods are optional and may be NULL, but if one of them is
     implemented then they all must be.  */

  /* Called before printing a type.  */
  void (*start_type_printers) (const struct extension_language_defn *,
			       struct ext_lang_type_printers *);

  /* Try to pretty-print TYPE.  If successful the pretty-printed type is
     stored in *PRETTIED_TYPE, and the caller must free it.
     Returns EXT_LANG_RC_OK upon success, EXT_LANG_RC_NOP if the type
     is not recognized, and EXT_LANG_RC_ERROR if an error was encountered.
     This function has a bit of a funny name, since it actually applies
     recognizers, but this seemed clearer given the start_type_printers
     and free_type_printers functions.  */
  enum ext_lang_rc (*apply_type_printers)
    (const struct extension_language_defn *,
     const struct ext_lang_type_printers *,
     struct type *, char **prettied_type);

  /* Called after a type has been printed to give the type pretty-printer
     mechanism an opportunity to clean up.  */
  void (*free_type_printers) (const struct extension_language_defn *,
			      struct ext_lang_type_printers *);

  /* Try to pretty-print a value of type TYPE located at VAL's contents
     buffer + EMBEDDED_OFFSET, which came from the inferior at address
     ADDRESS + EMBEDDED_OFFSET, onto stdio stream STREAM according to
     OPTIONS.
     VAL is the whole object that came from ADDRESS.
     Returns EXT_LANG_RC_OK upon success, EXT_LANG_RC_NOP if the value
     is not recognized, and EXT_LANG_RC_ERROR if an error was encountered.  */
  enum ext_lang_rc (*apply_val_pretty_printer)
    (const struct extension_language_defn *,
     struct type *type,
     LONGEST embedded_offset, CORE_ADDR address,
     struct ui_file *stream, int recurse,
     struct value *val, const struct value_print_options *options,
     const struct language_defn *language);

  /* GDB access to the "frame filter" feature.
     FRAME is the source frame to start frame-filter invocation.  FLAGS is an
     integer holding the flags for printing.  The following elements of
     the FRAME_FILTER_FLAGS enum denotes the make-up of FLAGS:
     PRINT_LEVEL is a flag indicating whether to print the frame's
     relative level in the output.  PRINT_FRAME_INFO is a flag that
     indicates whether this function should print the frame
     information, PRINT_ARGS is a flag that indicates whether to print
     frame arguments, and PRINT_LOCALS, likewise, with frame local
     variables.  ARGS_TYPE is an enumerator describing the argument
     format, OUT is the output stream to print.  FRAME_LOW is the
     beginning of the slice of frames to print, and FRAME_HIGH is the
     upper limit of the frames to count.  Returns SCR_BT_ERROR on error,
     or SCR_BT_COMPLETED on success.  */
  enum ext_lang_bt_status (*apply_frame_filter)
    (const struct extension_language_defn *,
     struct frame_info *frame, int flags, enum ext_lang_frame_args args_type,
     struct ui_out *out, int frame_low, int frame_high);

  /* Update values held by the extension language when OBJFILE is discarded.
     New global types must be created for every such value, which must then be
     updated to use the new types.
     This function typically just iterates over all appropriate values and
     calls preserve_one_value for each one.
     COPIED_TYPES is used to prevent cycles / duplicates and is passed to
     preserve_one_value.  */
  void (*preserve_values) (const struct extension_language_defn *,
			   struct objfile *objfile, htab_t copied_types);

  /* Return non-zero if there is a stop condition for the breakpoint.
     This is used to implement the restriction that a breakpoint may have
     at most one condition.  */
  int (*breakpoint_has_cond) (const struct extension_language_defn *,
			      struct breakpoint *);

  /* Return a value of enum ext_lang_bp_stop indicating if there is a stop
     condition for the breakpoint, and if so whether the program should
     stop.  This is called when the program has stopped at the specified
     breakpoint.
     While breakpoints can have at most one condition, this is called for
     every extension language, even if another extension language has a
     "stop" method: other kinds of breakpoints may be implemented using
     this method, e.g., "finish breakpoints" in Python.  */
  enum ext_lang_bp_stop (*breakpoint_cond_says_stop)
    (const struct extension_language_defn *, struct breakpoint *);

  /* The next two are used to connect GDB's SIGINT handling with the
     extension language's.

     Terminology: If an extension language can use GDB's SIGINT handling then
     we say the extension language has "cooperative SIGINT handling".
     Python is an example of this.

     These need not be implemented, but if one of them is implemented
     then they all must be.  */

  /* Set the SIGINT indicator.
     This is called by GDB's SIGINT handler and must be async-safe.  */
  void (*set_quit_flag) (const struct extension_language_defn *);

  /* Return non-zero if a SIGINT has occurred.
     This is expected to also clear the indicator.  */
  int (*check_quit_flag) (const struct extension_language_defn *);

  /* Called before gdb prints its prompt, giving extension languages an
     opportunity to change it with set_prompt.
     Returns EXT_LANG_RC_OK if the prompt was changed, EXT_LANG_RC_NOP if
     the prompt was not changed, and EXT_LANG_RC_ERROR if an error was
     encountered.
     Extension languages are called in order, and once the prompt is
     changed or an error occurs no further languages are called.  */
  enum ext_lang_rc (*before_prompt) (const struct extension_language_defn *,
				     const char *current_gdb_prompt);

  /* xmethod support:
     clone_xmethod_worker_data, free_xmethod_worker_data,
     get_matching_xmethod_workers, get_xmethod_arg_types,
     get_xmethod_return_type, invoke_xmethod.
     These methods are optional and may be NULL, but if one of them is
     implemented then they all must be.  */

  /* Clone DATA and return a new but identical xmethod worker data
     object for this extension language.  */
  void * (*clone_xmethod_worker_data)
    (const struct extension_language_defn *extlang, void *data);

  /* Free the DATA object of this extension language.  */
  void (*free_xmethod_worker_data)
    (const struct extension_language_defn *extlang, void *data);

  /* Return a vector of matching xmethod workers defined in this
     extension language.  The workers service methods with name
     METHOD_NAME on objects of type OBJ_TYPE.  The vector is returned
     in DM_VEC.  */
  enum ext_lang_rc (*get_matching_xmethod_workers)
    (const struct extension_language_defn *extlang,
     struct type *obj_type,
     const char *method_name,
     xmethod_worker_vec **dm_vec);

  /* Given a WORKER servicing a particular method, return the types
     of the arguments the method takes.  The number of arguments is
     returned in NARGS, and their types are returned in the array
     ARGTYPES.  */
  enum ext_lang_rc (*get_xmethod_arg_types)
    (const struct extension_language_defn *extlang,
     struct xmethod_worker *worker,
     int *nargs,
     struct type ***arg_types);

  /* Given a WORKER servicing a particular method, fetch the type of the
     result of the method.  OBJECT, ARGS, NARGS are the same as for
     invoke_xmethod.  The result type is stored in *RESULT_TYPE.
     For backward compatibility with 7.9, which did not support getting the
     result type, if the get_result_type operation is not provided by WORKER
     then EXT_LANG_RC_OK is returned and NULL is returned in *RESULT_TYPE.  */
  enum ext_lang_rc (*get_xmethod_result_type)
    (const struct extension_language_defn *extlang,
     struct xmethod_worker *worker,
     struct value *object, struct value **args, int nargs,
     struct type **result_type);

  /* Invoke the xmethod serviced by WORKER.  The xmethod is invoked
     on OBJECT with arguments in the array ARGS.  NARGS is the length of
     this array.  Returns the value returned by the xmethod.  */
  struct value * (*invoke_xmethod)
    (const struct extension_language_defn *extlang,
     struct xmethod_worker *worker,
     struct value *object,
     struct value **args,
     int nargs);
};

/* State necessary to restore a signal handler to its previous value.  */

struct signal_handler
{
  /* Non-zero if "handler" has been set.  */
  int handler_saved;

  /* The signal handler.  */
  sighandler_t handler;
};

/* State necessary to restore the currently active extension language
   to its previous value.  */

struct active_ext_lang_state
{
  /* The previously active extension language.  */
  const struct extension_language_defn *ext_lang;

  /* Its SIGINT handler.  */
  struct signal_handler sigint_handler;
};

extern const struct extension_language_defn *get_active_ext_lang (void);

extern struct active_ext_lang_state *set_active_ext_lang
  (const struct extension_language_defn *);

extern void restore_active_ext_lang (struct active_ext_lang_state *previous);

#endif /* EXTENSION_PRIV_H */
