/* Header for GDB line completion.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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

#if !defined (COMPLETER_H)
#define COMPLETER_H 1

#include "gdb_vecs.h"

extern VEC (char_ptr) *complete_line (const char *text,
				      char *line_buffer,
				      int point);

extern char *readline_line_completion_function (const char *text,
						int matches);

extern VEC (char_ptr) *noop_completer (struct cmd_list_element *,
				       char *, char *);

extern VEC (char_ptr) *filename_completer (struct cmd_list_element *,
					   char *, char *);

extern VEC (char_ptr) *expression_completer (struct cmd_list_element *,
					     char *, char *);

extern VEC (char_ptr) *location_completer (struct cmd_list_element *,
					   char *, char *);

extern VEC (char_ptr) *command_completer (struct cmd_list_element *,
					  char *, char *);

extern VEC (char_ptr) *signal_completer (struct cmd_list_element *,
					 char *, char *);

extern char *get_gdb_completer_quote_characters (void);

extern char *gdb_completion_word_break_characters (void);

/* Exported to linespec.c */

extern char *skip_quoted_chars (char *, char *, char *);

extern char *skip_quoted (char *);

#endif /* defined (COMPLETER_H) */
