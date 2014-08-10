/* MI Command Set - symbol commands.
   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
#include "mi-cmds.h"
#include "symtab.h"
#include "objfiles.h"
#include "ui-out.h"

/* Print the list of all pc addresses and lines of code for the
   provided (full or base) source file name.  The entries are sorted
   in ascending PC order.  */

void
mi_cmd_symbol_list_lines (char *command, char **argv, int argc)
{
  struct gdbarch *gdbarch;
  char *filename;
  struct symtab *s;
  int i;
  struct cleanup *cleanup_stack, *cleanup_tuple;
  struct ui_out *uiout = current_uiout;

  if (argc != 1)
    error (_("-symbol-list-lines: Usage: SOURCE_FILENAME"));

  filename = argv[0];
  s = lookup_symtab (filename);

  if (s == NULL)
    error (_("-symbol-list-lines: Unknown source file name."));

  /* Now, dump the associated line table.  The pc addresses are
     already sorted by increasing values in the symbol table, so no
     need to perform any other sorting.  */

  gdbarch = get_objfile_arch (s->objfile);
  cleanup_stack = make_cleanup_ui_out_list_begin_end (uiout, "lines");

  if (LINETABLE (s) != NULL && LINETABLE (s)->nitems > 0)
    for (i = 0; i < LINETABLE (s)->nitems; i++)
    {
      cleanup_tuple = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      ui_out_field_core_addr (uiout, "pc", gdbarch, LINETABLE (s)->item[i].pc);
      ui_out_field_int (uiout, "line", LINETABLE (s)->item[i].line);
      do_cleanups (cleanup_tuple);
    }

  do_cleanups (cleanup_stack);
}
