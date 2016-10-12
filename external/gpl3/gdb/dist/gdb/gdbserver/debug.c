/* Debugging routines for the remote server for GDB.
   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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
#include "gdb_sys_time.h"

/* Enable miscellaneous debugging output.  The name is historical - it
   was originally used to debug LinuxThreads support.  */
int debug_threads;

/* Include timestamps in debugging output.  */
int debug_timestamp;

/* Print a debugging message.
   If the text begins a new line it is preceded by a timestamp, if the
   system has gettimeofday.
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
      struct timeval tm;

      gettimeofday (&tm, NULL);

      /* If gettimeofday doesn't exist, and as a portability solution it has
	 been replaced with, e.g., time, then it doesn't make sense to print
	 the microseconds field.  Is there a way to check for that?  */
      fprintf (stderr, "%ld:%06ld ", (long) tm.tv_sec, (long) tm.tv_usec);
    }
#endif

  vfprintf (stderr, format, ap);

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
  fflush (stderr);
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
