/* Top level stuff for GDB, the GNU debugger.
   Copyright 1986-1994, 2000 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* From top.c.  */
extern char *line;
extern int linesize;
extern FILE *instream;
extern char gdb_dirbuf[1024];
extern int inhibit_gdbinit;
extern int epoch_interface;
extern char gdbinit[];

extern void print_gdb_version (struct ui_file *);

extern void source_command PARAMS ((char *, int));
extern void cd_command PARAMS ((char *, int));
extern void read_command_file PARAMS ((FILE *));
extern void init_history PARAMS ((void));
extern void command_loop PARAMS ((void));
extern void simplified_command_loop PARAMS ((char *(*read_input_func) (char *),
			       void (*execute_command_func) (char *, int)));
extern int quit_confirm PARAMS ((void));
extern void quit_force PARAMS ((char *, int));
extern void quit_command PARAMS ((char *, int));
extern void command_loop_marker PARAMS ((int));
extern int quit_cover PARAMS ((PTR));
extern void execute_command PARAMS ((char *, int));

/* This function returns a pointer to the string that is used
   by gdb for its command prompt. */
extern char *get_prompt PARAMS ((void));

/* This function copies the specified string into the string that
   is used by gdb for its command prompt. */
extern void set_prompt PARAMS ((char *));

/* From random places.  */
extern int mapped_symbol_files;
extern int readnow_symbol_files;

/* Perform _initialize initialization */
extern void gdb_init PARAMS ((char *));

/* For use by event-top.c */
/* Variables from top.c. */
extern int source_line_number;
extern char *source_file_name;
extern char *source_error;
extern char *source_pre_error;
extern int history_expansion_p;
extern int server_command;
