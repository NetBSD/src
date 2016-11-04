/* Low-level file-handling.
   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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

#ifndef FILESTUFF_H
#define FILESTUFF_H

/* Note all the file descriptors which are open when this is called.
   These file descriptors will not be closed by close_most_fds.  */

extern void notice_open_fds (void);

/* Mark a file descriptor as inheritable across an exec.  */

extern void mark_fd_no_cloexec (int fd);

/* Mark a file descriptor as no longer being inheritable across an
   exec.  This is only meaningful when FD was previously passed to
   mark_fd_no_cloexec.  */

extern void unmark_fd_no_cloexec (int fd);

/* Close all open file descriptors other than those marked by
   'notice_open_fds', and stdin, stdout, and stderr.  Errors that
   occur while closing are ignored.  */

extern void close_most_fds (void);

/* Like 'open', but ensures that the returned file descriptor has the
   close-on-exec flag set.  */

extern int gdb_open_cloexec (const char *filename, int flags,
			     /* mode_t */ unsigned long mode);

/* Like 'fopen', but ensures that the returned file descriptor has the
   close-on-exec flag set.  */

extern FILE *gdb_fopen_cloexec (const char *filename, const char *opentype);

/* Like 'socketpair', but ensures that the returned file descriptors
   have the close-on-exec flag set.  */

extern int gdb_socketpair_cloexec (int domain, int style, int protocol,
				   int filedes[2]);

/* Like 'socket', but ensures that the returned file descriptor has
   the close-on-exec flag set.  */

extern int gdb_socket_cloexec (int domain, int style, int protocol);

/* Like 'pipe', but ensures that the returned file descriptors have
   the close-on-exec flag set.  */

extern int gdb_pipe_cloexec (int filedes[2]);

/* Return a new cleanup that closes FD.  */

extern struct cleanup *make_cleanup_close (int fd);

#endif /* FILESTUFF_H */
