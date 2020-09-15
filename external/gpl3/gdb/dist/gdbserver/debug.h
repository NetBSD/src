/* Debugging routines for the remote server for GDB.
   Copyright (C) 2014-2020 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_DEBUG_H
#define GDBSERVER_DEBUG_H

#if !defined (IN_PROCESS_AGENT)
extern int remote_debug;

/* Switch all debug output to DEBUG_FILE.  If DEBUG_FILE is nullptr or an
   empty string, or if the file cannot be opened, then debug output is sent to
   stderr.  */
void debug_set_output (const char *debug_file);
#endif

extern int using_threads;

/* Enable miscellaneous debugging output.  The name is historical - it
   was originally used to debug LinuxThreads support.  */

extern int debug_threads;

extern int debug_timestamp;

void debug_flush (void);
void do_debug_enter (const char *function_name);
void do_debug_exit (const char *function_name);

/* Async signal safe debug output function that calls write directly.  */
ssize_t debug_write (const void *buf, size_t nbyte);

/* These macros are for use in major functions that produce a lot of
   debugging output.  They help identify in the mass of debugging output
   when these functions enter and exit.  debug_enter is intended to be
   called at the start of a function, before any other debugging output.
   debug_exit is intended to be called at the end of the same function,
   after all debugging output.  */
#ifdef FUNCTION_NAME
#define debug_enter() \
  do { do_debug_enter (FUNCTION_NAME); } while (0)
#define debug_exit() \
  do { do_debug_exit (FUNCTION_NAME); } while (0)
#else
#define debug_enter() \
  do { } while (0)
#define debug_exit() \
  do { } while (0)
#endif

#endif /* GDBSERVER_DEBUG_H */
