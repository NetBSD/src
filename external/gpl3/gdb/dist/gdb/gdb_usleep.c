/* Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdb_usleep.h"
#include "gdb_select.h"

#include <sys/time.h>

int
gdb_usleep (int usec)
{
  struct timeval delay;
  int retval;

  delay.tv_sec = usec / 1000000;
  delay.tv_usec = usec % 1000000;
  retval = gdb_select (0, 0, 0, 0, &delay);

  if (retval < 0)
    retval = -1;
  else
    retval = 0;

  return retval;
}
