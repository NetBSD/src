/* Low-level file-handling.
   Copyright (C) 2012-2014 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#include <string.h>
#endif
#include "filestuff.h"
#include "gdb_vecs.h"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef USE_WIN32API
#include <winsock2.h>
#include <windows.h>
#define HAVE_SOCKETS 1
#elif defined HAVE_SYS_SOCKET_H
#include <sys/socket.h>
/* Define HAVE_F_GETFD if we plan to use F_GETFD.  */
#define HAVE_F_GETFD F_GETFD
#define HAVE_SOCKETS 1
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif



#ifndef HAVE_FDWALK

#include <dirent.h>

/* Replacement for fdwalk, if the system doesn't define it.  Walks all
   open file descriptors (though this implementation may walk closed
   ones as well, depending on the host platform's capabilities) and
   call FUNC with ARG.  If FUNC returns non-zero, stops immediately
   and returns the same value.  Otherwise, returns zero when
   finished.  */

static int
fdwalk (int (*func) (void *, int), void *arg)
{
  /* Checking __linux__ isn't great but it isn't clear what would be
     better.  There doesn't seem to be a good way to check for this in
     configure.  */
#ifdef __linux__
  DIR *dir;

  dir = opendir ("/proc/self/fd");
  if (dir != NULL)
    {
      struct dirent *entry;
      int result = 0;

      for (entry = readdir (dir); entry != NULL; entry = readdir (dir))
	{
	  long fd;
	  char *tail;
	  int result;

	  errno = 0;
	  fd = strtol (entry->d_name, &tail, 10);
	  if (*tail != '\0' || errno != 0)
	    continue;
	  if ((int) fd != fd)
	    {
	      /* What can we do here really?  */
	      continue;
	    }

	  if (fd == dirfd (dir))
	    continue;

	  result = func (arg, fd);
	  if (result != 0)
	    break;
	}

      closedir (dir);
      return result;
    }
  /* We may fall through to the next case.  */
#endif

  {
    int max, fd;

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_NOFILE)
    struct rlimit rlim;

    if (getrlimit (RLIMIT_NOFILE, &rlim) == 0 && rlim.rlim_max != RLIM_INFINITY)
      max = rlim.rlim_max;
    else
#endif
      {
#ifdef _SC_OPEN_MAX
	max = sysconf (_SC_OPEN_MAX);
#else
	/* Whoops.  */
	return 0;
#endif /* _SC_OPEN_MAX */
      }

    for (fd = 0; fd < max; ++fd)
      {
	struct stat sb;
	int result;

	/* Only call FUNC for open fds.  */
	if (fstat (fd, &sb) == -1)
	  continue;

	result = func (arg, fd);
	if (result != 0)
	  return result;
      }

    return 0;
  }
}

#endif /* HAVE_FDWALK */



/* A VEC holding all the fds open when notice_open_fds was called.  We
   don't use a hashtab because libiberty isn't linked into gdbserver;
   and anyway we don't expect there to be many open fds.  */

static VEC (int) *open_fds;

/* An fdwalk callback function used by notice_open_fds.  It puts the
   given file descriptor into the vec.  */

static int
do_mark_open_fd (void *ignore, int fd)
{
  VEC_safe_push (int, open_fds, fd);
  return 0;
}

/* See filestuff.h.  */

void
notice_open_fds (void)
{
  fdwalk (do_mark_open_fd, NULL);
}

/* See filestuff.h.  */

void
mark_fd_no_cloexec (int fd)
{
  do_mark_open_fd (NULL, fd);
}

/* See filestuff.h.  */

void
unmark_fd_no_cloexec (int fd)
{
  int i, val;

  for (i = 0; VEC_iterate (int, open_fds, i, val); ++i)
    {
      if (fd == val)
	{
	  VEC_unordered_remove (int, open_fds, i);
	  return;
	}
    }

  gdb_assert_not_reached (_("fd not found in open_fds"));
}

/* Helper function for close_most_fds that closes the file descriptor
   if appropriate.  */

static int
do_close (void *ignore, int fd)
{
  int i, val;

  for (i = 0; VEC_iterate (int, open_fds, i, val); ++i)
    {
      if (fd == val)
	{
	  /* Keep this one open.  */
	  return 0;
	}
    }

  close (fd);
  return 0;
}

/* See filestuff.h.  */

void
close_most_fds (void)
{
  fdwalk (do_close, NULL);
}



