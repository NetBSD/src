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

#include "server.h"
#include <chrono>

#if !defined (IN_PROCESS_AGENT)
int remote_debug = 0;
#endif

/* Output file for debugging.  Default to standard error.  */
FILE *debug_file = stderr;

/* See debug.h.  */
int debug_threads;

/* Include timestamps in debugging output.  */
int debug_timestamp;

#if !defined (IN_PROCESS_AGENT)

/* See debug.h.  */

void
debug_set_output (const char *new_debug_file)
{
  /* Close any existing file and reset to standard error.  */
  if (debug_file != stderr)
    {
      fclose (debug_file);
    }
  debug_file = stderr;

  /* Catch empty filenames.  */
  if (new_debug_file == nullptr || strlen (new_debug_file) == 0)
    return;

  FILE *fptr = fopen (new_debug_file, "w");

  if (fptr == nullptr)
    {
      debug_printf ("Cannot open %s for writing. %s. Switching to stderr.\n",
		    new_debug_file, safe_strerror (errno));
      return;
    }

  debug_file = fptr;
}

#endif

/* Print a debugging message.
   If the text begins a new line it is preceded by a timestamp.
   We don't get fancy with newline checking, we just check whether the
   previous call ended with "\n".  */

void
debug_vprintf (const char *format, va_list ap)
{
#if !defined (IN_PROCESS_AGENT)
  /* N.B. Not thread safe, and can't be used, as is, with IPA.  */
  static int new_line = 1;

  if (debug_timestamp && new_line)
    {
      using namespace std::chrono;

      steady_clock::time_point now = steady_clock::now ();
      seconds s = duration_cast<seconds> (now.time_since_epoch ());
      microseconds us = duration_cast<microseconds> (now.time_since_epoch ()) - s;

      fprintf (debug_file, "%ld.%06ld ", (long) s.count (), (long) us.count ());
    }
#endif

  vfprintf (debug_file, format, ap);

#if !defined (IN_PROCESS_AGENT)
  if (*format)
    new_line = format[strlen (format) - 1] == '\n';
#endif
}

/* Flush debugging output.
   This is called, for example, when starting an inferior to ensure all debug
   output thus far appears before any inferior output.  */

void
debug_flush (void)
{
  fflush (debug_file);
}

/* Notify the user that the code is entering FUNCTION_NAME.
   FUNCTION_NAME is the name of the calling function, or NULL if unknown.

   This is intended to be called via the debug_enter macro.  */

void
do_debug_enter (const char *function_name)
{
  if (function_name != NULL)
    debug_printf (">>>> entering %s\n", function_name);
}

/* Notify the user that the code is exiting FUNCTION_NAME.
   FUNCTION_NAME is the name of the calling function, or NULL if unknown.

   This is intended to be called via the debug_exit macro.  */

void
do_debug_exit (const char *function_name)
{
  if (function_name != NULL)
    debug_printf ("<<<< exiting %s\n", function_name);
}

/* See debug.h.  */

ssize_t
debug_write (const void *buf, size_t nbyte)
{
  int fd = fileno (debug_file);
  return write (fd, buf, nbyte);
}
