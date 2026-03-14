/* Terminal interface definitions for GDB, the GNU Debugger.
   Copyright (C) 1986-2025 Free Software Foundation, Inc.

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

#ifndef GDB_TERMINAL_H
#define GDB_TERMINAL_H

#include "serial.h"

struct inferior;

extern void new_tty_prefork (std::string ttyname);

extern void new_tty (void);

extern void new_tty_postfork (void);

extern void copy_terminal_info (struct inferior *to, struct inferior *from);

/* Exchange the terminal info and state between inferiors A and B.  */
extern void swap_terminal_info (inferior *a, inferior *b);

extern pid_t create_tty_session (void);

/* Set up a serial structure describing standard input.  In inflow.c.  */
extern void initialize_stdin_serial (void);

extern void gdb_save_tty_state (void);

/* Take a snapshot of our initial tty state before readline/ncurses
   have had a chance to alter it.  */
extern void set_initial_gdb_ttystate (void);

/* Restore initial tty state.  */
extern void restore_initial_gdb_ttystate (void);

/* An RAII-based object that saves the tty state, and then restores it again
   when this object is destroyed.  */
class scoped_restore_tty_state
{
public:
  scoped_restore_tty_state ();
  ~scoped_restore_tty_state ();

  DISABLE_COPY_AND_ASSIGN (scoped_restore_tty_state);

private:
  /* The saved tty state.  This can remain NULL even after the constructor
     has run if serial_get_tty_state fails to fetch the tty state.  */
  serial_ttystate m_ttystate = nullptr;
};

#ifdef USE_WIN32API
/* Set translation mode of stdout/stderr to binary.  */
extern void set_output_translation_mode_binary ();
#endif

#endif /* GDB_TERMINAL_H */