/* This is a tri-state flag.  When zero it means we haven't yet tried
   O_CLOEXEC.  When positive it means that O_CLOEXEC works on this
   host.  When negative, it means that O_CLOEXEC doesn't work.  We
   track this state because, while gdb might have been compiled
   against a libc that supplies O_CLOEXEC, there is no guarantee that
   the kernel supports it.  */

static int trust_o_cloexec;

/* Mark FD as close-on-exec, ignoring errors.  Update
   TRUST_O_CLOEXEC.  */

static void
mark_cloexec (int fd)
{
#ifdef HAVE_F_GETFD
  int old = fcntl (fd, F_GETFD, 0);

  if (old != -1)
    {
      fcntl (fd, F_SETFD, old | FD_CLOEXEC);

      if (trust_o_cloexec == 0)
	{
	  if ((old & FD_CLOEXEC) != 0)
	    trust_o_cloexec = 1;
	  else
	    trust_o_cloexec = -1;
	}
    }
#endif /* HAVE_F_GETFD */
}

/* Depending on TRUST_O_CLOEXEC, mark FD as close-on-exec.  */

static void
maybe_mark_cloexec (int fd)
{
  if (trust_o_cloexec <= 0)
    mark_cloexec (fd);
}

#ifdef HAVE_SOCKETS

/* Like maybe_mark_cloexec, but for callers that use SOCK_CLOEXEC.  */

static void
socket_mark_cloexec (int fd)
{
  if (SOCK_CLOEXEC == 0 || trust_o_cloexec <= 0)
    mark_cloexec (fd);
}

#endif



/* See filestuff.h.  */

int
gdb_open_cloexec (const char *filename, int flags, unsigned long mode)
{
  int fd = open (filename, flags | O_CLOEXEC, mode);

  if (fd >= 0)
    maybe_mark_cloexec (fd);

  return fd;
}

/* See filestuff.h.  */

FILE *
gdb_fopen_cloexec (const char *filename, const char *opentype)
{
  FILE *result;
  /* Probe for "e" support once.  But, if we can tell the operating
     system doesn't know about close on exec mode "e" without probing,
     skip it.  E.g., the Windows runtime issues an "Invalid parameter
     passed to C runtime function" OutputDebugString warning for
     unknown modes.  Assume that if O_CLOEXEC is zero, then "e" isn't
     supported.  */
  static int fopen_e_ever_failed_einval = O_CLOEXEC == 0;

  if (!fopen_e_ever_failed_einval)
    {
      char *copy;

      copy = alloca (strlen (opentype) + 2);
      strcpy (copy, opentype);
      /* This is a glibc extension but we try it unconditionally on
	 this path.  */
      strcat (copy, "e");
      result = fopen (filename, copy);

      if (result == NULL && errno == EINVAL)
	{
	  result = fopen (filename, opentype);
	  if (result != NULL)
	    fopen_e_ever_failed_einval = 1;
	}
    }
  else
    result = fopen (filename, opentype);

  if (result != NULL)
    maybe_mark_cloexec (fileno (result));

  return result;
}

#ifdef HAVE_SOCKETS
/* See filestuff.h.  */

int
gdb_socketpair_cloexec (int namespace, int style, int protocol, int filedes[2])
{
#ifdef HAVE_SOCKETPAIR
  int result = socketpair (namespace, style | SOCK_CLOEXEC, protocol, filedes);

  if (result != -1)
    {
      socket_mark_cloexec (filedes[0]);
      socket_mark_cloexec (filedes[1]);
    }

  return result;
#else
  gdb_assert_not_reached (_("socketpair not available on this host"));
#endif
}

/* See filestuff.h.  */

int
gdb_socket_cloexec (int namespace, int style, int protocol)
{
  int result = socket (namespace, style | SOCK_CLOEXEC, protocol);

  if (result != -1)
    socket_mark_cloexec (result);

  return result;
}
#endif

/* See filestuff.h.  */

int
gdb_pipe_cloexec (int filedes[2])
{
  int result;

#ifdef HAVE_PIPE2
  result = pipe2 (filedes, O_CLOEXEC);
  if (result != -1)
    {
      maybe_mark_cloexec (filedes[0]);
      maybe_mark_cloexec (filedes[1]);
    }
#else
#ifdef HAVE_PIPE
  result = pipe (filedes);
  if (result != -1)
    {
      mark_cloexec (filedes[0]);
      mark_cloexec (filedes[1]);
    }
#else /* HAVE_PIPE */
  gdb_assert_not_reached (_("pipe not available on this host"));
#endif /* HAVE_PIPE */
#endif /* HAVE_PIPE2 */

  return result;
}
