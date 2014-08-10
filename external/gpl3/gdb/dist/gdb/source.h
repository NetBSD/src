/* List lines of source files for GDB, the GNU debugger.
   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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

#ifndef SOURCE_H
#define SOURCE_H

struct symtab;

/* This function is capable of finding the absolute path to a
   source file, and opening it, provided you give it a FILENAME.  Both the
   DIRNAME and FULLNAME are only added suggestions on where to find the file.

   FILENAME should be the filename to open.
   DIRNAME is the compilation directory of a particular source file.
	   Only some debug formats provide this info.
   FULLNAME can be the last known absolute path to the file in question.
     Space for the path must have been malloc'd.  If a path substitution
     is applied we free the old value and set a new one.

   On Success
     A valid file descriptor is returned (the return value is positive).
     FULLNAME is set to the absolute path to the file just opened.
     The caller is responsible for freeing FULLNAME.

   On Failure
     An invalid file descriptor is returned (the return value is negative).
     FULLNAME is set to NULL.  */
extern int find_and_open_source (const char *filename,
				 const char *dirname,
				 char **fullname);

/* Open a source file given a symtab S.  Returns a file descriptor or
   negative number for error.  */
extern int open_source_file (struct symtab *s);

extern char *rewrite_source_path (const char *path);

extern const char *symtab_to_fullname (struct symtab *s);

/* Returns filename without the compile directory part, basename or absolute
   filename.  It depends on 'set filename-display' value.  */
extern const char *symtab_to_filename_for_display (struct symtab *symtab);

/* Create and initialize the table S->line_charpos that records the
   positions of the lines in the source file, which is assumed to be
   open on descriptor DESC.  All set S->nlines to the number of such
   lines.  */
extern void find_source_lines (struct symtab *s, int desc);

/* Return the first line listed by print_source_lines.
   Used by command interpreters to request listing from
   a previous point.  */
extern int get_first_line_listed (void);

/* Return the default number of lines to print with commands like the
   cli "list".  The caller of print_source_lines must use this to
   calculate the end line and use it in the call to print_source_lines
   as it does not automatically use this value.  */
extern int get_lines_to_list (void);

/* Return the current source file for listing and next line to list.
   NOTE: The returned sal pc and end fields are not valid.  */
extern struct symtab_and_line get_current_source_symtab_and_line (void);

/* If the current source file for listing is not set, try and get a default.
   Usually called before get_current_source_symtab_and_line() is called.
   It may err out if a default cannot be determined.
   We must be cautious about where it is called, as it can recurse as the
   process of determining a new default may call the caller!
   Use get_current_source_symtab_and_line only to get whatever
   we have without erroring out or trying to get a default.  */
extern void set_default_source_symtab_and_line (void);

/* Return the current default file for listing and next line to list
   (the returned sal pc and end fields are not valid.)
   and set the current default to whatever is in SAL.
   NOTE: The returned sal pc and end fields are not valid.  */
extern struct symtab_and_line set_current_source_symtab_and_line (const struct symtab_and_line *);

/* Reset any information stored about a default file and line to print.  */
extern void clear_current_source_symtab_and_line (void);

/* Add a source path substitution rule.  */
extern void add_substitute_path_rule (char *, char *);
#endif
