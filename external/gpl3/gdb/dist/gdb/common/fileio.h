/* File-I/O functions for GDB, the GNU debugger.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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

#ifndef FILEIO_H
#define FILEIO_H

#include "gdb/fileio.h"
#include <sys/stat.h>

/* Convert a host-format errno value to a File-I/O error number.  */

extern int host_to_fileio_error (int error);

/* Convert File-I/O open flags FFLAGS to host format, storing
   the result in *FLAGS.  Return 0 on success, -1 on error.  */

extern int fileio_to_host_openflags (int fflags, int *flags);

/* Convert File-I/O mode FMODE to host format, storing
   the result in *MODE.  Return 0 on success, -1 on error.  */

extern int fileio_to_host_mode (int fmode, mode_t *mode);

/* Pack a host-format integer into a byte buffer in big-endian
   format.  BYTES specifies the size of the integer to pack in
   bytes.  */

static inline void
host_to_bigendian (LONGEST num, char *buf, int bytes)
{
  int i;

  for (i = 0; i < bytes; ++i)
    buf[i] = (num >> (8 * (bytes - i - 1))) & 0xff;
}

/* Pack a host-format integer into an fio_uint_t.  */

static inline void
host_to_fileio_uint (long num, fio_uint_t fnum)
{
  host_to_bigendian ((LONGEST) num, (char *) fnum, 4);
}

/* Pack a host-format time_t into an fio_time_t.  */

static inline void
host_to_fileio_time (time_t num, fio_time_t fnum)
{
  host_to_bigendian ((LONGEST) num, (char *) fnum, 4);
}

/* Pack a host-format struct stat into a struct fio_stat.  */

extern void host_to_fileio_stat (struct stat *st, struct fio_stat *fst);

#endif /* FILEIO_H */
