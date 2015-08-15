/* General utility routines for the remote server for GDB.
   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

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

#ifndef IN_PROCESS_AGENT

/* Free a standard argv vector.  */

void
freeargv (char **vector)
{
  char **scan;

  if (vector != NULL)
    {
      for (scan = vector; *scan != NULL; scan++)
	{
	  free (*scan);
	}
      free (vector);
    }
}

#endif

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

/* Print an error message and return to command level.
   STRING is the error message, used as a fprintf string,
   and ARG is passed as an argument to it.  */

void
error (const char *string,...)
{
#ifndef IN_PROCESS_AGENT
  extern jmp_buf toplevel;
#endif
  va_list args;
  va_start (args, string);
  fflush (stdout);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
#ifndef IN_PROCESS_AGENT
  longjmp (toplevel, 1);
#else
  exit (1);
#endif
}

/* Print an error message and exit reporting failure.
   This is for a error that we cannot continue from.
   STRING and ARG are passed to fprintf.  */

/* VARARGS */
void
fatal (const char *string,...)
{
  va_list args;
  va_start (args, string);
  fprintf (stderr, PREFIX);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
  va_end (args);
  exit (1);
}

/* VARARGS */
void
warning (const char *string,...)
{
  va_list args;
  va_start (args, string);
  fprintf (stderr, PREFIX);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
  va_end (args);
}

/* Report a problem internal to GDBserver, and exit.  */

void
internal_error (const char *file, int line, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  fprintf (stderr,  "\
%s:%d: A problem internal to " TOOLNAME " has been detected.\n", file, line);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
  exit (1);
}

/* Temporary storage using circular buffer.  */
#define NUMCELLS 10
#define CELLSIZE 50

/* Return the next entry in the circular buffer.  */

static char *
get_cell (void)
{
  static char buf[NUMCELLS][CELLSIZE];
  static int cell = 0;
  if (++cell >= NUMCELLS)
    cell = 0;
  return buf[cell];
}

static char *
decimal2str (char *sign, ULONGEST addr)
{
  /* Steal code from valprint.c:print_decimal().  Should this worry
     about the real size of addr as the above does? */
  unsigned long temp[3];
  char *str = get_cell ();
  int i = 0;
  int width = 9;

  do
    {
      temp[i] = addr % (1000 * 1000 * 1000);
      addr /= (1000 * 1000 * 1000);
      i++;
    }
  while (addr != 0 && i < (sizeof (temp) / sizeof (temp[0])));

  switch (i)
    {
    case 1:
      xsnprintf (str, CELLSIZE, "%s%0*lu", sign, width, temp[0]);
      break;
    case 2:
      xsnprintf (str, CELLSIZE, "%s%0*lu%09lu", sign, width,
		 temp[1], temp[0]);
      break;
    case 3:
      xsnprintf (str, CELLSIZE, "%s%0*lu%09lu%09lu", sign, width,
		 temp[2], temp[1], temp[0]);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      "failed internal consistency check");
    }

  return str;
}

/* %u for ULONGEST.  The result is stored in a circular static buffer,
   NUMCELLS deep.  */

char *
pulongest (ULONGEST u)
{
  return decimal2str ("", u);
}

/* %d for LONGEST.  The result is stored in a circular static buffer,
   NUMCELLS deep.  */

char *
plongest (LONGEST l)
{
  if (l < 0)
    return decimal2str ("-", -l);
  else
    return decimal2str ("", l);
}

/* Eliminate warning from compiler on 32-bit systems.  */
static int thirty_two = 32;

/* Convert a ULONGEST into a HEX string, like %lx.  The result is
   stored in a circular static buffer, NUMCELLS deep.  */

char *
phex_nz (ULONGEST l, int sizeof_l)
{
  char *str;

  switch (sizeof_l)
    {
    case 8:
      {
	unsigned long high = (unsigned long) (l >> thirty_two);
	str = get_cell ();
	if (high == 0)
	  xsnprintf (str, CELLSIZE, "%lx",
		     (unsigned long) (l & 0xffffffff));
	else
	  xsnprintf (str, CELLSIZE, "%lx%08lx", high,
		     (unsigned long) (l & 0xffffffff));
	break;
      }
    case 4:
      str = get_cell ();
      xsnprintf (str, CELLSIZE, "%lx", (unsigned long) l);
      break;
    case 2:
      str = get_cell ();
      xsnprintf (str, CELLSIZE, "%x", (unsigned short) (l & 0xffff));
      break;
    default:
      str = phex_nz (l, sizeof (l));
      break;
    }

  return str;
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
