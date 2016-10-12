/* Interface between gdb and its extension languages.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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

#ifndef EXTENSION_H
#define EXTENSION_H

#include "mi/mi-cmds.h" /* For PRINT_NO_VALUES, etc.  */
#include "common/vec.h"

struct breakpoint;
struct command_line;
struct frame_info;
struct language_defn;
struct objfile;
struct extension_language_defn;
struct type;
struct ui_file;
struct ui_out;
struct value;
struct value_print_options;

/* A function to load and process a script file.
   The file has been opened and is ready to be read from the beginning.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void script_sourcer_func (const struct extension_language_defn *,
				  FILE *stream, const char *filename);

/* A function to load and process a script for an objfile.
   The file has been opened and is ready to be read from the beginning.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void objfile_script_sourcer_func
  (const struct extension_language_defn *,
   struct objfile *, FILE *stream, const char *filename);

/* A function to execute a script for an objfile.
   Any exceptions are not caught, and are passed to the caller.  */
typedef void objfile_script_executor_func
  (const struct extension_language_defn *,
   struct objfile *, const char *name, const char *script);

/* Enum of each extension(/scripting) language.  */

enum extension_language
  {
    EXT_LANG_NONE,
    EXT_LANG_GDB,
    EXT_LANG_PYTHON,
    EXT_LANG_GUILE
  };

/* Extension language frame-filter status return values.  */

enum ext_lang_bt_status
  {
    /* Return when an error has occurred in processing frame filters,
       or when printing the stack.  */
    EXT_LANG_BT_ERROR = -1,

    /* Return from internal routines to indicate that the function
       succeeded.  */
    EXT_LANG_BT_OK = 1,

    /* Return when the frame filter process is complete, and all
       operations have succeeded.  */
    EXT_LANG_BT_COMPLETED = 2,

    /* Return when the frame filter process is complete, but there
       were no filter registered and enabled to process.  */
    EXT_LANG_BT_NO_FILTERS = 3
  };

/* Flags to pass to apply_extlang_frame_filter.  */

enum frame_filter_flags
  {
    /* Set this flag if frame level is to be printed.  */
    PRINT_LEVEL = 1,

    /* Set this flag if frame information is to be printed.  */
    PRINT_FRAME_INFO = 2,

    /* Set this flag if frame arguments are to be printed.  */
    PRINT_ARGS = 4,

    /* Set this flag if frame locals are to be printed.  */
    PRINT_LOCALS = 8,
  };

/* A choice of the different frame argument printing strategies that
   can occur in different cases of frame filter instantiation.  */

enum ext_lang_frame_args
  {
    /* Print no values for arguments when invoked from the MI. */
    NO_VALUES = PRINT_NO_VALUES,

    MI_PRINT_ALL_VALUES = PRINT_ALL_VALUES,

    /* Print only simple values (what MI defines as "simple") for
       arguments when invoked from the MI. */
    MI_PRINT_SIMPLE_VALUES = PRINT_SIMPLE_VALUES,

    /* Print only scalar values for arguments when invoked from the CLI. */
    CLI_SCALAR_VALUES,

    /* Print all values for arguments when invoked from the CLI. */
    CLI_ALL_VALUES
  };

/* The possible results of
   extension_language_ops.breakpoint_cond_says_stop.  */

enum ext_lang_bp_stop
  {
    /* No "stop" condition is set.  */
    EXT_LANG_BP_STOP_UNSET,

    /* A "stop" condition is set, and it says "don't stop".  */
    EXT_LANG_BP_STOP_NO,

    /* A "stop" condition is set, and it says "stop".  */
    EXT_LANG_BP_STOP_YES
  };

/* Table of type printers associated with the global typedef table.  */

struct ext_lang_type_printers
{
  /* Type-printers from Python.  */
  void *py_type_printers;
};

/* A type which holds its extension language specific xmethod worker data.  */

struct xmethod_worker
{
  /* The language the xmethod worker is implemented in.  */
  const struct extension_language_defn *extlang;

