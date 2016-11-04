/* Host file transfer support for gdbserver.
   Copyright (C) 2007-2016 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

/* This file implements the hostio_last_error target callback
   on top of errno.  */

#include "server.h"
#include "fileio.h"

void
hostio_last_error_from_errno (char *buf)
{
  int error = errno;
  int fileio_error = host_to_fileio_error (error);
  sprintf (buf, "F-1,%x", fileio_error);
}
