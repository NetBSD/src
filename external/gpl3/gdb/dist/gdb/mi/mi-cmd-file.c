/* MI Command Set - file commands.
   Copyright (C) 2000-2016 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-getopt.h"
#include "ui-out.h"
#include "symtab.h"
#include "source.h"
#include "objfiles.h"
#include "psymtab.h"

/* Return to the client the absolute path and line number of the 
   current file being executed.  */

void
mi_cmd_file_list_exec_source_file (char *command, char **argv, int argc)
{
  struct symtab_and_line st;
  struct ui_out *uiout = current_uiout;
  
  if (!mi_valid_noargs ("-file-list-exec-source-file", argc, argv))
    error (_("-file-list-exec-source-file: Usage: No args"));

  /* Set the default file and line, also get them.  */
  set_default_source_symtab_and_line ();
  st = get_current_source_symtab_and_line ();

  /* We should always get a symtab.  Apparently, filename does not
     need to be tested for NULL.  The documentation in symtab.h
     suggests it will always be correct.  */
  if (!st.symtab)
    error (_("-file-list-exec-source-file: No symtab"));

  /* Print to the user the line, filename and fullname.  */
  ui_out_field_int (uiout, "line", st.line);
  ui_out_field_string (uiout, "file",
		       symtab_to_filename_for_display (st.symtab));

  ui_out_field_string (uiout, "fullname", symtab_to_fullname (st.symtab));

  ui_out_field_int (uiout, "macro-info",
		    COMPUNIT_MACRO_TABLE
		      (SYMTAB_COMPUNIT (st.symtab)) != NULL);
}

/* A callback for map_partial_symbol_filenames.  */

static void
print_partial_file_name (const char *filename, const char *fullname,
			 void *ignore)
{
  struct ui_out *uiout = current_uiout;

  ui_out_begin (uiout, ui_out_type_tuple, NULL);

  ui_out_field_string (uiout, "file", filename);

  if (fullname)
    ui_out_field_string (uiout, "fullname", fullname);

  ui_out_end (uiout, ui_out_type_tuple);
}

void
mi_cmd_file_list_exec_source_files (char *command, char **argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct compunit_symtab *cu;
  struct symtab *s;
  struct objfile *objfile;

  if (!mi_valid_noargs ("-file-list-exec-source-files", argc, argv))
    error (_("-file-list-exec-source-files: Usage: No args"));

  /* Print the table header.  */
  ui_out_begin (uiout, ui_out_type_list, "files");

  /* Look at all of the file symtabs.  */
  ALL_FILETABS (objfile, cu, s)
  {
    ui_out_begin (uiout, ui_out_type_tuple, NULL);

    ui_out_field_string (uiout, "file", symtab_to_filename_for_display (s));
    ui_out_field_string (uiout, "fullname", symtab_to_fullname (s));

    ui_out_end (uiout, ui_out_type_tuple);
  }

  map_symbol_filenames (print_partial_file_name, NULL,
			1 /*need_fullname*/);

  ui_out_end (uiout, ui_out_type_list);
}
