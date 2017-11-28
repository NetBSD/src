/* Terminal interface definitions for GDB, the GNU Debugger.
   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#if !defined (TERMINAL_H)
#define TERMINAL_H 1

struct inferior;

extern void new_tty_prefork (const char *);

extern void new_tty (void);

extern void new_tty_postfork (void);

extern void copy_terminal_info (struct inferior *to, struct inferior *from);

/* Do we have job control?  Can be assumed to always be the same within
   a given run of GDB.  In inflow.c.  */
extern int job_control;

extern pid_t create_tty_session (void);

/* Set the process group of the caller to its own pid, or do nothing if
   we lack job control.  */
extern int gdb_setpgid (void);

/* Set up a serial structure describing standard input.  In inflow.c.  */
extern void initialize_stdin_serial (void);

extern void gdb_save_tty_state (void);

/* Take a snapshot of our initial tty state before readline/ncurses
   have had a chance to alter it.  */
extern void set_initial_gdb_ttystate (void);

/* Set the process group of the caller to its own pid, or do nothing
   if we lack job control.  */
extern int gdb_setpgid (void);

#endif /* !defined (TERMINAL_H) */
