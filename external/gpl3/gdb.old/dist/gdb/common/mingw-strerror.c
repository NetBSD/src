/* Safe version of strerror for MinGW, for GDB, the GNU debugger.

   Copyright (C) 2006-2016 Free Software Foundation, Inc.

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

#include "common-defs.h"

#include <windows.h>

/* Implementation of safe_strerror as defined in common-utils.h.

   The Windows runtime implementation of strerror never returns NULL,
   but does return a useless string for anything above sys_nerr;
   unfortunately this includes all socket-related error codes.
   This replacement tries to find a system-provided error message.  */

char *
safe_strerror (int errnum)
{
  static char *buffer;
  int len;

  if (errnum >= 0 && errnum < sys_nerr)
    return strerror (errnum);

  if (buffer)
    {
      LocalFree (buffer);
      buffer = NULL;
    }

  if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER
		     | FORMAT_MESSAGE_FROM_SYSTEM,
		     NULL, errnum,
		     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		     (LPTSTR) &buffer, 0, NULL) == 0)
    {
      static char buf[32];
      xsnprintf (buf, sizeof buf, "(undocumented errno %d)", errnum);
      return buf;
    }

  /* Windows error messages end with a period and a CR-LF; strip that
     out.  */
  len = strlen (buffer);
  if (len > 3 && strcmp (buffer + len - 3, ".\r\n") == 0)
    buffer[len - 3] = '\0';

  return buffer;
}
