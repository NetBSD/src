/* Definitions used by event-top.c, for GDB, the GNU debugger.

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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

#ifndef EVENT_TOP_H
#define EVENT_TOP_H

struct cmd_list_element;

/* Exported functions from event-top.c.
   FIXME: these should really go into top.h.  */

extern void display_gdb_prompt (const char *new_prompt);
extern void gdb_setup_readline (int);
extern void gdb_disable_readline (void);
extern void async_init_signals (void);
extern void change_line_handler (int);

extern void command_line_handler (char *rl);
extern void command_handler (char *command);

/* Signal to catch ^Z typed while reading a command: SIGTSTP or SIGCONT.  */
#ifndef STOP_SIGNAL
#include <signal.h>
#ifdef SIGTSTP
#define STOP_SIGNAL SIGTSTP
extern void handle_stop_sig (int sig);
#endif
#endif
extern void handle_sigint (int sig);
extern void handle_sigterm (int sig);
extern void async_request_quit (void *arg);
extern void stdin_event_handler (int error, void *client_data);
extern void async_disable_stdin (void);
extern void async_enable_stdin (void);

/* Exported variables from event-top.c.
   FIXME: these should really go into top.h.  */

extern int set_editing_cmd_var;
extern int exec_done_display_p;
extern struct prompts the_prompts;
extern void (*after_char_processing_hook) (void);
extern int call_stdin_event_handler_again_p;
extern void gdb_readline_no_editing_callback (void *client_data);

/* Wrappers for rl_callback_handler_remove and
   rl_callback_handler_install that keep track of whether the callback
   handler is installed in readline.  Do not call the readline
   versions directly.  */
extern void gdb_rl_callback_handler_remove (void);
extern void gdb_rl_callback_handler_install (const char *prompt);

/* Reinstall the readline callback handler (with no prompt), if not
   currently installed.  */
extern void gdb_rl_callback_handler_reinstall (void);

extern void cli_command_loop (void *);

#endif
