/* Host support routines for MinGW, for GDB, the GNU debugger.

   Copyright (C) 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"

#include "gdb_string.h"

#include "gdb_select.h"

/* The strerror() function can return NULL for errno values that are
   out of range.  Provide a "safe" version that always returns a
   printable string. */

char *
safe_strerror (int errnum)
{
  char *msg;

  msg = strerror (errnum);
  if (msg == NULL)
    {
      static char buf[32];
      xsnprintf (buf, sizeof buf, "(undocumented errno %d)", errnum);
      msg = buf;
    }
  return (msg);
}

/* Wrapper for select.  Nothing special needed on POSIX platforms.  */

int
gdb_select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timeval *timeout)
{
  return select (n, readfds, writefds, exceptfds, timeout);
}
