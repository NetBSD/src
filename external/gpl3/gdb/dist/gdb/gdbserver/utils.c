/* General utility routines for the remote server for GDB.
   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifdef IN_PROCESS_AGENT
#  define PREFIX "ipa: "
#  define TOOLNAME "GDBserver in-process agent"
#else
#  define PREFIX "gdbserver: "
#  define TOOLNAME "GDBserver"
#endif

/* Generally useful subroutines used throughout the program.  */

void
malloc_failure (long size)
{
  fprintf (stderr,
	   PREFIX "ran out of memory while trying to allocate %lu bytes\n",
	   (unsigned long) size);
  exit (1);
}

/* Copy a string into a memory buffer.
   If malloc fails, this will print a message to stderr and exit.  */

char *
xstrdup (const char *s)
{
  char *ret = strdup (s);
  if (ret == NULL)
    malloc_failure (strlen (s) + 1);
  return ret;
}

/* Print the system error message for errno, and also mention STRING
   as the file name for which the error was encountered.
   Then return to command level.  */

void
perror_with_name (const char *string)
{
  const char *err;
  char *combined;

  err = strerror (errno);
  if (err == NULL)
    err = "unknown error";

  combined = (char *) alloca (strlen (err) + strlen (string) + 3);
  strcpy (combined, string);
  strcat (combined, ": ");
  strcat (combined, err);

  error ("%s.", combined);
}

/* Print an error message and return to top level.  */

void
verror (const char *string, va_list args)
{
#ifdef IN_PROCESS_AGENT
  fflush (stdout);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
  exit (1);
#else
  throw_verror (GENERIC_ERROR, string, args);
#endif
}

void
vwarning (const char *string, va_list args)
{
  fprintf (stderr, PREFIX);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
}

/* Report a problem internal to GDBserver, and exit.  */

void
internal_verror (const char *file, int line, const char *fmt, va_list args)
{
  fprintf (stderr,  "\
%s:%d: A problem internal to " TOOLNAME " has been detected.\n", file, line);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  exit (1);
}

/* Report a problem internal to GDBserver.  */

void
internal_vwarning (const char *file, int line, const char *fmt, va_list args)
{
  fprintf (stderr,  "\
%s:%d: A problem internal to " TOOLNAME " has been detected.\n", file, line);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
}

/* Convert a CORE_ADDR into a HEX string, like %lx.
   The result is stored in a circular static buffer, NUMCELLS deep.  */

char *
paddress (CORE_ADDR addr)
{
  return phex_nz (addr, sizeof (CORE_ADDR));
}

/* Convert a file descriptor into a printable string.  */

char *
pfildes (gdb_fildes_t fd)
{
#if USE_WIN32API
  return phex_nz (fd, sizeof (gdb_fildes_t));
#else
  return plongest (fd);
#endif
}
