/* Python/gdb header for generic use in gdb

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

#ifndef GDB_PYTHON_H
#define GDB_PYTHON_H

#include "value.h"
#include "mi/mi-cmds.h"

struct gdbpy_breakpoint_object;

/* The suffix of per-objfile scripts to auto-load.
   E.g. When the program loads libfoo.so, look for libfoo-gdb.py.  */
#define GDBPY_AUTO_FILE_NAME "-gdb.py"

/* Python frame-filter status return values.  */
enum py_bt_status
  {
    /* Return when an error has occurred in processing frame filters,
       or when printing the stack.  */
    PY_BT_ERROR = -1,

    /* Return from internal routines to indicate that the function
       succeeded.  */
    PY_BT_OK = 1,

    /* Return when the frame filter process is complete, and all
       operations have succeeded.  */
    PY_BT_COMPLETED = 2,

    /* Return when the frame filter process is complete, but there
       were no filter registered and enabled to process. */
    PY_BT_NO_FILTERS = 3
  };

/* Flags to pass to apply_frame_filter.  */

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
typedef enum py_frame_args
{
  /* Print no values for arguments when invoked from the MI. */
  NO_VALUES = PRINT_NO_VALUES,

  MI_PRINT_ALL_VALUES = PRINT_ALL_VALUES,

  /* Print only simple values (what MI defines as "simple") for
     arguments when invoked from the MI. */
  MI_PRINT_SIMPLE_VALUES = PRINT_SIMPLE_VALUES,


  /* Print only scalar values for arguments when invoked from the
     CLI. */
  CLI_SCALAR_VALUES,

  /* Print all values for arguments when invoked from the
     CLI. */
  CLI_ALL_VALUES
} py_frame_args;

/* Returns true if Python support is built into GDB, false
   otherwise.  */

static inline int
have_python (void)
{
#ifdef HAVE_PYTHON
  return 1;
#else
  return 0;
#endif
}

extern void finish_python_initialization (void);

void eval_python_from_control_command (struct command_line *);

void source_python_script (FILE *file, const char *filename);

int apply_val_pretty_printer (struct type *type, const gdb_byte *valaddr,
			      int embedded_offset, CORE_ADDR address,
			      struct ui_file *stream, int recurse,
			      const struct value *val,
			      const struct value_print_options *options,
			      const struct language_defn *language);

enum py_bt_status apply_frame_filter (struct frame_info *frame, int flags,
				      enum py_frame_args args_type,
				      struct ui_out *out, int frame_low,
				      int frame_high);

void preserve_python_values (struct objfile *objfile, htab_t copied_types);

const struct script_language *gdbpy_script_language_defn (void);

int gdbpy_should_stop (struct gdbpy_breakpoint_object *bp_obj);

int gdbpy_breakpoint_has_py_cond (struct gdbpy_breakpoint_object *bp_obj);

void *start_type_printers (void);

char *apply_type_printers (void *, struct type *type);

void free_type_printers (void *arg);

#endif /* GDB_PYTHON_H */
