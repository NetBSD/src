/* Copying of files.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "copy-file.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_UTIME || HAVE_UTIMES
# if HAVE_UTIME_H
#  include <utime.h>
# else
#  include <sys/utime.h>
# endif
#endif

#include "error.h"
#include "safe-read.h"
#include "full-write.h"
#include "binary-io.h"
#include "exit.h"
#include "gettext.h"

#define _(str) gettext (str)

void
copy_file_preserving (const char *src_filename, const char *dest_filename)
{
  int src_fd;
  struct stat statbuf;
  int mode;
  int dest_fd;
  char buf[4096];
  const size_t buf_size = sizeof (buf);

  src_fd = open (src_filename, O_RDONLY | O_BINARY);
  if (src_fd < 0 || fstat (src_fd, &statbuf) < 0)
    error (EXIT_FAILURE, errno, _("error while opening \"%s\" for reading"),
	   src_filename);

  mode = statbuf.st_mode & 07777;

  dest_fd = open (dest_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
  if (dest_fd < 0)
    error (EXIT_FAILURE, errno, _("cannot open backup file \"%s\" for writing"),
	   dest_filename);

  /* Copy the file contents.  */
  for (;;)
    {
      size_t n_read = safe_read (src_fd, buf, buf_size);
      if (n_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, _("error reading \"%s\""), src_filename);
      if (n_read == 0)
	break;

      if (full_write (dest_fd, buf, n_read) < n_read)
	error (EXIT_FAILURE, errno, _("error writing \"%s\""), dest_filename);
    }

  if (close (dest_fd) < 0)
    error (EXIT_FAILURE, errno, _("error writing \"%s\""), dest_filename);
  if (close (src_fd) < 0)
    error (EXIT_FAILURE, errno, _("error after reading \"%s\""), src_filename);

  /* Preserve the access and modification times.  */
#if HAVE_UTIME
  {
    struct utimbuf ut;

    ut.actime = statbuf.st_atime;
    ut.modtime = statbuf.st_mtime;
    utime (dest_filename, &ut);
  }
#elif HAVE_UTIMES
  {
    struct timeval ut[2];

    ut[0].tv_sec = statbuf.st_atime; ut[0].tv_usec = 0;
    ut[1].tv_sec = statbuf.st_mtime; ut[1].tv_usec = 0;
    utimes (dest_filename, &ut);
  }
#endif

#if HAVE_CHOWN
  /* Preserve the owner and group.  */
  chown (dest_filename, statbuf.st_uid, statbuf.st_gid);
#endif

  /* Preserve the access permissions.  */
  chmod (dest_filename, mode);
}
