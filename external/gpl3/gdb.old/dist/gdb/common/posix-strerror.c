/* Safe version of strerror for POSIX systems for GDB, the GNU debugger.

   Copyright (C) 2006-2017 Free Software Foundation, Inc.

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

/* Implementation of safe_strerror as defined in common-utils.h.  */

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
