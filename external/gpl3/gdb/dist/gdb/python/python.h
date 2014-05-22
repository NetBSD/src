/* Python/gdb header for generic use in gdb

   Copyright (C) 2008-2013 Free Software Foundation, Inc.

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

struct breakpoint_object;

/* The suffix of per-objfile scripts to auto-load.
   E.g. When the program loads libfoo.so, look for libfoo-gdb.py.  */
#define GDBPY_AUTO_FILE_NAME "-gdb.py"

extern void finish_python_initialization (void);

void eval_python_from_control_command (struct command_line *);

void source_python_script (FILE *file, const char *filename);

int apply_val_pretty_printer (struct type *type, const gdb_byte *valaddr,
			      int embedded_offset, CORE_ADDR address,
			      struct ui_file *stream, int recurse,
			      const struct value *val,
			      const struct value_print_options *options,
			      const struct language_defn *language);

void preserve_python_values (struct objfile *objfile, htab_t copied_types);

void gdbpy_load_auto_scripts_for_objfile (struct objfile *objfile);

int gdbpy_should_stop (struct breakpoint_object *bp_obj);

int gdbpy_breakpoint_has_py_cond (struct breakpoint_object *bp_obj);

void *start_type_printers (void);

char *apply_type_printers (void *, struct type *type);

void free_type_printers (void *arg);

#endif /* GDB_PYTHON_H */
