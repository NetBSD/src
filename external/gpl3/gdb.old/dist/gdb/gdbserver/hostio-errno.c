/* Host file transfer support for gdbserver.
   Copyright (C) 2007-2014 Free Software Foundation, Inc.

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

#include <errno.h>
#include "server.h"
#include "gdb/fileio.h"

static int
errno_to_fileio_error (int error)
{
  switch (error)
    {
    case EPERM:
      return FILEIO_EPERM;
    case ENOENT:
      return FILEIO_ENOENT;
    case EINTR:
      return FILEIO_EINTR;
    case EIO:
      return FILEIO_EIO;
    case EBADF:
      return FILEIO_EBADF;
    case EACCES:
      return FILEIO_EACCES;
    case EFAULT:
      return FILEIO_EFAULT;
    case EBUSY:
      return FILEIO_EBUSY;
    case EEXIST:
      return FILEIO_EEXIST;
    case ENODEV:
      return FILEIO_ENODEV;
    case ENOTDIR:
      return FILEIO_ENOTDIR;
    case EISDIR:
      return FILEIO_EISDIR;
    case EINVAL:
      return FILEIO_EINVAL;
    case ENFILE:
      return FILEIO_ENFILE;
    case EMFILE:
      return FILEIO_EMFILE;
    case EFBIG:
      return FILEIO_EFBIG;
    case ENOSPC:
      return FILEIO_ENOSPC;
    case ESPIPE:
      return FILEIO_ESPIPE;
    case EROFS:
      return FILEIO_EROFS;
    case ENOSYS:
      return FILEIO_ENOSYS;
    case ENAMETOOLONG:
      return FILEIO_ENAMETOOLONG;
    }

  return FILEIO_EUNKNOWN;
}

void
hostio_last_error_from_errno (char *buf)
{
  int error = errno;
  int fileio_error = errno_to_fileio_error (error);
  sprintf (buf, "F-1,%x", fileio_error);
}