  /* The extension language specific data for this xmethod worker.  */
  void *data;

  /* The TYPE_CODE_XMETHOD value corresponding to this worker.
     Always use value_of_xmethod to access it.  */
  struct value *value;
};

typedef struct xmethod_worker *xmethod_worker_ptr;
DEF_VEC_P (xmethod_worker_ptr);
typedef VEC (xmethod_worker_ptr) xmethod_worker_vec;


/* The interface for gdb's own extension(/scripting) language.  */
extern const struct extension_language_defn extension_language_gdb;

extern const struct extension_language_defn *get_ext_lang_defn
  (enum extension_language lang);

extern const struct extension_language_defn *get_ext_lang_of_file
  (const char *file);

extern int ext_lang_present_p (const struct extension_language_defn *);

extern int ext_lang_initialized_p (const struct extension_language_defn *);

extern void throw_ext_lang_unsupported
  (const struct extension_language_defn *);

/* Accessors for "public" attributes of the extension language definition.  */

extern enum extension_language ext_lang_kind
  (const struct extension_language_defn *);

extern const char *ext_lang_name (const struct extension_language_defn *);

extern const char *ext_lang_capitalized_name
  (const struct extension_language_defn *);

extern const char *ext_lang_suffix (const struct extension_language_defn *);

extern const char *ext_lang_auto_load_suffix
  (const struct extension_language_defn *);

extern script_sourcer_func *ext_lang_script_sourcer
  (const struct extension_language_defn *);

extern objfile_script_sourcer_func *ext_lang_objfile_script_sourcer
  (const struct extension_language_defn *);

extern objfile_script_executor_func *ext_lang_objfile_script_executor
  (const struct extension_language_defn *);

extern int ext_lang_auto_load_enabled (const struct extension_language_defn *);

/* Wrappers for each extension language API function that iterate over all
   extension languages.  */

extern void finish_ext_lang_initialization (void);

extern void eval_ext_lang_from_control_command (struct command_line *cmd);

extern void auto_load_ext_lang_scripts_for_objfile (struct objfile *);

extern struct ext_lang_type_printers *start_ext_lang_type_printers (void);

extern char *apply_ext_lang_type_printers (struct ext_lang_type_printers *,
					   struct type *);

extern void free_ext_lang_type_printers (struct ext_lang_type_printers *);

extern int apply_ext_lang_val_pretty_printer
  (struct type *type, const gdb_byte *valaddr,
   LONGEST embedded_offset, CORE_ADDR address,
   struct ui_file *stream, int recurse,
   const struct value *val, const struct value_print_options *options,
   const struct language_defn *language);

extern enum ext_lang_bt_status apply_ext_lang_frame_filter
  (struct frame_info *frame, int flags, enum ext_lang_frame_args args_type,
   struct ui_out *out, int frame_low, int frame_high);

extern void preserve_ext_lang_values (struct objfile *, htab_t copied_types);

extern const struct extension_language_defn *get_breakpoint_cond_ext_lang
  (struct breakpoint *b, enum extension_language skip_lang);

extern int breakpoint_ext_lang_cond_says_stop (struct breakpoint *);

extern struct value *invoke_xmethod (struct xmethod_worker *,
				     struct value *,
				     struct value **, int nargs);

extern struct xmethod_worker *clone_xmethod_worker (struct xmethod_worker *);

extern struct xmethod_worker *new_xmethod_worker
  (const struct extension_language_defn *extlang, void *data);

extern void free_xmethod_worker (struct xmethod_worker *);

extern void free_xmethod_worker_vec (void *vec);

extern xmethod_worker_vec *get_matching_xmethod_workers
  (struct type *, const char *);

extern struct type **get_xmethod_arg_types (struct xmethod_worker *, int *);

extern struct type *get_xmethod_result_type (struct xmethod_worker *,
					     struct value *object,
					     struct value **args, int nargs);

#endif /* EXTENSION_H */
